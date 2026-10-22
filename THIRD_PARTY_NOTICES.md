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
no synthesizer, SoundFont or commercial music download is bundled.

The optional SDL3 backend statically links official SDL 3.4.18 under the zlib
license, preserved in `source/third_party/SDL3-LICENSE.txt` and its runtime package.
Source downloads are SHA-256 pinned in `source/cmake/sdl.cmake`; no SDL source is
modified. The Win32 comparison backend does not link SDL. A portable MIDI
synthesizer/instrument bank still require selection and their own notices.

No project-wide license has been selected for the reconstructed source. Before
public distribution, separately establish code, original asset, music and
dependency permissions. This file does not grant those permissions.
