# SDL3 and platform implementation plan

Targets: Windows x86/x64/ARM64, Linux x64/ARM64, macOS ARM64. Windows x86/x64
currently run the application; ARM64 Windows is configured but not locally built.
Unix CI checks the ordinary-C core and shared settings. The optional SDL3 Windows
candidate now runs the real application with SDL presentation/input/timing; it
uses SDL PCM/FluidSynth MIDI and retains the Windows F10 editor. Default remains
Win32. No Unix runtime or ARM execution is claimed at this checkpoint.

## Keep the game and its owners

Preserve the software renderer, 320x240 indexed framebuffer, asset decoders,
gameplay/RNG, `BlazeApp`, `BlazeWorld`, `BlazeMixer` and `BlazeMedia` callbacks.
Platform migration changes presentation/devices/files, not recovered game behavior.
Keep Win32 as the comparison backend until SDL passes the same routes.
Do not add a second writable game state or redesign the renderer.

## Connected batches, in order

1. **Paths, settings and controls storage.** Shared C storage is integrated in
   both backends: strict numeric loading, exclusive temporary creation, flush and
   atomic replacement, unknown byte preservation, score/mode pairing and three-file
   ownership. All owned duplicate keys update together; collision failure must not
   remove another owner's temp file. Remaining work: fully separate asset root
   from writable root; retain executable-adjacent `dat/` on Windows and portable
   Linux packages. Installed Linux/macOS use a writable user directory with the
   same `dat/Blz.ini`, `BlzScr.ini`, `BlzCtrl.ini` names. No writes into app bundles.
   Do not silently switch Windows to AppData. Preserve existing Windows key/button
   bindings through an explicit mapping; SDL scancodes are not Win32 VK values.
   Handle asset filename casing and legacy INI backslash paths explicitly on Unix.
2. **SDL application, window/input/pacing.** SDL 3.4.18 is pinned/checksummed and
   statically linked in the Windows candidate. Its window/renderer/streaming RGBA
   texture, palette expansion, nearest-neighbor scaling, action masks and monotonic
   frame clock are connected. F10 still opens the Win32 editor; replace that
   dependency before enabling Unix. Preserve the current
   palette framebuffer, letterboxing and all screen modes without desktop changes.
   Validate keyboard/gamepad/joystick hotplug, F10 editor, focus/minimize/restore,
   pause, main/quit and bounded hidden/scripted runs. Devices/window/texture have
   one platform owner. Retain `Time=40`, monotonic deadlines and no catch-up bursts;
   presentation/VSync must not change simulation or RNG cadence. Gate on menus,
   real level loading, combat, repeated abort/new level, focus and shutdown.
3. **Complete PCM and MIDI lifecycle.** SDL PCM now feeds the existing mixer
   through a device callback, independently of frame work, with one bank/mixer and
   the stream lock around play/clear/mix. Its owned device closes before samples
   are freed. The F10 editor borrows an idle callback rather than an audio type.
   Exact multi-rate/clipping/voice tests and dummy/WASAPI stalls/reset/reopen pass.
   Portable MIDI now uses shared FluidSynth 2.6.1, built from pinned source with
   its native sample-timed SMF player. A worker owns synth, two song slots and a
   bounded stereo SDL queue. The GeneralUser GS 2.0.3 beta bank loads once at
   startup; song changes never reload it. FIFO requests, intent-aware EOF after
   queue drain, loops and stop cleanup are connected. Muted diagnostics retain
   the earlier no-wall-clock-EOF behavior and original RNG/selection/credits.
   CD requests report unsupported. Listening/timbre acceptance remains open.

   Static inspection of the 32 shipped MIDI files found 17 SMF-0, 15 SMF-1,
   PPQ division, 47 tempo events, GS/GM reset SysEx and controllers including
   modulation (1), portamento time/switch (5/65), reverb (91), chorus (93) and
   bank selection (0/32). A note-only player is insufficient. TinySoundFont's
   current MIDI-control switch does not implement those modulation/portamento/
   effects controls. FluidSynth covers those controllers and GM/GS reset; tests
   check their delivery and tempo changes. The SC-88 MODE-1 message in
   `blz_coin.mid` configures one hardware module/port, already our topology;
   FluidSynth does not handle that hardware configuration message. See the
   [Roland address map](https://cdn.roland.com/assets/media/pdf/SC-88_OM.pdf).
   GeneralUser GS author's sample-provenance caveat remains part of the separate
   distribution review. Dependency hashes and the complete build recipe are in
   `source/cmake/midi.cmake`; runtime packages include library sources/licenses.
   Evidence: [TinySoundFont handler](https://github.com/schellingb/TinySoundFont/blob/main/tsf.h),
   [FluidSynth](https://github.com/FluidSynth/fluidsynth),
   [FluidLite](https://github.com/divideconcept/FluidLite),
   [bank author's license](https://github.com/mrbumpy409/GeneralUser-GS/blob/main/documentation/LICENSE.txt).
4. **Six native applications and packages.** Run existing fixtures on all targets,
   then owned bounded platform sessions and manual visible/audio/controller tests.
   Audit integer overflow/shifts/conversions/alignment and pointer assumptions
   under GCC/Clang/MSVC; add focused coverage only for discovered issues. Windows
   ZIPs keep `blaze.exe`; Linux tar.gz keeps `blaze`; macOS packages `Blaze.app`
   with immutable assets in Resources, icon/Info.plist and user data outside it.
   Add ad-hoc signing/DMG validation; this is not notarization. Enable runtime
   artifacts and manual tagged releases only after all real application jobs pass.

## Evidence required

Record compilation/link, menu, level loading, gameplay, persistence, audio,
shutdown and native architecture execution separately. Cross-compilation and
headless core fixtures do not establish platform playability. Exercise all five
boss backgrounds, both weapons/abilities, damage, repeated mission changes,
score/unlocks, active-combat abort, focus, controller disconnect and final exit.
Retain natural campaign/manual acceptance gaps and matched performance baselines.

## Primary API references

- [SDL3 platform scope](https://wiki.libsdl.org/SDL3/FrontPage).
- [Streaming texture guidance](https://wiki.libsdl.org/SDL3/SDL_UpdateTexture).
- [Audio stream ownership/callback](https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream).
- [Application asset path and macOS Resources](https://wiki.libsdl.org/SDL3/SDL_GetBasePath).
- [Writable user directory](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath).
- [Connected gamepad enumeration](https://wiki.libsdl.org/SDL3/SDL_GetGamepads).
