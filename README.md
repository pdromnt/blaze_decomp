# BLAZE — native C reconstruction

A behavior-focused reconstruction of BLAZE 2.00. The game runs ordinary C:
no CPU/x87 interpreter, generated instruction execution or original executable
is used by the shipping target. Five stages, bosses, weapons, menus, SFX/MIDI,
pause, controls and score/unlock persistence are connected.

Windows x86/x64 are locally tested. Native CI has built and run the SDL application
on macOS ARM64 and Linux x64/ARM64, including deterministic gameplay and packaged
launches. Windows ARM64 CI is still pending. The SDL3 backend handles presentation,
keyboard/joystick input, PCM SFX, software MIDI, controls editing and pacing.
A complete natural campaign and real display/audio/controller acceptance remain open.
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
3.4.18, FluidSynth 2.6.1 and the GeneralUser GS instrument bank, and shares the
same game/settings owners. Music timbre differs from the Windows system synth;
listening acceptance remains open. Use `-NoDeploy` to keep root `blaze.exe` on
the stable Win32 backend. Asset lookup and writable paths are connected; Unix
hardware acceptance remains open. CD playback remains deferred.

On Linux/macOS, build the SDL application with CMake and a C/C++ toolchain:

```sh
cmake -S source -B source/build/native -DBLAZE_PLATFORM=SDL3 -DCMAKE_BUILD_TYPE=Release
cmake --build source/build/native --parallel 3
cmake --install source/build/native --prefix source/build/stage --component Runtime
```

Build trees share immutable assets and run without `--assets`. Linux installs
`blaze`; macOS installs `Blaze.app` with bundled assets/library. Native CI execution is verified; dependency setup is in the SDL build workflow.
Windows retains executable-adjacent `dat/`. Linux/macOS use SDL's user-data
directory with a `dat/` subfolder. `--portable` selects executable-adjacent data
on Linux; macOS bundles always use external writable storage. `--settings DIR`
selects an explicit directory; `--settings none` disables writes. Fresh folders
receive the original two INI defaults once, preserving later edits and full
startup configuration. Asset reads accept legacy backslashes and Unix ASCII
case differences; ambiguous case matches are rejected. UTF-8 paths are supported.

Arrow keys move/navigate, **Z** fires/confirms, **X** uses the ship ability/cancels,
**Esc** pauses, **F2** returns to title, **F9** exits and **F10** edits controls.
Keyboard and joystick bindings are saved in `dat/BlzCtrl.ini`; options and
scores/unlocks retain the original two INI filenames. First launch is windowed.
In the SDL controls menu, click or use arrows/Tab and Enter; Backspace unbinds,
Esc cancels capture/editing, and Save applies changes. Page Up/Down can set pad
button numbers without a connected controller. F10 always stays reserved.
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

CI builds six SDL application runtimes and preserves a Windows comparison job.
Download `runtime-<platform>` artifacts from Actions for ZIP/tar.gz/DMG packages
and SHA-256 checksums. Packaged launches and deterministic gameplay checks precede
upload; native macOS/Linux results pass; Windows ARM64 remains pending. See
[testing instructions](docs/TESTING.md) and [distribution notes](docs/DISTRIBUTION.md).
The manually dispatched release workflow requires all six packages from an existing
tag. Ordinary CI does not publish releases.
This is still under testing. Fixtures reach later bosses through controlled state;
they do not establish natural campaign victory, full scene parity or platform
hardware acceptance. No mid-mission/campaign resume is implemented.

Original game content retains its separate ownership. No project-wide source
license has been selected. See [Third-party notices](THIRD_PARTY_NOTICES.md).
