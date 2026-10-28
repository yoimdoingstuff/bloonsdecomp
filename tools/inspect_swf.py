"""Read SWF metadata without executing ActionScript (Python standard library only).

Reference: Adobe SWF File Format Specification, RECT/MATRIX/CXFORM and display
list tags. Output retains frame display lists so placements remain auditable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


class Reader:
    def __init__(self, data):
        self.data, self.bit = data, 0

    def bits(self, count, signed=False):
        if self.bit + count > len(self.data) * 8:
            raise ValueError('Truncated SWF bit field')
        value = 0
        for _ in range(count):
            value = (value << 1) | ((self.data[self.bit // 8] >> (7 - self.bit % 8)) & 1)
            self.bit += 1
        if signed and count and value & (1 << (count - 1)):
            value -= 1 << count
        return value

    def align(self):
        self.bit = (self.bit + 7) // 8 * 8

    def take(self, count):
        self.align()
        start = self.bit // 8
        if start + count > len(self.data):
            raise ValueError('Truncated SWF byte field')
        self.bit += count * 8
        return self.data[start:start + count]

    def u8(self):
        return self.take(1)[0]

    def u16(self):
        return struct.unpack('<H', self.take(2))[0]

    def string(self):
        chars = bytearray()
        while (value := self.u8()) != 0:
            chars.append(value)
        return chars.decode('utf-8')

    def rect(self):
        self.align()
        n = self.bits(5)
        values = [self.bits(n, True) / 20 for _ in range(4)]
        self.align()
        return dict(zip(('xmin', 'xmax', 'ymin', 'ymax'), values))

    def matrix(self):
        self.align()
        a = d = 1.0
        b = c = 0.0
        if self.bits(1):
            n = self.bits(5)
            a, d = [self.bits(n, True) / 65536 for _ in range(2)]
        if self.bits(1):
            n = self.bits(5)
            b, c = [self.bits(n, True) / 65536 for _ in range(2)]
        n = self.bits(5)
        tx, ty = [self.bits(n, True) / 20 for _ in range(2)]
        self.align()
        return [a, b, c, d, tx, ty]

    def color_transform(self, alpha=True):
        self.align()
        has_add, has_mult, n = self.bits(1), self.bits(1), self.bits(4)
        channels = 4 if alpha else 3
        mult = [self.bits(n, True) / 256 for _ in range(channels)] if has_mult else [1] * channels
        add = [self.bits(n, True) for _ in range(channels)] if has_add else [0] * channels
        self.align()
        return {'mult': mult, 'add': add}


def inspect(path):
    original = path.read_bytes()
    if original[:3] not in (b'FWS', b'CWS'):
        raise ValueError('Expected uncompressed or zlib-compressed SWF')
    declared = struct.unpack('<I', original[4:8])[0]
    body = zlib.decompress(original[8:]) if original[:3] == b'CWS' else original[8:]
    if len(body) + 8 != declared:
        raise ValueError('SWF length does not match header')
    reader = Reader(body)
    stage = reader.rect()
    rate, frames = reader.u16() / 256, reader.u16()
    definitions, exports, sounds = {}, {}, {}

    def timeline(data):
        stream = Reader(data)
        display, snapshots, labels, starts, counts = {}, [], {}, [], {}
        while stream.bit < len(data) * 8:
            header = stream.u16()
            code, size = header >> 6, header & 63
            if size == 63:
                size = struct.unpack('<I', stream.take(4))[0]
            payload = stream.take(size)
            r = Reader(payload)
            counts[code] = counts.get(code, 0) + 1
            if code == 0:
                break
            if code == 1:
                snapshots.append({str(k): dict(v) for k, v in sorted(display.items())})
            elif code in (2, 22, 32, 83, 11, 33, 37):
                character = r.u16()
                definition = {'tag': code, 'bounds': r.rect()}
                if code in (11, 33):
                    definition['matrix'] = r.matrix()
                if code == 37:
                    flags = r.bits(16)
                    definition['flags'] = flags
                    if flags & 0x100:  # HasFont
                        definition['font'] = r.u16()
                    if flags & 0x80:  # HasFontClass
                        definition['font_class'] = r.string()
                    if flags & 0x100:
                        definition['font_height'] = r.u16() / 20
                    if flags & 0x400:
                        definition['color'] = list(r.take(4))
                    if flags & 2:
                        definition['max_length'] = r.u16()
                    if flags & 0x20:
                        definition['layout'] = [r.u8(), r.u16()/20, r.u16()/20, r.u16()/20,
                                                struct.unpack('<h', r.take(2))[0]/20]
                    definition['variable'] = r.string()
                    if flags & 0x8000:
                        definition['text'] = r.string()
                definitions[str(character)] = definition
            elif code == 39:
                character, frame_count = r.u16(), r.u16()
                definitions[str(character)] = {'tag': code, 'frame_count': frame_count,
                                               **timeline(payload[4:])}
            elif code in (46, 84):
                character = r.u16()
                start, end = r.rect(), r.rect()
                definitions[str(character)] = {'tag': code, 'start_bounds': start, 'end_bounds': end,
                    'bounds': {key: (min if key.endswith('min') else max)(start[key], end[key])
                               for key in start}}
            elif code in (7, 34):
                character = r.u16()
                if code == 34:
                    r.u8()  # TrackAsMenu / reserved
                    r.u16()  # ActionOffset; button actions are never executed
                records = []
                while (flags := r.u8()) != 0:
                    item = {'character': r.u16(), 'depth': r.u16(), 'matrix': r.matrix(),
                            'states': flags & 15}
                    if code == 34:
                        item['color_transform'] = r.color_transform()
                    if flags & 0x30:
                        raise ValueError('Button filters/blend mode need explicit parsing')
                    records.append(item)
                definitions[str(character)] = {'tag': code, 'records': records}
            elif code in (56, 76):
                for _ in range(r.u16()):
                    character = r.u16()
                    exports[r.string()] = character
            elif code == 14:
                character, flags = r.u16(), r.u8()
                samples = struct.unpack('<I', r.take(4))[0]
                sounds[str(character)] = {'format': flags >> 4, 'rate_index': (flags >> 2) & 3,
                                          'bits': 16 if flags & 2 else 8, 'channels': 2 if flags & 1 else 1,
                                          'samples': samples}
            elif code == 15:
                starts.append({'frame': len(snapshots) + 1, 'sound': r.u16()})
            elif code == 43:
                labels[r.string()] = len(snapshots) + 1
            elif code in (26, 70):
                flags = r.u8()
                flags2 = r.u8() if code == 70 else 0
                depth = r.u16()
                item = dict(display.get(depth, {})) if flags & 1 else {}
                if flags2 & 8 or (flags2 & 16 and flags & 2):
                    item['class'] = r.string()
                if flags & 2:
                    item['character'] = r.u16()
                if flags & 4:
                    item['matrix'] = r.matrix()
                if flags & 8:
                    item['color_transform'] = r.color_transform()
                if flags & 16:
                    item['ratio'] = r.u16()
                if flags & 32:
                    item['name'] = r.string()
                if flags & 64:
                    item['clip_depth'] = r.u16()
                # Filters, blend/cache hints and clip action bytecode are not
                # interpreted or executed. They do not affect fields above.
                display[depth] = item
            elif code in (5, 28):
                if code == 5:
                    r.u16()
                display.pop(r.u16(), None)
            elif code == 4:
                character, depth = r.u16(), r.u16()
                display[depth] = {'character': character, 'matrix': r.matrix()}
        return {'frames': snapshots, 'labels': labels, 'start_sounds': starts, 'tag_counts': counts}

    root = timeline(body[reader.bit // 8:])
    if len(root['frames']) != frames:
        raise ValueError('SWF frame count does not match ShowFrame tags')
    return {'sha256': hashlib.sha256(original).hexdigest(), 'version': original[3],
            'stage': stage, 'frame_rate': rate, 'frame_count': frames, 'exports': exports,
            'sounds': sounds, 'root': root, 'definitions': definitions}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('swf', type=Path)
    parser.add_argument('--output', type=Path, default=Path('reference/swf_metadata.json'))
    args = parser.parse_args()
    metadata = inspect(args.swf)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: metadata[key] for key in ('sha256', 'version', 'stage', 'frame_rate', 'frame_count')}))
    print('Root labels:', metadata['root']['labels'])
    print('Exported sounds:', {name: identifier for name, identifier in metadata['exports'].items()
                                if str(identifier) in metadata['sounds']})
