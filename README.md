# BLAZE — native C reconstruction

A behavior-focused reconstruction of BLAZE 2.00. The game runs ordinary C:
no CPU/x87 interpreter, generated instruction execution or original executable
is used by the shipping target. Five stages, bosses, weapons, menus, SFX/MIDI,
pause, controls and score/unlock persistence are connected.

Windows x86/x64 are locally tested. Windows ARM64 is configured for native CI;
Linux x64/ARM64 and macOS ARM64 currently build core checks only. Their playable
SDL3 backend is in progress. A Windows SDL3 candidate now handles presentation,
keyboard/joystick input, PCM output and pacing while retaining Windows MIDI and the
F10 editor. A complete natural campaign and physical
controller acceptance remain open.

## Build and run

Visual Studio C/C++ tools, Windows SDK and CMake 3.24+:

```powershell
./source/scripts/build.ps1 -Architecture x64 -Tests
./blaze.exe
```

Use `-Architecture x86` or `ARM64` with the matching installed tools. Local builds
replace root `blaze.exe` beside the assets; binaries and private recovery material
are ignored. An original executable is replaced only after verifying its preserved
canonical copy. `-NoDeploy` builds/tests without replacing the root executable.
Rebuilds preserve `dat/Blz.ini`, `dat/BlzScr.ini` and `dat/BlzCtrl.ini`.

To build the SDL3 candidate, add `-Backend SDL3`. It fetches checksum-pinned SDL
3.4.18 and shares the same game/settings owners. Use `-NoDeploy` to keep root
`blaze.exe` on the stable Win32 backend. SDL3 is not yet a Unix runtime: portable
MIDI, the F10 editor and installed-user paths remain to be connected.

Arrow keys move/navigate, **Z** fires/confirms, **X** uses the ship ability/cancels,
**Esc** pauses, **F2** returns to title, **F9** exits and **F10** edits controls.
Keyboard and joystick bindings are saved in `dat/BlzCtrl.ini`; options and
scores/unlocks retain the original two INI filenames. First launch is windowed.
The original `Time=40` cadence is 25 simulation ticks per second.
Use `--mute` for a quiet session. CD playback remains deferred.

Assets resolve beside the executable, independently of the working directory.
The window title is `BLAZE`, with the original embedded icon. Source resources
are already extracted and verified: Python and the original EXE are unnecessary
for ordinary builds. For a self-contained runtime package, see
[Building](docs/BUILDING.md).

## Project and releases

`source/` is the authoritative shipping implementation. `docs/` contains the
[active plan](docs/plan.md), [checkpoint](docs/handoff.md),
[technical contracts](docs/architecture.md) and [SDL3/platform plan](docs/PORTABILITY.md).
The ignored `game-recomp/` retains the private Git history, original inputs,
extraction and executable differential tools. It is not a public build dependency.

CI checks six native architectures, distinguishing application builds from core
checks. Windows runtime ZIPs are configured now. The manually dispatched release
workflow resolves an existing tag to its commit and requires all six runtime
packages before publishing; release publication is blocked until SDL3 integration.
Linux tar.gz and an ad-hoc-signed macOS app/DMG are planned. See
[CI/release details](docs/BUILDING.md#ci-and-releases).

This is still under testing. Fixtures reach later bosses through controlled state;
they do not establish natural campaign victory, full scene parity or platform
hardware acceptance. No mid-mission/campaign resume is implemented.

Original game content retains its separate ownership. No project-wide source
license has been selected. See [Third-party notices](THIRD_PARTY_NOTICES.md).
