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
EPIC_GAME = '0c6db5941ac8420aabbd8510e94e7d9a'
D3DMETAL_LICENSE = 'apple-gptk-license-2023-08-17'
DEFAULT_HIGHBALL_HOME = Path.home() / 'Library/Application Support/Highball'


def highball_runtime(cfg):
    home = Path(cfg.get('highball_home', DEFAULT_HIGHBALL_HOME))
    bottle = home / 'bottles' / cfg.get('highball_bottle', 'Games')
    try:
        settings = json.loads((bottle / 'bottle.json').read_text())
        engine = home / 'engines' / settings['engineID']
        manifest = json.loads((engine / 'manifest.json').read_text())
    except (OSError, ValueError, KeyError, TypeError):
        raise SystemExit(
            f'Could not read runtime settings for the {bottle.name} environment. Open Highball and finish setup.'
        ) from None
    if D3DMETAL_LICENSE not in (manifest.get('acceptedLicenses') or []):
        raise SystemExit('Review the Apple Game Porting Toolkit license in Highball to enable D3DMetal.')
    if manifest.get('arch') != 'x86_64':
        raise SystemExit('Select a Highball Wine engine that runs through Rosetta for this D3DMetal launch.')

    frameworks = engine / 'frameworks'
    renderers = {}
    for name in ('d3dmetal', 'dxmt', 'd9vk'):
        for root in (engine / 'renderers', frameworks / 'renderer'):
            renderer = root / name
            if (renderer / 'wine').is_dir():
                renderers[name] = renderer
                break
    if 'd3dmetal' not in renderers:
        raise SystemExit('D3DMetal is missing from this Highball engine.')
    external = renderers['d3dmetal'] / 'external'
    if not (external / 'D3DMetal.framework').is_dir():
        raise SystemExit('The Highball engine is missing D3DMetal.framework.')

    gstreamer = frameworks / 'GStreamer.framework/Versions/1.0/lib'
    return engine / 'engine/bin/wine', {
        'WINEPREFIX': str(bottle),
        'WINEDEBUG': '-all',
        'WINEMSYNC': '0',
        'WINEESYNC': '0',
        'ROSETTA_ADVERTISE_AVX': '1',
        'CX_D3DMETALPATH': str(external),
        'WINEDLLPATH_PREPEND': ':'.join(str(renderer / 'wine') for renderer in renderers.values()),
        'WINEDLLOVERRIDES': 'winemenubuilder.exe=d',
        'DYLD_FALLBACK_FRAMEWORK_PATH': f'{external}:{frameworks}',
        'DYLD_FALLBACK_LIBRARY_PATH': f'{external}:{frameworks}:{gstreamer}',
        'GST_PLUGIN_PATH': str(gstreamer / 'gstreamer-1.0'),
        'MVK_CONFIG_LOG_LEVEL': '2',
    }


def epic_arguments(legendary, game, env):
    # Keep Epic's one-time credentials in memory and omit Legendary's captured output on failure.
    try:
        result = subprocess.run(
            [legendary, 'launch', EPIC_GAME, '--json', '--skip-version-check', '--no-wine'],
            env=env, capture_output=True, text=True, timeout=60, check=True,
        )
        launch = json.loads(result.stdout[result.stdout.index('{'):])
        exe = (Path(launch['game_directory']) / launch['game_executable']).resolve()
        arguments = launch.get('egl_parameters', []) + launch.get('game_parameters', [])
        if not isinstance(arguments, list) or not all(isinstance(arg, str) for arg in arguments):
            raise ValueError('Invalid Epic launch arguments')
    except (OSError, subprocess.SubprocessError, ValueError, KeyError, TypeError, AttributeError):
        raise SystemExit('Epic authorization failed. Refresh your Epic sign-in in Heroic and retry.') from None
    if exe != game.resolve():
        raise SystemExit("Legendary's installed executable does not match flat_game in local.json.")
    return arguments


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['flat', 'highball', 'vr'])
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
    required = [Path(cfg['legendary'])]
    if args.mode == 'highball':
        wine, runtime_env = highball_runtime(cfg)
        required.extend([wine, Path(cfg['legendary_config']) / 'installed.json'])
    else:
        cx = Path(cfg['crossover_app']) / 'Contents/SharedSupport/CrossOver'
        required.append(cx / 'bin/wine')
    if args.mode == 'vr':
        vr = Path(cfg['vr_root'])
        required.extend([vr / 'launch-vr-test.py', vr / 'Game/RealVR.ini', vr / 'Debug/wine-debug-loader'])
    else:
        game = Path(cfg['flat_game'])
        required.append(game)
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
    # Each launch mode supplies its own Wine and graphics settings.
    for key in ['WINEDLLOVERRIDES', 'WINEPREFIX', 'WINELOADER', 'WINELOADERNOEXEC',
                'WINESERVER', 'WINEDLLPATH', 'WINEMSYNC', 'WINEESYNC', 'WINEFSYNC',
                'CX_ROOT', 'CX_BOTTLE', 'CX_GRAPHICS_BACKEND', 'SM2_NATIVE_RUN',
                'SM2_NO_KEYED_MUTEX', 'SM2_QUEST_GAMEPAD', 'XR_RUNTIME_JSON',
                'XR_API_LAYER_PATH', 'DYLD_INSERT_LIBRARIES']:
        env.pop(key, None)
    secrets = []
    if args.mode == 'highball':
        arguments = epic_arguments(cfg['legendary'], game, env)
        secrets = [arg.partition('=')[2] for arg in arguments if arg.upper().startswith('-AUTH_')]
        env.update(runtime_env)
        command = [str(wine), str(game), *arguments, '-nolauncher']
        print('Launching Spider-Man 2 using Highball’s Wine engine and the existing Epic installation.', flush=True)
    else:
        command = [cfg['legendary'], 'launch', EPIC_GAME,
                   '--crossover-app', cfg['crossover_app'], '--crossover-bottle', cfg['flat_bottle']]
        print('Launching original flat game with CrossOver; VR modifications are isolated.', flush=True)
    log_path = state / f'{args.mode}-launch.log'
    os.umask(0o077)
    with log_path.open('w') as log:
        proc = subprocess.Popen(command, cwd=game.parent, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors='replace')
        (state / f'{args.mode}-pid.txt').write_text(str(proc.pid))
        for line in proc.stdout:
            if any(word in line.lower() for word in ['auth_password', 'auth_login', 'exchange_code', 'access_token', 'refresh_token', 'command line', 'commandline']):
                log.write('[authorization/command-line diagnostic omitted]\n')
            else:
                for secret in secrets:
                    if secret:
                        line = line.replace(secret, '[redacted]')
                log.write(line)
            log.flush()
        code = proc.wait()
    print(f'{args.mode.capitalize()} game exited ({code}). Diagnostics: {log_path}', flush=True)
    raise SystemExit(code)


if __name__ == '__main__':
    main()
