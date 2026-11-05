#!/usr/bin/env python3
"""Install user-downloaded script mods only into the isolated VR game."""
import argparse
import datetime
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def script_package(path):
    """Read a .script ZIP, or a single .script nested in a downloaded archive."""
    data = Path(path).read_bytes()
    if zipfile.is_zipfile(io.BytesIO(data)):
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            if 'info.json' in z.namelist():
                return z.read('info.json'), {n: z.read(n) for n in z.namelist() if n.lower().endswith('.dll')}
            nested = [n for n in z.namelist() if n.lower().endswith('.script')]
            if len(nested) != 1:
                raise ValueError('Archive must contain exactly one .script package')
            data = z.read(nested[0])
    else:
        listing = subprocess.check_output(['tar', '-tf', str(path)], text=True).splitlines()
        nested = [n for n in listing if n.lower().endswith('.script')]
        if len(nested) != 1:
            raise ValueError('Archive must contain exactly one .script package')
        data = subprocess.check_output(['tar', '-xOf', str(path), nested[0]])
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        return z.read('info.json'), {n: z.read(n) for n in z.namelist() if n.lower().endswith('.dll')}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('camera', type=Path)
    ap.add_argument('modsettings', type=Path)
    ap.add_argument('--loader', type=Path, default=ROOT/'state/Overstrike_v1.9.0.zip')
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args()
    config = json.loads((ROOT/'local.json').read_text())
    vr = Path(os.path.expanduser(config['vr_root'])).resolve()
    game = vr/'Game'
    flat = Path(os.path.expanduser(config['flat_game'])).resolve().parent
    if game.resolve() == flat or game.is_symlink() or not (game/'Spider-Man2.exe').is_file():
        raise SystemExit('Refusing install: isolated VR game directory not verified')
    if (game/'scripts').is_symlink():
        raise SystemExit('Refusing symlink scripts directory')
    payload = {}
    for archive, expected in [(a.modsettings, 'modsettings.dll'), (a.camera, 'sm2_first_person_camera.dll')]:
        raw, files = script_package(archive)
        info = json.loads(raw)
        if info.get('game') != 'MSM2' or info.get('type') != 'script':
            raise SystemExit(f'Not a Spider-Man 2 script package: {archive.name}')
        matching = [data for name, data in files.items() if name == expected]
        if len(matching) != 1 or not matching[0].startswith(b'MZ'):
            raise SystemExit(f'Package lacks the required runtime DLL: {expected}')
        payload['scripts/'+expected] = matching[0]
        print(f"Validated {info.get('name')} {info.get('version')}")
    with zipfile.ZipFile(a.loader) as z:
        payload['winmm.dll'] = z.read('scripts_proxy.dll')
    existing = (game/'scripts.txt').read_text().splitlines() if (game/'scripts.txt').exists() else []
    other = [x.strip() for x in existing if x.strip() and x.strip().lower() not in ('modsettings.dll','sm2_first_person_camera.dll')]
    payload['scripts.txt'] = ('\r\n'.join(['modsettings.dll','sm2_first_person_camera.dll']+other)+'\r\n').encode()
    if a.check:
        print('Preflight passed; nothing installed.')
        return
    processes = subprocess.check_output(['ps','-axo','comm='],text=True)
    if any('Spider-Man2.exe' in line or line.endswith('wine-debug-loader') for line in processes.splitlines()):
        raise SystemExit('Exit the game before installing camera scripts.')
    backup = ROOT/'state'/('camera-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    manifest = {}
    for name, data in payload.items():
        target = game/name
        if target.is_symlink():
            raise SystemExit(f'Refusing symlink target: {name}')
        if target.exists():
            saved = backup/name
            saved.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(target,saved)
        manifest[name] = {'existed':target.exists(),'sha256_installed':hashlib.sha256(data).hexdigest()}
    (backup/'manifest.json').write_text(json.dumps(manifest,indent=2))
    for name,data in payload.items():
        target=game/name;target.parent.mkdir(parents=True,exist_ok=True)
        fd,temp=tempfile.mkstemp(dir=target.parent,prefix=target.name+'.')
        with os.fdopen(fd,'wb') as f:f.write(data)
        os.replace(temp,target)
    marker = {'backup':str(backup),'files':manifest,'enabled':True}
    (vr/'Records/camera-mod-install.json').write_text(json.dumps(marker,indent=2))
    print('Installed into isolated VR copy. Backup:',backup)
    print('Launch requires native winmm override and -scripts; launcher integration must be enabled separately.')

if __name__ == '__main__':
    main()
