# Porting notes and fidelity limits

The preserved `scripts/__Packages` classes are the behavioral reference:
PopGame, Dart, Bloon, Tack, SpikeyBall, Pacman, and Level. Rendering and platform
services are confined to `src/windows.cpp`; the game core remains C++17 without
Windows dependencies. No new levels, power-ups, scoring systems, or modes have
been introduced.

## SWF reference

The user supplied `Bloons_decrypted.swf`. Its SHA-256 is:

```text
f7f781d7c39770ee65d44090f7d71d4c13f22e24a74abd35da26a3ea85017b73
```

`tools/inspect_swf.py` reads metadata without executing embedded ActionScript.
`tools/extract_swf.py` generates `src/swf_data.hpp` and the compact audit report
`reference/swf_metadata.json`. Generated data is checked in so ordinary builds
do not need Python or access to the user's Downloads directory. The supplied SWF
is not modified or copied into the repository.

The parser follows Adobe's SWF RECT, MATRIX, CXFORM, tag, and display-list formats;
see the [SWF File Format Specification](https://www.flashrealtime.com/content/dam/Adobe/en/devnet/swf/pdf/swf_file_format_spec_v10.pdf).
It handles metadata required by this file, including zlib compression, sprites,
button records, morph bounds, frame labels, placements, exports, and sound IDs.
It is not an ActionScript interpreter or a general-purpose SWF renderer.

## Values recovered and used

- SWF version 8; 640×480 stage; **50 FPS (20 ms per simulation step)**; 59 root
  frames. The earlier 30 FPS estimate was incorrect and has been removed.
- Root labels `load=3`, `intro=6`, `main=16`, `howtoplay=42`,
  `levelselect=48`, and `play=55` are recorded for reference.
- Play-area registration `(25, 0)`, correcting the earlier missing horizontal
  offset. AS2's CreateBloon arithmetic uses this registration, not its scale.
- The primary and extra darts' original nonuniform scales, normal tip offset,
  and boomerang collision clip. Tack and spikey collision clips also use the
  recovered child matrices and bounding boxes.
- Monkey arm pivot, arm tip registration, throw poses, original aim-arrow
  artwork and scaling, HUD wrapper/inner placement, text field sizes/alignment,
  level thumbnail positions, and loss text placement.
- 171 sprite union bounds and **960 stopped balloon poses** (80 per balloon
  variant). Idle balloons use their actual selected outer matrix over the clean,
  stopped inner clip instead of resizing a flattened PNG to a generic cell.
- 1,955 cloud placement frames, with the sky gradient from exported shape 183.
- Actual per-variant pop sounds, wood/rubber sounds, delayed ice sprite sound,
  monkey throw sound, and boomerang sound. The old pop/throw mapping was wrong.
  Sound 311 is a short menu-hover effect, not music. There is **no exported
  `pacsong` sound** in this SWF, so the original unsuccessful attachment is silent
  in the port rather than playing an unrelated file.
- Loss uses the original Try Again text and the existing HUD reset control;
  the previously reconstructed win-style loss panel has been removed.

The 50 original layouts, titles, targets, dart allowances, and 15,600 grid cells
are still generated directly from the unchanged ActionScript export. The core
retains the source frame-based physics constants, power-up behavior, delayed
end checks, score rounding, unlocks, and unlimited-darts rules.

## Remaining differences

The SWF resolves missing metadata; it does not by itself establish playback
equivalence with Flash Player. These areas remain unfinished or unverified:

- **Rendering and timelines:** the native host uses original raster exports and
  composes their recovered transforms. It does not render SWF vector/morph tags,
  filters, masks, or arbitrary nested timelines directly. Some PNGs have export
  artifacts. Win-panel compositing and transitions, menu hover animation,
  full eye animation, freeze draw-on timing, and rubber side animation still
  differ. The monkey's flattened base can retain its resting arm beneath the
  separately animated arm. The full introductory timeline is not replayed.
- **Collision:** native rectangles now use recovered geometry and transformed
  projectile hit clips, but Pacman's `hitTest(..., true)` pixel/shape tests are
  still approximated with bounds. Rubber/frozen side response and boomerang
  catch tests still need closer reconstruction and comparison in motion.
  Sprite bounds may differ around masks, filters, morphs, and stroke rounding.
- **Audio:** linked file identities and several timeline triggers are recovered.
  Sound overlap, stop flags, precise event ordering, and Flash mixer behavior
  are not reproduced in full. Windows MCI codecs decode the original MP3s;
  actual device playback has not been verified in the automated checks.
- **Randomness:** the portable generator differs from Flash's unspecified
  random generator. Initial colors, selected stopped poses, and spikey spin
  need not match a particular Flash playthrough.
- **AS2 quirks:** undefined variables such as `dart.stuck` and the
  `userPforile.flush()` typo do not directly translate to C++. The port applies
  intended projectile state and writes valid native progress. Degenerate aim
  vectors are handled to avoid division by zero.
- **Online services:** Mochi leaderboard/score submission and remote user-created
  level loading have no native integration. Original website links remain.
  No replacement leaderboard or level editor has been added.
- **Persistence/platforms:** progress uses `%LOCALAPPDATA%\Bloons\profile.txt`;
  Flash SharedObject files are not imported. Windows has a built graphical host;
  `ports/rockbox` adds an experimental iPod host whose native build/hardware
  behavior is still unverified. Its format and platform-specific limitations
  are described in that directory's README.

## Verification

`core_tests` covers all 50 level populations and idle states, recovered cadence
and placement, aiming and launch completion, power limits, frozen/duplicate pops,
each special-balloon effect, sound IDs/timing, stable artwork after bomb type
changes, score/unlock handling, and the precise greater-than-1000-ms end delay.

`swf_parser` (when Python is installed) tests signed matrix values, color-transform
alignment, compressed/uncompressed headers, frame snapshots, move/remove depth
semantics, labels/exports/sounds, and truncated-file rejection using synthetic
fixtures. Extraction from the supplied SWF is reproducible.

`render_and_input` renders the menu, selector, instructions, levels 1/25/50,
aiming, dart flight, frozen balloons, and win/loss states to `build/render`.
It exercises menu/selector clicks and launch input without opening a window,
playing audio, or writing a profile. Representative render output is visually
inspected. These checks are native regressions, not Flash differential tests.

The graphics/audio host is built with MSVC on Windows. Interactive end-to-end
completion of every level, direct comparison against a running original SWF,
and graphical hosts for other platforms have not been verified.
