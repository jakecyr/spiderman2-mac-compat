#!/usr/bin/env python3
"""Separate local launchers; never print Epic authorization arguments."""
import argparse
import datetime
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['flat', 'vr'])
    parser.add_argument('--render', choices=['keep', 'mono', 'third'], default='keep')
    parser.add_argument('--quest', action='store_true', help='Use the experimental Quest-to-gamepad bridge (VR only)')
    parser.add_argument('--check', action='store_true', help='Validate paths without launching or changing settings')
    args = parser.parse_args()
    if args.quest and args.mode != 'vr':
        parser.error('--quest applies only to VR')
    cfg = json.loads((ROOT / 'local.json').read_text())
    cfg = {k: os.path.expanduser(v) for k, v in cfg.items()}
    state = ROOT / 'state'
    state.mkdir(exist_ok=True)
    lock = (state / 'launcher.lock').open('w')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        raise SystemExit('A launcher is already active. Exit the game before switching modes.')
    vr = Path(cfg['vr_root'])
    cx = Path(cfg['crossover_app']) / 'Contents/SharedSupport/CrossOver'
    required = [cx / 'bin/wine', Path(cfg['legendary'])]
    required += ([vr / 'launch-vr-test.py', vr / 'Game/RealVR.ini', vr / 'Debug/wine-debug-loader']
                 if args.mode == 'vr' else [Path(cfg['flat_game'])])
    for path in required:
        if not path.is_file():
            raise SystemExit(f'Missing required file: {path}')
    if args.check:
        print(f'{args.mode}: required launch paths exist; no game started or settings changed.')
        return
    # Do not edit settings under a game started outside these wrappers.
    processes = subprocess.check_output(['ps', '-axo', 'comm='], text=True)
    if any('Spider-Man2.exe' in x or x.rstrip().endswith('wine-debug-loader') for x in processes.splitlines()):
        raise SystemExit('A game/debug loader is running. Exit it normally first.')
    if args.mode == 'vr':
        if args.render != 'keep':
            ini = vr / 'Game/RealVR.ini'
            data = ini.read_bytes()
            if len(re.findall(rb'(?m)^RenderMode2=\d+', data)) != 1:
                raise SystemExit('Unexpected render configuration; left unchanged.')
            backup = state / ('RealVR-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.ini')
            shutil.copy2(ini, backup)
            # Verified against the installed 2607.31.1 option table: third=2, mono=7.
            value = b'7' if args.render == 'mono' else b'2'
            updated = re.sub(rb'(?m)^RenderMode2=\d+', b'RenderMode2=' + value, data)
            temp = ini.with_suffix('.ini.tmp')
            temp.write_bytes(updated)
            temp.replace(ini)
            print(f'VR render mode: {args.render}; configuration backup saved.', flush=True)
        env = os.environ.copy()
        env.update(SM2_VR_NATIVE_SHARING='1', SM2_VR_SKIP_LAUNCHER='1')
        if args.quest:
            env['SM2_QUEST_GAMEPAD'] = '1'
        raise SystemExit(subprocess.call(['/usr/bin/python3', str(vr / 'launch-vr-test.py')], cwd=vr, env=env))
    if args.render != 'keep':
        raise SystemExit('--render applies only to VR.')
    env = os.environ.copy()
    env['LEGENDARY_CONFIG_PATH'] = cfg['legendary_config']
    result = subprocess.run([
        cfg['legendary'], 'launch', '0c6db5941ac8420aabbd8510e94e7d9a', '--json',
        '--crossover-app', cfg['crossover_app'], '--crossover-bottle', cfg['flat_bottle'],
    ], capture_output=True, text=True, env=env)
    if result.returncode:
        raise SystemExit('Epic authorization failed. Refresh the Epic login in Heroic.')
    meta = json.loads(result.stdout)
    game = Path(cfg['flat_game'])
    if Path(meta['working_directory']).resolve() != game.parent.resolve():
        raise SystemExit('Heroic game directory differs from flat_game; refusing to launch the wrong copy.')
    env.update(meta.get('environment', {}))
    # Never inherit the experimental VR runtime into the original flat bottle.
    for key in ['WINEDLLOVERRIDES', 'WINEPREFIX', 'WINELOADER', 'WINELOADERNOEXEC', 'WINESERVER', 'WINEDLLPATH', 'SM2_NATIVE_RUN', 'SM2_NO_KEYED_MUTEX', 'SM2_QUEST_GAMEPAD', 'XR_RUNTIME_JSON', 'XR_API_LAYER_PATH', 'DYLD_INSERT_LIBRARIES']:
        env.pop(key, None)
    env.update(CX_BOTTLE=cfg['flat_bottle'], CX_ROOT=str(cx), CX_GRAPHICS_BACKEND='d3dmetal',
               WINEMSYNC='0', WINEESYNC='0', WINEFSYNC='0', ROSETTA_ADVERTISE_AVX='1', WINEDEBUG='-all')
    command = [str(cx / 'bin/wine'), '--bottle', cfg['flat_bottle'], '--no-update', str(game)]
    for key in ['game_parameters', 'egl_parameters', 'user_parameters']:
        command += meta.get(key, [])
    print('Launching original flat game with CrossOver; VR modifications are isolated.', flush=True)
    log_path = state / 'flat-launch.log'
    with log_path.open('w') as log:
        proc = subprocess.Popen(command, cwd=game.parent, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors='replace')
        (state / 'flat-pid.txt').write_text(str(proc.pid))
        for line in proc.stdout:
            if any(word in line.lower() for word in ['auth_password', 'auth_login', 'exchange_code', 'access_token', 'refresh_token', 'command line', 'commandline']):
                log.write('[authorization/command-line diagnostic omitted]\n')
            else:
                log.write(line)
            log.flush()
        code = proc.wait()
    print(f'Flat game exited ({code}). Diagnostics: {log_path}', flush=True)
    raise SystemExit(code)


if __name__ == '__main__':
    main()
