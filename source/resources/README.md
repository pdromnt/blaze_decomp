# Canonical static resources

Extracted without executing the original BLAZE 2.00 PE, SHA-256
`02332ea9c4746ce6255f044097c4b095f44c539f265927a3ca76ca01737fa74d`.

| File | Content | SHA-256 |
|---|---|---|
| rop.bin | 65536-byte WINGLROP palette-operation table | 9c8b740e4b45b2a8266c0f3d306d42f41376f027661aa6ec975d2e980cafe944 |
| trig.bin | 320 signed little-endian motion samples | 20cf95a9a6dfa8c13fb183f4acf118a836d327f0b7940f06e5579d0cdeaf00c7 |
| blaze.ico | Group-icon 1 repackaged as ICO | 9d244a7ea82847fa23327dc3da024a0fc9727262df5c6d5c5c0773a80a2d976f |

The embedded icon image itself hashes to
`8217ead39da5d7df0ce08b8931740325401d665dc9d3087933895cd176d9662c`.
Configuration verifies the three files; normal builds copy them, without Python
or an EXE. Optional reproduction using a separately supplied canonical input:

```sh
python source/scripts/extract_resources.py /path/to/original/blaze.exe /tmp/blaze-resources/rop.bin
```

`defaults/Blz.ini` and `defaults/BlzScr.ini` are pristine startup/package defaults.
Player INIs under root `dat/` are ignored and never overwritten on rebuild.
Original ownership applies to these resources; see root THIRD_PARTY_NOTICES.md.
