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

SDL3 is planned, not yet linked or bundled. Select a pinned official release and
preserve its zlib license when integrating it. A portable MIDI synthesizer and
instrument bank still require selection and their own notices/licensing review.

No project-wide license has been selected for the reconstructed source. Before
public distribution, separately establish code, original asset, music and
dependency permissions. This file does not grant those permissions.
