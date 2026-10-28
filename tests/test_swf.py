"""Metadata parsing regressions; no Flash runtime or external SWF required."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_swf import Reader, inspect


def packed(fields):
    bits = ''.join(format(value & ((1 << width) - 1), f'0{width}b')
                   for value, width in fields if width)
    bits += '0' * (-len(bits) % 8)
    return int(bits, 2).to_bytes(len(bits) // 8, 'big')


def rectangle(values):
    return packed([(16, 5)] + [(value, 16) for value in values])


def matrix(tx, ty):
    return packed([(0, 1), (0, 1), (12, 5), (tx, 12), (ty, 12)])


def tag(code, payload=b''):
    if len(payload) >= 63:
        return struct.pack('<HI', (code << 6) | 63, len(payload)) + payload
    return struct.pack('<H', (code << 6) | len(payload)) + payload


def fixture(compressed):
    body = rectangle([0, 12800, 0, 9600]) + struct.pack('<HH', 50 * 256, 3)
    body += tag(2, struct.pack('<H', 1) + rectangle([-200, 600, -400, 800]) + b'\x00\x00')
    body += tag(43, b'intro\x00')
    body += tag(56, struct.pack('<HH', 1, 1) + b'reference_only\x00')
    body += tag(14, struct.pack('<HBIh', 23, 0x26, 1398, 0))
    body += tag(15, struct.pack('<HB', 23, 0))
    body += tag(26, struct.pack('<BHH', 6, 7, 1) + matrix(500, -100))
    body += tag(1)
    body += tag(26, struct.pack('<BH', 5, 7) + matrix(-200, 300))
    body += tag(1)
    body += tag(28, struct.pack('<H', 7)) + tag(1) + tag(0)
    return (b'CWS' if compressed else b'FWS') + bytes([8]) + struct.pack('<I', len(body) + 8) + (
        zlib.compress(body) if compressed else body)


class MetadataTests(unittest.TestCase):
    def test_signed_rotation_scale_and_translation(self):
        data = packed([(1, 1), (18, 5), (98304, 18), (32768, 18),
                       (1, 1), (17, 5), (16384, 17), (-32768, 17),
                       (12, 5), (-400, 12), (800, 12)])
        self.assertEqual(Reader(data).matrix(), [1.5, .25, -.5, .5, -20, 40])

    def test_color_transform_alignment(self):
        data = packed([(1, 1), (1, 1), (10, 4)] + [(256, 10)] * 3 + [(128, 10)] +
                      [(-20, 10), (30, 10), (0, 10), (-5, 10)]) + b'after\x00'
        reader = Reader(data)
        self.assertEqual(reader.color_transform(), {'mult': [1, 1, 1, .5], 'add': [-20, 30, 0, -5]})
        self.assertEqual(reader.string(), 'after')

    def test_compression_display_list_and_sound(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'reference.swf'
            for compressed in (False, True):
                with self.subTest(compressed=compressed):
                    path.write_bytes(fixture(compressed))
                    result = inspect(path)
                    self.assertEqual(result['frame_rate'], 50)
                    self.assertEqual(result['stage']['xmax'], 640)
                    self.assertEqual(result['root']['labels'], {'intro': 1})
                    self.assertEqual(result['exports'], {'reference_only': 1})
                    self.assertEqual(result['root']['frames'][0]['7']['matrix'][4:], [25, -5])
                    self.assertEqual(result['root']['frames'][1]['7']['character'], 1)
                    self.assertEqual(result['root']['frames'][1]['7']['matrix'][4:], [-10, 15])
                    self.assertEqual(result['root']['frames'][2], {})
                    self.assertEqual(result['sounds']['23']['samples'], 1398)
                    self.assertEqual(result['root']['start_sounds'], [{'frame': 1, 'sound': 23}])

    def test_truncation_is_rejected(self):
        with self.assertRaises(ValueError):
            Reader(b'\xff').rect()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'truncated.swf'
            path.write_bytes(fixture(False)[:-3])
            with self.assertRaises(ValueError):
                inspect(path)


if __name__ == '__main__':
    unittest.main()
