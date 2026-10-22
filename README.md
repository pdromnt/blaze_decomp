# BLAZE - RE/Decompiled

<img width="641" height="509" alt="Screenshot 2026-10-22 012637" src="https://github.com/user-attachments/assets/939ba904-134a-4a07-97a7-3a0e4027393a" /><img width="637" height="510" alt="Screenshot 2026-10-22 012930" src="https://github.com/user-attachments/assets/aee5224c-42de-4e56-ba3b-7236678be724" />

A reconstruction of **BLAZE 2.00**, the side-scrolling shooter by **TEA**, written
in C with SDL3 support for Windows, Linux and macOS. The original game data is
included alongside the source.

The game is playable, but still under active testing. Bugs may remain, especially
on platforms that have not had much hands-on testing yet. If you find one, please
report your platform, build and the steps that triggered it.

## Running

Download the `runtime-<platform>` artifact for your machine from
[GitHub Actions](https://github.com/pdromnt/blaze_decomp/actions), then extract the
whole package. Launch **blaze.exe** on Windows, **blaze** on Linux, or copy
**Blaze.app** from the Mac DMG and open it. Keep the Windows/Linux executable
beside its supplied files; the Mac app contains its own assets.

Supported targets are Windows x86/x64/ARM64, Linux x64/ARM64 and macOS ARM64.
Mac builds are ad-hoc signed, not notarized. See [Testing](docs/TESTING.md) for
installation details and things to try.

The game starts windowed. Options offers 1× (320×240), 2× (640×480), 3× (960×720)
and fullscreen modes. Scaling keeps the original pixel artwork sharp.

| Control | Action |
| --- | --- |
| Arrow keys | Move / navigate menus |
| Z | Fire / confirm |
| X | Ship ability / cancel |
| Esc | Pause |
| F2 | Return to title |
| F9 | Quit |
| F10 | Edit keyboard and joystick controls |

Choose **Save** in the controls menu to keep your bindings. Settings, scores and
weapon unlocks persist between sessions; mid-mission saves are not supported.
Windows keeps `Blz.ini`, `BlzScr.ini` and `BlzCtrl.ini` in `dat/` beside the game.
Linux/macOS keep them in the application's per-user data directory. Linux also
supports `--portable` for adjacent settings.

SFX and MIDI music are included. MIDI uses the GeneralUser GS instrument bank,
so it may sound different from the original Windows synth. CD audio playback is
not implemented; the original soundtrack is linked below.

## Building and contributing

On Windows, install the Visual Studio C/C++ tools, Windows SDK and CMake 3.24+:

```powershell
./source/scripts/build.ps1 -Architecture x64 -Backend SDL3 -Tests
./blaze.exe
```

Use `x86` or `ARM64` for another architecture, or `-NoDeploy` to build without
replacing the executable in the checkout. Rebuilds preserve your settings.
Omitting `-Backend SDL3` selects the older Win32 backend for comparison.

For Linux/macOS builds, dependencies and packaging, see [Building](docs/BUILDING.md).
The original executable, Python and Ghidra are not needed to build or run the game.
Technical details live in [Architecture](docs/ARCHITECTURE.md) and
[Platform support](docs/PORTABILITY.md).

Original game content and dependencies retain their separate ownership and terms.
No project-wide license has been selected for the reconstructed code. See
[Third-party notices](THIRD_PARTY_NOTICES.md) and [Distribution](docs/DISTRIBUTION.md).

## Original creator

BLAZE was created by **TEA — Ryohei Koizumi (小泉亮平)**, credited for programming
and graphics and named as the original copyright holder in the game's manual.
Visit [TEA's original website](https://www.asahi-net.or.jp/~vi8r-kizm/index_org.html).

The original CD soundtrack is available through
[this music release page](https://linkco.re/TNsDhama?lang=en).
