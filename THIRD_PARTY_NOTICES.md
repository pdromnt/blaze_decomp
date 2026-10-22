# Third-party material

Original BLAZE game content in `dar/`, `dat/`, `midi/`, `se/`, `html/` and
`html_en/` retains its original ownership and terms. Historical documentation and
music credits are preserved with that content. Reconstructing the program does
not establish redistribution rights for the assets or music.

`source/resources/blaze.ico` is original game artwork extracted for application
identification. `rop.bin` and `trig.bin` reproduce original immutable lookup data;
the INI defaults reproduce original configuration. Their provenance and exact
hashes are in [resource notes](source/resources/README.md).

The current Windows backend links Windows SDK system libraries (User32, GDI32
and WinMM). Windows MIDI playback uses the user's installed MIDI device/synth;
no commercial music download is bundled by that backend.

The optional SDL3 backend statically links official SDL 3.4.18 under the zlib
license, preserved in `source/third_party/SDL3-LICENSE.txt` and its runtime package.
Source downloads are SHA-256 pinned in `source/cmake/sdl.cmake`; no SDL source is
modified. The Win32 comparison backend does not link SDL.

The SDL candidate dynamically links FluidSynth 2.6.1 (LGPL-2.1-or-later), with
GCEM's Apache-2.0 headers used to generate DSP tables. Pinned source downloads
and all build options are in `source/cmake/midi.cmake`. Synthesis/player source
is unchanged; only unconditional upstream test/documentation CMake entries are
excluded. The replaceable library, its license, GCEM license and corresponding
source archives/build recipe accompany runtime packages. The library can be
rebuilt using that recipe or FluidSynth's upstream CMake instructions and its
public ABI, then replaced beside the game. No restriction on replacing or
debugging that library is intended.

SDL MIDI uses S. Christian Collins' GeneralUser GS 2.0.3 beta SoundFont, pinned
to author commit `684543d5e5efaef08d02be50dcda8d552478fa60`. Its full license
is in `source/third_party/GeneralUser-GS-LICENSE.txt` and runtime packages.
The author permits software use/modification but explicitly cannot establish
the provenance of every historical sample. Preserve that caveat in distribution
review; do not treat this selection as settled rights for public redistribution.
This bank changes timbre relative to the host's Windows MIDI synth.

No project-wide license has been selected for the reconstructed source. Before
public distribution, separately establish code, original asset, music and
dependency permissions. This file does not grant those permissions.
