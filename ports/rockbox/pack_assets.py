"""Pack the existing original PNGs into a bounded RGBA atlas for color iPods.

Requires Pillow at build time only. No new artwork or runtime image decoder.
"""
from pathlib import Path
import argparse
import json
import re
import struct
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]


def pack(destination):
    header=(ROOT/'src/swf_data.hpp').read_text()
    sprite_table=header.split('sprites{{',1)[1].split('}};',1)[0]
    bounds={int(k):[float(x) for x in values.split(',')]
            for k,values in re.findall(r'\{(\d+),\{([^}]+)\}\}',sprite_table)}
    ids={8,12,33,37,49,63,80,82,152,155,163,170,271,275}
    styles=header.split('balloons{{',1)[1].split('inline constexpr int balloonSymbol',1)[0]
    ids.update(int(body) for _,body,_ in re.findall(r'\{(\d+),(\d+),(\d+),\{\{',styles))
    folders={int(p.name.split('_')[1]):p for p in (ROOT/'sprites').iterdir() if p.is_dir()}
    entries=[]
    # The background PNG uses the union registration recorded by the SWF.
    bg=Image.open(folders[187]/'1.png').convert('RGBA')
    bx,by,_,_=bounds[187]
    bg=bg.crop((round(-bx),round(-by),round(-bx)+640,round(-by)+480))
    bg=bg.resize((320,240),Image.Resampling.LANCZOS)
    entries.append((10000,0.,0.,2.,2.,bg))
    for identifier in sorted(ids):
        image=Image.open(folders[identifier]/'1.png').convert('RGBA')
        crop=image.getbbox()
        if crop is None:
            raise ValueError(f'Empty required sprite {identifier}')
        image=image.crop(crop)
        original_width,original_height=image.size
        image.thumbnail((192,192),Image.Resampling.LANCZOS)
        x,y,_,_=bounds[identifier]
        entries.append((identifier,x+crop[0],y+crop[1],original_width/image.width,
                        original_height/image.height,image))
    offset=12+len(entries)*28
    records=bytearray(); pixels=bytearray(); summary=[]
    for identifier,x,y,sx,sy,image in entries:
        records+=struct.pack('<IffffHHI',identifier,x,y,sx,sy,image.width,image.height,offset+len(pixels))
        pixels+=image.tobytes()
        summary.append({'id':identifier,'width':image.width,'height':image.height})
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_bytes(b'BLRB'+struct.pack('<II',1,len(entries))+records+pixels)
    destination.with_suffix('.json').write_text(json.dumps({'bytes':destination.stat().st_size,
                                                          'sprites':summary},indent=2)+'\n')
    print(f'{destination}: {len(entries)} original sprites, {destination.stat().st_size} bytes')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'build/rockbox/bloons.dat')
    pack(parser.parse_args().output)
