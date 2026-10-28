# Bloons — C++ port

The original Bloons, decompiled with JPEXS Flash Decompiler, with a native C++17
implementation. The original ActionScript and exported assets remain unchanged
as the reference. No replacement levels or new game modes have been added.

Build on Windows with Visual Studio 2022 C++ tools, a Windows SDK, and CMake:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\bloons.exe
```

The executable finds the original asset directories in the source checkout, or
beside itself. To create a standalone directory with those assets:

```powershell
cmake --install build --config Release --prefix out
```

Use the mouse to aim; hold the left button to select power and release to throw.
Use the arrow keys while the Pacman power-up is active. The level selector
retains the original Unlimited Darts Mode. Progress and personal bests are saved
in `%LOCALAPPDATA%\Bloons\profile.txt`. Flash SharedObject saves are not imported.

`src/game.hpp` and `src/game.cpp` contain the platform-independent simulation.
`src/levels.cpp` contains all 50 original levels, including their titles, targets,
dart limits, and 15,600 grid cells. Regenerate it with
`python tools/extract_levels.py`; Python is not required to build or run the game.
`src/windows.cpp` supplies rendering, input, time, audio, and persistence using
Windows APIs. A future host can reuse `bloons_core` and supply those services
without changing the level data or simulation. Other systems can already build
and test the core with CMake; they do not yet have a graphical host.

The supplied `Bloons_decrypted.swf` has now been inspected directly. Its stage
is 640×480 and its frame rate is **50 FPS**. `src/swf_data.hpp` contains generated
placements, sprite bounds, stopped balloon poses, and sound linkage. The compact
audit record is `reference/swf_metadata.json`, including the SWF's SHA-256.
To regenerate both from the same reference file:

```powershell
python tools/extract_swf.py "C:\path\to\Bloons_decrypted.swf"
```

The extractor reads binary metadata and never executes ActionScript. The SWF
and Python are not needed to run the packaged game. When Python is available,
CMake also registers the metadata parser's regression tests.

This is a playable native port, **not a verified exact reproduction**. Known
differences and source limitations are recorded in [PORTING.md](PORTING.md).
The build and tests do not establish equivalence with the original Flash runtime.

The existing repository license and original asset attribution still apply.

The `codex/rockbox-ipod` branch adds an experimental iPod Video/Classic Rockbox
adapter, bounded-memory core build, asset packer and source installer. See
[the Rockbox port guide](ports/rockbox/README.md) for controls, build prerequisites,
verified host checks, and the remaining ARM/hardware work. No `.rock` binary has
been built or installed on an iPod yet.

GitHub Actions runs Windows and Linux builds/tests on pushes and pull requests,
packs the original Rockbox assets, and uploads Windows binaries, iPod previews,
and test reports. The separate **Experimental Rockbox ARM build** workflow can
be dispatched manually for Video and Classic plugin builds. See the
[Rockbox guide](ports/rockbox/README.md#github-actions) for artifact contents and
the distinction between host checks and target/hardware verification.
