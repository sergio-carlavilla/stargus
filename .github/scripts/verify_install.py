#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Sergio Carlavilla Delgado <sergio.carlavilla91@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only

"""Check the installed Startool4/Stargus layout without original game data."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('--desktop', action='store_true')
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    suffix = '.exe' if os.name == 'nt' else ''
    runtime = prefix / 'share/games/stratagus/stargus'
    importer = prefix / 'games' / ('startool4' + suffix)
    launcher = prefix / 'games' / ('stargus' + suffix)

    required = [importer, launcher, runtime / 'scripts/stratagus.lua',
                runtime / 'contrib/fog.png', runtime / 'contrib/transparent.png']
    required += [runtime / 'manifests' / (name + '.json')
                 for name in ['classic', 'broodwar', 'casc-classic', 'casc-broodwar']]
    required.append(runtime / 'manifests/classic/portraits.json')
    if args.desktop:
        required += [prefix / name for name in [
            'share/pixmaps/stargus.png',
            'share/pixmaps/stargus.xpm',
            'share/applications/stargus.desktop']]
    for path in required:
        if not path.is_file():
            raise RuntimeError('Missing installed file: ' + str(path))
    for path in [importer, launcher]:
        if not os.access(path, os.X_OK):
            raise RuntimeError('Installed executable is not executable: ' + str(path))

    retired = ['startool', 'scdat2json', 'grptool', 'sauwetter', 'iscript', 'json_transition']
    for directory in ['bin', 'games']:
        for name in retired:
            path = prefix / directory / (name + suffix)
            if path.exists():
                raise RuntimeError('Retired tool is still installed: ' + str(path))
    for path in [runtime / 'dataset', runtime / 'mpqlist.txt']:
        if path.exists():
            raise RuntimeError('Retired data is still installed: ' + str(path))

    # Run from outside the checkout to also check installed runtime linking.
    version = subprocess.run([str(importer), '-V'], cwd=prefix, check=True, capture_output=True, text=True, timeout=30).stdout.strip()
    if not version:
        raise RuntimeError('Installed Startool4 returned an empty data version')
    for option in ['--version', '--help']:
        subprocess.run([str(importer), option], cwd=prefix, check=True, capture_output=True, text=True, timeout=30)
    if args.desktop:
        desktop = (prefix / 'share/applications/stargus.desktop').read_text()
        if 'Exec=' + launcher.as_posix() not in desktop.splitlines():
            raise RuntimeError('Desktop entry does not point to the installed launcher')
    print('PASS: installed Startool4 ' + version + ', Stargus, manifests and assets; retired files absent')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
