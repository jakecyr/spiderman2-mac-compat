#!/usr/bin/env python3
"""Launch only the isolated Epic Spider-Man 2 VR test; keep exchange tokens in memory."""
import json
import fcntl
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent
_launch_lock = (ROOT / 'Records' / 'launch.lock').open('w')
try:
    fcntl.flock(_launch_lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    raise SystemExit('The isolated test is already running.')
GAME = ROOT / 'Game' / 'Spider-Man2.exe'
WINE = Path('/Users/YOUR_USER/Applications/CrossOver 25.1.1.app/Contents/SharedSupport/CrossOver/bin/wine')
LEGENDARY = '/Applications/Heroic.app/Contents/Resources/app.asar.unpacked/build/bin/arm64/darwin/legendary'
BOTTLE = Path('/Users/YOUR_USER/Library/Application Support/CrossOver/Bottles/SpiderMan2-VR-Test')
managed = os.environ.get('SM2_VR_RUNNER') == 'molten'
if managed:
    BOTTLE = ROOT / 'ManagedBottle'
    WINE = Path('/Users/YOUR_USER/Library/Application Support/MoltenVR/wine-d3dmetal/bin/wine64')
docs = BOTTLE / 'drive_c/users/crossover/Documents'
assert docs.is_dir() and not docs.is_symlink(), 'Test saves must stay isolated.'
if not managed:
    assert '"Version" = "25.1.1.38624"' in (BOTTLE / 'cxbottle.conf').read_text(), 'Restore the test bottle to CrossOver 25.1.1 before launching.'
env = os.environ.copy()
env['LEGENDARY_CONFIG_PATH'] = '/Users/YOUR_USER/Library/Application Support/heroic/legendaryConfig/legendary'
meta = subprocess.run([
    LEGENDARY, 'launch', '0c6db5941ac8420aabbd8510e94e7d9a', '--json',
    '--crossover-app', '/Users/YOUR_USER/Applications/CrossOver 25.1.1.app',
    '--crossover-bottle', 'SpiderMan2-VR-Test',
    '--override-exe', '../../VR-Tests/SpiderMan2-REALVR/Game/Spider-Man2.exe',
], env=env, capture_output=True, text=True)
if meta.returncode:
    raise SystemExit('Epic launch authorization failed. Open Heroic and refresh the Epic login.')
j = json.loads(meta.stdout)
assert GAME.is_file()
assert Path(j['working_directory']).resolve() == GAME.parent
env.update(j.get('environment', {}))
env.update(CX_BOTTLE='SpiderMan2-VR-Test', CX_DEBUGMSG='-all', WINEDEBUG='-all',
           WINEMSYNC='0', ROSETTA_ADVERTISE_AVX='1',
           WINEDLLOVERRIDES='dxgi=n,b;openxr_loader=n,b;openvr_api=n,b')
cmd = [str(WINE), '--bottle', 'SpiderMan2-VR-Test', '--no-update', str(GAME)]
if managed:
    env.pop('CX_BOTTLE', None)
    env['WINEPREFIX'] = str(BOTTLE)
    env['WINEDLLOVERRIDES'] = 'dxgi=n,b;d3d11=n,b;d3d12=n,b;openxr_loader=n,b'
    cmd = [str(WINE), str(GAME)]
if os.environ.get('SM2_VR_NATIVE_SHARING') == '1':
    assert not managed
    import uuid
    cxroot = str(WINE.parent.parent)
    env.update(WINELOADERNOEXEC='1', CX_GRAPHICS_BACKEND='d3dmetal', CX_ROOT=cxroot,
               WINEPREFIX=str(BOTTLE), WINESERVER=str(WINE.parent / 'wineserver'),
               WINEDLLPATH=cxroot + '/lib/wine', SM2_NATIVE_RUN='game-' + uuid.uuid4().hex,
               DMN_LOG='warn', SM2_NO_KEYED_MUTEX='1')
    cmd = [str(ROOT / 'Debug' / 'wine-debug-loader'), str(GAME)]
if env.get('SM2_QUEST_GAMEPAD') == '1':
    env['WINEDLLOVERRIDES'] += ';xinput1_4=n,b'
cmd += j.get('game_parameters', []) + j.get('egl_parameters', []) + j.get('user_parameters', [])
if env.get('SM2_VR_SKIP_LAUNCHER') == '1':
    cmd.append('-nolauncher')
print('Launching isolated Spider-Man 2 VR test with ' + ('MoltenVR Wine 11/D3DMetal 3.' if managed else 'CrossOver 25.1.1.'), flush=True)
with (ROOT / 'Records' / 'vr-launch.log').open('w') as log:
    p = subprocess.Popen(cmd, cwd=GAME.parent, env=env, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True, errors='replace')
    (ROOT / 'Records' / 'launch-pid.txt').write_text(str(p.pid))
    for line in p.stdout:
        line = re.sub(r'(?i)(AUTH_PASSWORD[=\s]+)\S+', r'\1[redacted]', line)
        log.write(line)
        log.flush()
    code = p.wait()
print(f'VR test process exited: {code}', flush=True)
modlog = ROOT / 'Game' / 'RealVR64.log'
if modlog.exists():
    contents = modlog.read_text(errors='replace')
    modlog.write_text(re.sub(r'(?i)(AUTH_PASSWORD[=\s]+)\S+', r'\1[redacted]', contents))
