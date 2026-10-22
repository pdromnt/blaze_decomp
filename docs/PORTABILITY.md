# SDL3 and platform implementation plan

Targets: Windows x86/x64/ARM64, Linux x64/ARM64, macOS ARM64. Windows x86/x64
currently run the application; ARM64 Windows is configured but not locally built.
Unix CI checks the ordinary-C core and shared settings. The optional SDL3 Windows
candidate now runs the real application with SDL presentation/input/timing; it
retains the existing WinMM media owner and native F10 editor. Default remains
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
3. **Complete PCM and MIDI lifecycle.** Feed existing `BlazeMixer` output to an
   SDL audio stream serviced independently of frame work. Synchronize only mixer
   mutation; never let a callback touch mutable game state. Preserve four voices
   per sound, reset/close ordering, no underruns on menu/song changes, pause policy
   and real music EOF. SDL PCM transport alone does not replace Windows MCI MIDI:
   select a portable synth plus licensed instrument bank, then cover both MIDI
   choices, reset files, two loaded slots, loops, random selection and credits.
   Pin dependency/data versions and notices. Keep CD deferred. MIDI timbre and
   licensing are the main unresolved dependency, not an excuse to ship silence.
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
