# Distribution status

CI artifacts are development builds for testing, not a declaration that all rights
for a public release have been established. No public release is created by the
ordinary build workflow. The tagged release workflow remains manual.

| Material | Current status |
| --- | --- |
| Reconstructed source | No project-wide license has been selected; no new grant is asserted. |
| Original game assets, music, artwork and static resources | Original ownership applies; redistribution permission remains to be established separately. |
| SDL 3.4.18 | zlib license included; pinned source URL/hash in the build recipe. |
| FluidSynth 2.6.1 | LGPL-2.1-or-later; replaceable shared library, license, corresponding upstream source and complete integration recipe included. |
| GCEM | Apache-2.0 license and pinned source archive included with the synth sources. |
| GeneralUser GS | Author's full license included; its explicit uncertainty about historical sample provenance remains unresolved for distribution review. |

THIRD_PARTY_NOTICES.md preserves the component details. The original artwork is
repackaged into the Mac icon without redesign. No commercial music download or
original executable is embedded. Private recovery/oracle material and player
configuration are excluded from packages.

Package names identify the version and native architecture. Each archive has a
SHA-256 sidecar. Windows uses ZIP, Linux tar.gz, macOS an ad-hoc-signed app in a
DMG. Ad-hoc signing is not Apple notarization or an identified publisher signature.
The source repository and dependency recipes remain the build provenance; CI
records the exact commit, runs lifecycle/gameplay checks and tests the extracted
archive/mounted DMG before uploading it.

Before an external public release, resolve the source license and original
asset/music permissions, retain all dependency terms/source material, and review
the SoundFont provenance caveat. This document grants no additional permissions.
