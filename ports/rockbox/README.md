# Bloons for Rockbox: iPod Video / Classic

This branch contains a **source port in progress**, not a tested iPod binary.
The 50 original levels and C++ simulation are shared with the desktop port.
The Rockbox adapter targets the 320×240, RGB565, click-wheel iPod Video and
Classic configurations. Build for the actual model; their `.rock` binaries are
not interchangeable. A photograph alone cannot establish the exact generation.

## What is implemented

- Native `plugin_start`, Rockbox LCD/button/menu/file APIs, and USB exit handling.
- Wheel-position aiming; Select hold/release maps to the original charge/throw.
- Original 50 Hz simulation; rendering requests are limited to 25 Hz to reduce
  LCD traffic. Late steps are retained, so slow hardware may run slower in wall
  time rather than changing physics constants. Real-time performance is untested.
- A C renderer that scales the original scene to 320×240 using original PNG
  sprites packed offline as RGBA. No Flash player, Windows APIs, image decoder,
  or new artwork is required on the player.
- No `std::vector`, `std::deque`, `std::string`, exceptions, RTTI, or heap
  allocation in the embedded core. Desktop builds retain their existing dynamic
  containers. The generated geometry and level arrays use a small aggregate type.
- A bounded arena for the game plus assets and framebuffer in Rockbox's audio
  buffer. Starting the plugin therefore interrupts music playback.
- Versioned, validated progression/best-score saves in `bloons.save`.
- Source installer and multi-file Rockbox make recipe.

## Controls

| Control | Action |
|---|---|
| Touch/rotate wheel | Aim around the monkey |
| Hold Select, then release | Charge, then throw |
| Menu | Pause / level selector / unlimited darts / retry / quit |
| Menu, Play, Previous, Next while Pacman is active | Up, down, left, right |
| Select + Menu while Pacman is active | Open the pause menu |
| Select after a result | Retry a failed level or advance after a win |

The wheel supplies a direction instead of Flash mouse coordinates. Text and
navigation use Rockbox's system font/menus to remain legible on the smaller LCD.
Pausing cancels a partially charged throw and consumes the menu's Select press.
Unlimited darts is the original mode, not a newly added feature.

## Build and install source

Use a Linux Rockbox development environment or a suitable existing simulator
environment. This Windows workspace has MSVC but no WSL installation, Rockbox
cross compiler, or ARM linker; no actual `.rock` has been built here.

The implementation was checked against upstream Rockbox source at:

```text
8755e35daa20b2daf6788c01e75a947fd5f60e72
```

