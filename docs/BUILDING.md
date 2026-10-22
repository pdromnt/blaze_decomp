# Building

Configure from source/. No private files, original executable, Ghidra or Python
are required. Extracted resources are committed and SHA-256 checked. SDL3,
FluidSynth and the instrument bank have pinned downloads/build recipes.

## Windows

Use Visual Studio C/C++ tools, Windows SDK and CMake 3.24+:

```powershell
./source/scripts/build.ps1 -Architecture x64 -Backend SDL3 -Tests -NoDeploy
./source/scripts/build.ps1 -Architecture x86 -Backend SDL3 -Tests -NoDeploy
# Default WIN32 backend remains the independent comparison:
./source/scripts/build.ps1 -Architecture x64 -Tests -NoDeploy
```

Run architectures sequentially. ARM64 requires matching tools and a host able to
execute ARM64 checks. Local evidence covers x86/x64. Builds use
source/build/windows-ARCH[-sdl3]/bin/blaze.exe, sharing immutable assets through
hard-links with copy fallback. They run without --assets. Writable build INIs
are independent and preserved on rebuild. Without -NoDeploy, the helper replaces
root blaze.exe after successful build/checks and deploys required dependencies;
existing root INIs and the preserved original remain intact.

```powershell
cmake --install source/build/windows-x64-sdl3 --config RelWithDebInfo --prefix source/build/sdl-stage --component Runtime
./source/scripts/package-release.ps1 -Version snapshot -Platform windows-x64 -StageDirectory source/build/sdl-stage
```

Use clean or same-purpose staging: install does not purge stale files. Packages
include pristine INI defaults, original assets, resources, dependency licenses and
corresponding synth sources, excluding player controls and private recovery.
Architecture/identity/dependency checks run before packaging into source/dist/.

## Linux/macOS application candidates

Use native C11/C++ tools and CMake 3.24+. Linux SDK dependencies are listed in
.github/workflows/build.yml. These real application paths are enabled but have
not yet built/run on a native Unix host:

```sh
cmake -S source -B source/build/native -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBLAZE_PLATFORM=SDL3
cmake --build source/build/native --parallel 3
ctest --test-dir source/build/native --output-on-failure
cmake --install source/build/native --prefix source/build/stage --component Runtime
```

Linux builds bin/blaze and installs blaze with assets and its shared library
alongside it ($ORIGIN). macOS builds/installs Blaze.app with assets in Resources
and the dylib in Frameworks (@rpath). Core-only builds remain available with
-DBLAZE_BUILD_APPLICATION=OFF and need no SDL/synth dependencies. Core success
alone does not establish application playability.

Windows keeps dat beside the executable. Unix defaults to SDL's per-user storage
with a dat subdirectory. Linux --portable uses adjacent dat; macOS rejects that
mode to avoid bundle writes. --settings DIR selects/creates an isolated directory,
seeding only missing Blz.ini/BlzScr.ini. All original startup fields remain effective.
--settings none disables writes; hidden runs do likewise unless an explicit
directory is supplied. There is no migration from another settings location.

Asset reads accept legacy backslashes and unique ASCII case matches for each
Unix path component. Exact matches win; ambiguity fails. User writes retain exact
names. Paths use UTF-8 and Windows wide APIs. Original asset names are ASCII;
full Unicode case folding is not claimed.

## CI and releases

Baseline jobs retain Windows comparison applications and Unix core checks.
Separate SDL candidates now cover Windows x86/x64/ARM64, Linux x64/ARM64 and
macOS ARM64 on native runners. They build/check/stage the real application, run
bounded gameplay, and package separate candidate artifacts. Unix runs use silent
dummy devices and enforce the known final framebuffer hash. Hardware-only checks
skip explicitly without devices. None of these updated workflows has run remotely.

The manually dispatched release resolves an existing tag to its commit and
requires all six runtime packages. Candidate validation and promotion into the
primary release jobs are still pending; candidates cannot satisfy that gate.
No local helper pushes, creates tags or publishes. macOS signing is ad hoc,
not notarization. Code/asset/dependency redistribution review remains separate.
See PORTABILITY.md for remaining native execution and manual acceptance gaps.
