Spider-Man 2 on Apple Silicon: experimental compatibility research
=================================================================

Status, 2026-11-05
The user confirmed visible gameplay in a Quest 3S using the isolated VR
setup. Stereo mode was slow: a screenshot showed 17.4 FPS / 353 ms latency.
Mono is the next performance experiment, not a verified performance fix.
Long-session stability, portability, first-person camera support and Quest
controller mapping remain unverified. Use an Xbox controller paired to macOS.
This is a source/research kit, NOT a turnkey mod installer.

Double-click launchers (configured on this Mac)
----------------------------------------------
flat.command: original Epic install, CrossOver bottle SpiderMan2-CX25.
vr.command: isolated VR copy, preserving the selected render mode.
mono.command: isolated VR copy, backs up RealVR.ini and selects Mono.
Exit the game normally before switching. Dismiss its graphics warning when
shown. Open MoltenVR on the Quest for VR. Mono loses stereo depth.

Quick commands (from this folder):
  make flat       # Original non-VR game
  make vr         # VR with saved settings
  make mono       # VR without stereo depth
  make stereo     # Restore previous 1/3-rate stereo setting
  make check      # Validate launch paths without starting a game

Equivalent Bash launcher: ./play flat, ./play vr, ./play mono,
./play stereo, or ./play check. Run ./play or make for help.
The play script also works by absolute path from another directory.

local.json contains this Mac's paths and is ignored by Git. Other machines
must copy local.example.json to local.json and adapt it. The VR launcher
uses the existing external isolated installation and diagnostic binaries;
copying this repository alone does not install that environment. Saves remain
in their existing separate bottles. These scripts do not synchronize saves.
Flat launch path validation passed; a full new flat launch has not been tested
in this session, to avoid opening a second game alongside the VR test.

Tested components
-----------------
CrossOver 25.1.1; D3DMetal; MSync off; AVX advertised via Rosetta.
Epic Spider-Man 2 v2.810.0.0; R.E.A.L. VR 2607.31.1; MoltenVR 1.2.0.
Mac mini Apple Silicon; Quest 3S. No game, R.E.A.L. VR, CrossOver or MoltenVR
binaries are included. Obtain each component separately and legally.

What the compatibility changes do
--------------------------------
1. Supply D3DMetal shared textures/fences using d3dmetal-native's native hooks
   without replacing Wine's GFXT host interface.
2. Provide an IDXGIResource1 fallback and shared backing for VR swapchains.
3. Warm a native command queue so pre-existing game queues receive Signal hooks.
4. Poll actual GPU completion for signal-back under Wine instead of waiting
   on native event objects that Wine cannot signal.
5. Leave application SetEventOnCompletion handles with the original Wine
   implementation. Treating Windows HANDLE values as native event pointers
   caused access violations. Shared-slot-only Windows event notification still
   needs further validation; do not claim universal shared-fence correctness.
6. Disable the added keyed-mutex interface for this test so the mod uses fences.
7. Version-guarded in-memory geometry/HUD fallback for zero dimensions and a
   narrowly guarded writable-native-vtable compatibility shim.

Research source layout
----------------------
patches/d3dmetal-native.patch targets upstream commit
  af1ee1710711fadff41dead87960d0f1336c1047
  https://github.com/utmapp/d3dmetal-native
After applying, copy compat/wine_probe_bridge.cpp into its src directory.
Follow upstream's x86_64 Meson build instructions; retain its subprojects.
The patch adds that bridge to meson.build. Do NOT call dmn_init() in Wine.

compat/ contains the Wine diagnostic loader, OpenXR tracing generator,
version-specific mod instrumentation, and the OpenVR forwarding shim.
Paths are deliberately redacted to /Users/YOUR_USER. Replace them consistently
in a private working copy before compiling. They are not portable defaults.
The generator additionally requires Khronos openxr.h and openxr_platform.h,
which are not vendored here. OpenXR loader/real runtime configuration must
point to the appropriate locally installed runtime. Do not deploy while a
game has these DLLs loaded. The loader derives from Wine 10.0 loader/main.c.

Example build commands (from a prepared compat working directory):
  x86_64-w64-mingw32-gcc -shared -O0 -g -static -o trace_runtime.dll trace_runtime.c
  x86_64-w64-mingw32-gcc -shared -O2 -g -static -o openvr_compat.dll vtable_compat.c writable_probe.S openvr_compat.def
The trace generator must run before compiling trace_runtime.c.
The diagnostic Wine loader needs the custom address layout documented in
BUILD-NOTES.txt; a normal default clang invocation is insufficient.

Probes and evidence
-------------------
probes/ holds small source tests, not game code. Compile each with MinGW g++:
  x86_64-w64-mingw32-g++ -static -O0 NAME.cpp -o NAME.exe -ld3d11 -ld3d12 -ldxgi
Run using the diagnostic loader, a unique SM2_NATIVE_RUN, the isolated prefix,
and CrossOver 25's CX_ROOT / WINESERVER / WINEDLLPATH. Do not reuse bridge
pointer files across processes. WINELOADERNOEXEC=1 and
CX_GRAPHICS_BACKEND=d3dmetal are required.

Observed results:
- D3D12 producer -> D3D11 consumer fence: passed.
- D3D11 producer -> D3D12 imported signal-back: failed before polling fix,
  passed after (producer=1, consumer=1).
- Windows event SetEventOnCompletion + WaitForSingleObject: passed after fix.
- Queue created before hook installation: passed after warm-queue fix.
- D3D12 texture write -> D3D11 original texture read: passed.
- D3D11 write -> D3D12 read: unresolved; not all sharing directions are proven.
- In-game fences progressed to 229 before the separate event-handle crash.
- After event fix: user confirmed live headset gameplay; low FPS remains.
Raw logs, crash dumps, account identifiers, saves and exchange tokens are
excluded. Never commit the external Game, Records, or bottle directories.

Sharing and licensing
---------------------
This is a local Git repository; nothing has been published to GitHub.
Review and adapt the research before proposing upstream patches. Original
project scripts/probes are MIT licensed. d3dmetal-native changes retain its
MIT license; Wine-derived loader and main.h retain LGPL-2.1-or-later and
copyright notices (licenses/Wine-COPYING.LIB). No proprietary DLLs are
redistributed. The launcher template references external executables only.