1. Obtain a Rockbox source checkout and its normal development prerequisites.
   The [plugin API](https://github.com/Rockbox/rockbox/blob/8755e35daa20b2daf6788c01e75a947fd5f60e72/apps/plugin.h)
   and [multi-file plugin rules](https://github.com/Rockbox/rockbox/blob/8755e35daa20b2daf6788c01e75a947fd5f60e72/apps/plugins/plugins.make)
   are the integration reference. Do not install or replace the player's
   bootloader just to build this plugin.
2. Supply a compatible **C++17 ARM compiler and newlib libm**. Rockbox's normal
   [`rockboxdev.sh`](https://github.com/Rockbox/rockbox/blob/8755e35daa20b2daf6788c01e75a947fd5f60e72/tools/rockboxdev.sh)
   configures native GCC with `--enable-languages=c`, so a stock installation
   does not supply the needed C++ frontend. A matching C/C++ toolchain can be
   built, or an ABI/multilib-compatible ARM toolchain can be supplied explicitly.
   The recipe uses `BLOONS_CXX` and `BLOONS_LIBM`; it does **not** link libstdc++.
   CPU architecture, ARM/Thumb mode, soft-float ABI and libm multilib must match
   the configured Rockbox target. These combinations still need an actual link.
3. Install this adapter into the chosen Rockbox source checkout:

   ```sh
   python3 ports/rockbox/install.py --rockbox /path/to/rockbox
   ```

   The installer copies the shared core and adapter into
   `apps/plugins/bloons`, registers it under guarded `SUBDIRS`, and adds its games
   category. Re-running updates only its marked plugin directory and exact
   registration lines. It refuses an unrelated existing Bloons directory.
4. Configure Rockbox for the exact model using `tools/configure` from a separate
   build directory. For example, choose **iPod Video** or **iPod 6G / Classic**
   interactively; do not select based on the photo alone. Then build:

   ```sh
   make -j4 BLOONS_CXX=/path/to/compatible-arm-g++ \
       BLOONS_LIBM=/path/to/matching-multilib/libm.a
   ```

   To restrict the plugin selection while diagnosing the new target, Rockbox
   supports `SELECTED_PLUGINS_SRC` / `SELECTED_PLUGINS_SUBDIRS` overrides. The
   expected plugin output is `apps/plugins/bloons/bloons.rock` in the Rockbox
   build directory. Check the link map against the target plugin buffer limit
   (Video's source config specifies 512 KiB; Classic's specifies 2 MiB).
5. Pack the original assets once on the development machine. Install Pillow in
   that build environment if it is not already present:

   ```sh
   python3 ports/rockbox/pack_assets.py --output build/rockbox/bloons.dat
   ```

   The current pack has 27 original sprites/background entries and is 489,868
   bytes. The packer records its dimensions in a JSON sidecar. Player runtime
   does not require Pillow or Python.
6. After a successful target build, copy **only** `bloons.rock` to
   `.rockbox/rocks/games/` and `bloons.dat` to `.rockbox/rocks/data/games/` on the
   player. The plugin creates `bloons.save` alongside the asset file. No hardware
   or mounted iPod has been modified by this task.

## Local checks

The existing CMake project also builds the embedded core and adapter on the
development machine:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build/Release/rockbox_adapter_tests build/rockbox/bloons.dat build/rockbox/preview.ppm
```

Use `build/rockbox_adapter_tests` for single-configuration CMake generators.
The desktop and bounded-storage variants run the same level/power-up regression
suite. Adapter tests cover all 50 levels, hold/release, save validation, asset
directory validation, framebuffer guards, and an optional rendered preview.
The roughly 257 KB arena figure was measured on the host; verify actual ARM
layout, plugin size, stack depth, and performance from the target build.

## Remaining work before a hardware release

- Compile/link against real Rockbox headers with the selected ARM C++/libm
  toolchain, resolve any ABI/library issues, inspect plugin map and undefined
  symbols, and run the Rockbox simulator and actual player.
- Measure 50 Hz simulation and LCD performance on both Video and Classic.
  The renderer currently uses software double-precision transforms, while iPod
  CPUs do not provide desktop-class floating-point performance. Fixed-point
  optimization must be checked against the existing simulation, not silently
  substituted for its constants.
- Original MP3 sound mixing is not implemented on Rockbox. The adapter is
  silent and does not substitute beeps or change the original sound files.
- Menus/HUD use system text. The full original UI animations, sound, cloud
  animation, rubber animation, projectile animation, and pop effects are not
  yet reproduced by the small renderer. The asset pack uses clean first frames.
- Preserve and validate Flash-specific behavior already listed in the main
  [porting notes](../../PORTING.md), especially shape collision differences.

This is a concrete starting point for an iPod port, with host-tested shared
gameplay and an unverified native Rockbox host. It is not yet an install-and-play
release for the physical device.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes, pull requests, and manual dispatches.
Its Windows 2022 and Ubuntu 24.04 jobs install Python 3.11/Pillow, regenerate the
original asset pack **before** CMake configuration, build the desktop/embedded
targets, and run CTest. No Downloads directory, original SWF, or local runtime
bundle is required: generated SWF geometry is already committed.

Artifacts are retained for 14 days:

- `bloons-windows-x64`: executable plus its original asset directories.
- `rockbox-assets-<runner>`: `bloons.dat`, metadata and a 320×240 preview. This
  asset artifact is not an executable plugin.
- `checks-<runner>`: JUnit results, CTest logs and Windows render captures.

`.github/workflows/rockbox.yml` is a separate, manually dispatched experimental
ARM build. Once the workflow is available in GitHub's Actions interface, select
**Experimental Rockbox ARM build → Run workflow**, choosing this branch. GitHub
normally requires a dispatchable workflow to be present on the default branch
before exposing it in that interface; merely creating a local YAML file does
not execute a remote run.

The ARM jobs check out the pinned Rockbox source, install Ubuntu's bare-metal
ARM C/C++ compiler and newlib, install this plugin's source, and invoke
`ci_build.sh` separately for `ipodvideo` and `ipod6g`. A local tool-name shim
satisfies Rockbox configure's `arm-elf-eabi-*` naming while using the same
`arm-none-eabi-*` compiler for C and C++. Rockbox selects the target CPU flags;
the make recipe queries that compiler for the matching libm archive.

Successful jobs upload `experimental-bloons-<target>` with `rocks/games/bloons.rock`
and `rocks/data/games/bloons.dat`; these paths belong inside `.rockbox` on a
player using a compatible Rockbox build. Failures upload diagnostics instead of
an apparently valid binary. The workflow does not flash firmware, publish a
release, or upload any files to a player.

The hosted ARM integration is still experimental: workflow syntax and local
host tests can be validated here, but successful ARM linking and hardware
operation must be confirmed by the remote run and physical testing. Nothing in
the ordinary host CI job is presented as proof that an iPod binary works.
