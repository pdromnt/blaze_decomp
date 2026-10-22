# Building

Configure directly from `source/`; the public repository needs no private files,
Python, original executable or Ghidra. Extracted resources are committed and
SHA-256 checked during configuration.

## Windows

Install Visual Studio C/C++ build tools and Windows SDK for the desired target,
and CMake 3.24+. The script finds CMake on PATH or in Visual Studio:

```powershell
./source/scripts/build.ps1 -Architecture x64 -Tests
./source/scripts/build.ps1 -Architecture x86 -Tests -NoDeploy
./source/scripts/build.ps1 -Architecture ARM64 -Tests -NoDeploy
```

Run architectures sequentially. ARM64 tests require a machine that can execute
ARM64 binaries. The local host currently has x86/x64 compilers, not ARM64 tools.
`source/build/windows-ARCH/bin/blaze.exe` is the build artifact. Without
`-NoDeploy`, it replaces root `blaze.exe` only after a successful build and any
requested checks. Existing root INIs remain untouched; absent ones are seeded
from tracked defaults. The canonical original stays in private preservation.

The SDL3 Windows candidate uses the same script with `-Backend SDL3`; its separate
build directory is `source/build/windows-ARCH-sdl3`. SDL 3.4.18 is downloaded from
the official release with a pinned SHA-256 and linked statically. The default
remains `-Backend WIN32`. Both share one native settings writer; SDL currently
retains WinMM audio and the Windows F10 editor, so Unix application builds still
fail explicitly. Use `-NoDeploy` for comparison builds. To run one directly, pass
`--assets` with the absolute game root; deployed/staged packages need no such flag.

For a portable Windows runtime ZIP (use the matching platform label):

```powershell
cmake --install source/build/windows-x64 --config RelWithDebInfo --prefix source/build/stage --component Runtime
./source/scripts/package-release.ps1 -Version snapshot -Platform windows-x64 -StageDirectory source/build/stage
```

The installer includes assets/resources/docs and clean INI defaults. It excludes
private recovery, original EXEs, source, build tools and player controls/progress.
Use a clean or same-purpose staging directory; install does not purge stale files.
Packages are under `source/dist/`; architecture and application identity are checked.

## Linux and macOS — core checks today

Use a native C11 compiler and CMake 3.24+:

```sh
cmake -S source -B source/build/native -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBLAZE_BUILD_APPLICATION=OFF
cmake --build source/build/native --parallel 2
ctest --test-dir source/build/native --output-on-failure
```

These execute actual game/menu/lifecycle/mixer/shared-settings fixtures without a platform window
or audio device. They are not a playable Unix application. Requesting
`BLAZE_BUILD_APPLICATION=ON` fails explicitly until the remaining portable media,
controls editor and installed-user paths are connected.
See [Portability](PORTABILITY.md) for the connected implementation order.

Tests share immutable assets through a reusable build fixture tree with pristine
INI defaults; player settings do not affect expectations. Build output stays ignored.

## CI and releases

`build.yml` follows Uprising's native-runner layout: Windows x86/x64 on
`windows-2022`, Windows ARM64 on `windows-11-arm`, Linux x64/ARM64 on
`ubuntu-24.04`/`ubuntu-24.04-arm`, macOS ARM64 on `macos-15`.
Windows jobs build/check/package applications and smoke the staged EXE from an
unrelated working directory. Unix jobs currently check the core only, with no
runtime artifact. These workflows have not yet executed on GitHub.
Runner labels follow [GitHub's supported runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

Separate x86/x64 SDL3 candidate jobs run renderer readback/virtual input checks
and the existing 901-frame menu/combat/pause/abort route, then upload separately
named candidate ZIPs. They retain WinMM and F10 dependencies and cannot satisfy
the six-runtime release gate. Locally that route matched Win32 game/RNG state and
the final framebuffer exactly; it is still bounded Stage 1 evidence, not a natural
campaign or visible/controller/listening acceptance test.

Manually dispatch **Build release** with an existing tag. It resolves the tag
to a commit and calls the same build workflow with a real application required
on every target. Today Unix configuration rejects that request. Publishing also
requires all six named packages, preventing a partial-platform release.
Once SDL3 is integrated, enable Unix runtime installation/smoke/package steps
and replace the core-only CI labels with application labels.

No tag, release or push is created by local scripts. Signing/notarization and
redistribution permissions remain separate release work.
