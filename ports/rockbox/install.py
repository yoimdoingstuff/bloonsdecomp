"""Install the Bloons source adapter into a user-selected Rockbox checkout.

Only this plugin's marked directory and its exact registration lines are changed.
This does not build firmware, flash a player, or write to an iPod.
"""
import argparse
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
MARKER='.bloons-port'
REGISTRATION='''
/* Bloons: color iPod Video/Classic */
#if defined(HAVE_LCD_COLOR) && LCD_WIDTH == 320 && LCD_HEIGHT == 240 && CONFIG_KEYPAD == IPOD_4G_PAD
bloons
#endif
'''


def install(rockbox):
    rockbox=rockbox.resolve()
    plugins=rockbox/'apps/plugins'
    for required in (rockbox/'apps/plugin.h',plugins/'SUBDIRS',plugins/'CATEGORIES'):
        if not required.is_file():
            raise ValueError(f'Not a Rockbox source checkout: missing {required}')
    destination=plugins/'bloons'
    if not destination.resolve().is_relative_to(rockbox):
        raise ValueError('Plugin destination resolves outside the Rockbox checkout')
    if destination.exists() and not (destination/MARKER).is_file():
        raise ValueError(f'Refusing to overwrite an unrelated directory: {destination}')
    subdirs=(plugins/'SUBDIRS').read_bytes().decode('utf-8')
    categories=(plugins/'CATEGORIES').read_bytes().decode('utf-8')
    if 'bloons' in subdirs.splitlines() and REGISTRATION.strip() not in subdirs:
        raise ValueError('An existing Bloons registration needs manual review')
    destination.mkdir(exist_ok=True)
    (destination/'src').mkdir(exist_ok=True)
    for name in ('game.cpp','game.hpp','levels.cpp','swf_data.hpp','storage.hpp','core_math.hpp'):
        shutil.copy2(ROOT/'src'/name,destination/'src'/name)
    for name in ('bridge.h','bridge.cpp','render.h','render.c','rockbox.c','bloons.make'):
        shutil.copy2(ROOT/'ports/rockbox'/name,destination/name)
    shutil.copy2(ROOT/'LICENSE',destination/'COPYING')
    (destination/MARKER).write_text('Managed by bloonsdecomp ports/rockbox/install.py\n')
    if REGISTRATION.strip() not in subdirs:
        (plugins/'SUBDIRS').write_bytes((subdirs+('' if subdirs.endswith('\n') else '\n')+REGISTRATION).encode('utf-8'))
    if 'bloons,games' not in categories.splitlines():
        (plugins/'CATEGORIES').write_bytes((categories+('' if categories.endswith('\n') else '\n')+'bloons,games\n').encode('utf-8'))
    print(f'Installed source: {destination}')
    print('Configure Rockbox for your actual iPod model, then build with a matching C++17 compiler and libm.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rockbox',type=Path,required=True)
    install(parser.parse_args().rockbox)
