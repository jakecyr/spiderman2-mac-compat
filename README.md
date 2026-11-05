# Spider-Man 2 on Mac — setup and play

Play the Epic Games PC edition on an Apple Silicon Mac, with an optional
experimental Meta Quest VR setup. This repository contains convenient launch
commands and the compatibility research used on our test Mac.

Already set up? [Jump to the launch commands](#play-next-time).
Setting up another Mac? [Start at Step 1](#step-1--get-the-game-and-install-the-mac-apps).

## What works so far

The original flat-screen game launched and saved successfully. With the custom
VR fixes, the user also confirmed visible gameplay on a Quest 3S. VR is still
experimental: stereo gameplay showed about 17 FPS and 353 ms latency in one
sample. Mono is being tested for responsiveness; improvement is not verified.

The launchers are ready for the existing installation on this Mac. A fresh
Mac still needs manual compatibility work for VR; this repository is not an
automatic VR installer. See Step 5 before buying anything specifically for VR.

## What you need

- An Apple Silicon Mac. Our test machine is a Mac mini M5 Pro with 24 GB RAM;
  this is a tested configuration, not a proven minimum requirement.
- Space for the game and its separate VR copy. Our installed game occupied
  about 138 GiB. APFS clones can initially share storage, but updates and
  modifications need more space; allow substantial headroom.
- The PC edition of Marvel's Spider-Man 2 from Epic Games.
- Heroic Games Launcher and CodeWeavers CrossOver.
- An Xbox-compatible controller, or keyboard and mouse.
- For VR: a Quest headset (tested: Quest 3S), USB-C data cable, MoltenVR,
  R.E.A.L. VR, and the custom compatibility build described in Step 5.

### Tested software

| Component | Version |
| --- | --- |
| CrossOver | 25.1.1, with D3DMetal |
| Spider-Man 2 | 2.810.0.0 (Epic) |
| R.E.A.L. VR | 2607.31.1 |
| MoltenVR | 1.2.0 |

Newer versions are not automatically interchangeable. The VR instrumentation
uses version-specific checks and offsets. Do not update the working copy
without keeping a recoverable backup.

## Step 1 — Get the game and install the Mac apps

1. Buy or use your existing Epic PC copy of Marvel's Spider-Man 2:
   [Spider-Man 2 on Epic Games](https://store.epicgames.com/p/marvels-spider-man-2)
   A PlayStation purchase is not the Epic PC license used by these scripts.
   Steam ownership is not supported by the current Epic-specific launchers.
2. Download Heroic for macOS and move it to Applications:
   [Download Heroic](https://heroicgameslauncher.com/)
   Heroic signs in to Epic and downloads the Windows game on the Mac.
3. Install CodeWeavers CrossOver:
   [Download CrossOver](https://www.codeweavers.com/crossover/download-links)
   This setup used version 25.1.1. Use an official CodeWeavers download/account
   source for that version if available. CrossOver 26.3 did not work in our
   initial tests; the latest download is not a verified substitute.
4. Allow macOS to install Rosetta if prompted when opening Intel software.
5. Open Heroic, sign in to Epic, complete any verification, and download the
   game. Our install folder is `~/Games/Heroic/marvelsspiderman2`.

Heroic is the Epic launcher used by this setup. You do not need to install
the separate Epic Games Launcher just to follow this workflow.

## Step 2 — Get the normal game working first

A CrossOver "bottle" is a separate Windows environment. Keep one for normal
play and a different one for VR.

1. Create a 64-bit Windows bottle in CrossOver called `SpiderMan2-CX25`.
2. Enable D3DMetal for that bottle and disable MSync.
3. In Heroic's settings for this game, select CrossOver 25.1.1 and the
   `SpiderMan2-CX25` bottle. Enable "Advertise AVX for Rosetta". The launch
   scripts also set `ROSETTA_ADVERTISE_AVX=1` and `WINEMSYNC=0`.
4. Install the two bundled PlayStation PC SDK packages INTO THIS SAME BOTTLE:

   - `PsPcSdkRuntimeManager.msi`
   - `PsPcSdkRuntimeInstaller.msi`

   They are in the downloaded game's folder. Use CrossOver's installer or
   Run Command for the selected bottle. Do not download replacement DLLs
   from unrelated sites. Both packages were installed in our successful setup.
5. Launch from Heroic. If the graphics warning seen in our tests appears,
   dismiss it, then choose Play in the game's startup launcher if shown.
6. Confirm that you can reach gameplay. Make a manual save, quit through the
   game menu, relaunch, and check that Continue/Load Game finds the save.

If the game only shows black, first check the exact CrossOver version,
bottle selection, D3DMetal, AVX option, and installed PlayStation SDK.
Do not add VR until the normal game works.

## Step 3 — Pair the controller

1. Put the Xbox controller into Bluetooth pairing mode.
2. On the Mac, open System Settings > Bluetooth and connect the controller.
3. Test it in the normal game. Pair it with macOS, not inside Heroic.

For this setup, keep using the Xbox controller in VR. Quest motion-controller
mapping and hand-controlled web swinging are not implemented here. The tested
VR view retains the third-person camera; a first-person mode is not verified.

## Step 4 — Set up the optional Quest connection

Skip this step if you only want flat-screen play.

1. Download MoltenVR from its official releases and put MoltenVR.app in
   Applications: [MoltenVR releases](https://github.com/MoltenVR/MoltenVR-Preview/releases)
2. Enable Developer Mode for your Quest using Meta's current setup flow.
3. Turn on the headset and connect it to the Mac with a USB-C DATA cable.
   Inside the headset, accept the USB-debugging prompt for your Mac.
4. In MoltenVR on the Mac, choose Settings > Headsets > Install on headset.
5. Open MoltenVR on the Quest (under Unknown Sources if listed there).
   Complete pairing using the PIN shown on the Mac, if requested.
6. Confirm that the headset connects to the Mac before starting the game.

Official illustrated setup:

- [MoltenVR installation guide](https://moltenvr.com/docs/getting-started/installation)
- [Quest headset setup guide](https://moltenvr.com/docs/setup/headset-app)

This project uses MoltenVR's OpenXR runtime, not SteamVR. Installing SteamVR
is not a required step. MoltenVR's default managed Wine is also not the
CrossOver 25.1.1 environment used by our custom launch path.

## Step 5 — Prepare the isolated VR game (manual developer setup)

> **Manual setup required:** `make vr` and `make mono` launch an already prepared installation. They do not install these compatibility components.

The following summarizes the working installation. On a fresh Mac, a person
comfortable building compatibility code must adapt it before the VR launcher
will work. Installing the game, MoltenVR and the mod alone was insufficient.

1. Back up the normal game's saves and keep its installation unchanged.
2. Create a separate copy of the game at:

   ```text
   ~/Games/VR-Tests/SpiderMan2-REALVR/Game
   ```

   Duplicate the working CrossOver bottle as `SpiderMan2-VR-Test`. Its Documents
   folder must be a real isolated directory, not a link to the normal saves.
3. Obtain R.E.A.L. VR from its author and follow the instructions for the
   supported Spider-Man 2 build, applying it ONLY to the VR copy:
   [R.E.A.L. VR by Luke Ross](https://www.patreon.com/realvr)
   The mod is not included here. Availability and supported game versions
   may change; do not assume a different release accepts our patches.
4. Build and install the custom sharing bridge, diagnostic Wine loader,
   OpenXR shim and OpenVR forwarding shim. The source, upstream revision,
   build commands and known limitations are in RESEARCH.txt and BUILD-NOTES.txt.
   Source placeholders such as `/Users/YOUR_USER` must be adapted first.
5. Configure the isolated bottle's OpenXR runtime to use the tracing shim
   forwarding to the locally installed MoltenVR runtime. Use the OpenXR
   renderer in the R.E.A.L. overlay. The external installation must include
   Game/, Debug/, Records/, and `launch-vr-test.py` as described in the research.
6. Configure MoltenVR's game entry to use this isolated game and custom
   CrossOver launch path. On this Mac that bridge already exists. Simply
   adding the EXE to a new default MoltenVR bottle does not reproduce it.

The compatibility fixes cover shared graphics resources, GPU completion,
Windows event handles and zero-size VR textures. They have enabled gameplay
on one setup; they are not a general-purpose supported driver or installer.

## Step 6 — Configure this repository's launchers

On this Mac, `local.json` is already configured. Do not overwrite it.
On another Mac, from this repository's folder:

```bash
cp local.example.json local.json
```

Open `local.json` in a text editor and adjust:

| Setting | Value |
| --- | --- |
| `crossover_app` | Path to the CrossOver 25.1.1 app. |
| `flat_game` | Path to the original game's `Spider-Man2.exe`. |
| `flat_bottle` | Name of the original CrossOver bottle. |
| `legendary` | Heroic's bundled Legendary executable. |
| `legendary_config` | Heroic's Epic login/configuration directory. |
| `vr_root` | Root of the fully prepared isolated VR installation. |

Keep Heroic signed in. The launchers obtain fresh Epic authorization; do not
paste passwords or exchange tokens into `local.json`. This file is ignored by
Git. The VR launcher also relies on the external `launch-vr-test.py` having
matching paths; editing `local.json` alone does not configure that helper.

Check the setup without launching:

```bash
make check
```

This checks required file paths, not VR readiness or account validity. If
you have only installed the normal game, check that path independently:

```bash
./play flat --check
```

The commands use `/usr/bin/python3`. If macOS has not installed the developer
command-line tools needed for Python/make, run `xcode-select --install` and
complete Apple's installer. No Homebrew installation is needed just to run
these launchers; building the compatibility code needs additional tools.

## Play next time

Open Terminal and enter:

```bash
cd ~/Desktop/code/spiderman2-mac-compat
```

Then choose **one** command:

```bash
make flat       # Normal game on the Mac display
make vr         # VR using the last saved render setting
make mono       # VR without stereo depth; responsiveness experiment
make stereo     # VR with the previous 1/3-rate stereo setting
```

No Make? Use `./play flat`, `./play vr`, `./play mono` or `./play stereo` instead.
For double-click launch, use `flat.command`, `vr.command` or `mono.command` in
Finder. Run make or ./play for help.

For VR, first open MoltenVR on the Mac and Quest and keep the headset awake.
After launching, dismiss the game's graphics warning if shown. Select the
Spider-Man 2 session in the headset if it does not appear automatically.
Resume your save and check that head movement changes the view.

Stop VR in the overlay stops rendering; it does not quit the game. To finish,
make a manual save, wait for saving to finish, and quit via the game menu.
Exit one game before starting another mode. The launchers do not terminate
an existing game or restart it for you.

## Saves and backups

Normal and VR play use separate save locations. These scripts do not copy or
synchronize saves between them. Back up both before changing the setup.
On the original setup, normal saves were under:

```text
~/Documents/Marvel's Spider-Man 2/<account-folder>/
```

VR saves use the isolated bottle's Documents directory. Verify actual paths
on your machine; CrossOver bottles can redirect Documents differently.

`make mono` and `make stereo` back up `RealVR.ini` into this repo's ignored `state/`
folder before changing the setting. They do not back up your saved games.

## Troubleshooting

### "Missing required file"

Check `local.json`. For VR, finish Step 5; the source repo alone has no DLLs.

### "Epic authorization failed"

Open Heroic, refresh your Epic sign-in, and retry. Never share login tokens.

### "A launcher is already active" / "test is already running"

Exit the current game and allow its launcher to finish. If a crash left a
helper running, identify that specific process before stopping it. Do not
kill every Wine process or delete lock files as a general fix.

### "PlayStation PC SDK Runtime ... missing"

Install both bundled SDK packages in the bottle that launches this copy.

### Headset has sound but says "Waiting for frames"

Audio alone does not prove VR is rendering. Check the headset connection,
correct MoltenVR session, and OpenXR selection in the R.E.A.L. overlay.
On a fresh setup, confirm all custom compatibility components are installed.

### Head turning feels delayed

Our stereo test had very low FPS. Try `make mono` after exiting the game;
it removes stereo depth and may reduce work, but is not a guaranteed fix.
Keep a USB data connection for the initial test. Record FPS and latency
before/after. Lowering sensitivity does not fix rendering latency.

### Game/mod update breaks startup

Restore the backed-up matching test setup. The compatibility code is
version-specific; do not apply offsets blindly to a newer mod.

## For developers and sharing

| File or folder | Contents |
| --- | --- |
| [RESEARCH.txt](RESEARCH.txt) | Compatibility changes, test results and source layout. |
| [BUILD-NOTES.txt](BUILD-NOTES.txt) | Diagnostic loader build flags and render-mode values. |
| [compat/](compat/) | Instrumentation and launcher template source. |
| [patches/](patches/) | Patch for the pinned d3dmetal-native revision. |
| [probes/](probes/) | Small graphics-sharing regression tests. |
| [licenses/](licenses/) | Third-party license texts. |

No game, mod, CrossOver or MoltenVR binaries, saves, tokens, crash dumps or
private logs are included. Do not commit those files. The project scripts
are MIT licensed; Wine-derived files retain LGPL-2.1-or-later and their
copyright notices. See LICENSE and licenses/ for details.

Source repository: [jakecyr/spiderman2-mac-compat](https://github.com/jakecyr/spiderman2-mac-compat).
Setup and source links reviewed on 2026-11-05.
