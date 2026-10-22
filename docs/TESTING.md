# Testing a BLAZE artifact

Download the archive for your machine and its SHA-256 sidecar from the same CI
run. Extract the whole archive; the executable needs its adjacent assets/library.
These are development test builds. Automated checks do not establish a complete
campaign or hardware/audio acceptance.

- Windows: extract the ZIP and run blaze.exe. x86/x64/ARM64 are separate builds.
- Linux: extract the tar.gz and run ./blaze. Do not copy just the executable.
- macOS Apple Silicon: open the DMG and copy Blaze.app to your Applications folder
  or another location. Open the app. It is ad-hoc signed, not notarized; record
  any launch/security message exactly if macOS refuses it.

Arrow keys move, Z fires/confirms, X uses the ability/cancels, Esc pauses,
F2 returns to title, F9 exits and F10 edits controls. In the controls menu use
clicks/arrows/Tab/Enter, Backspace to unbind and Save to apply. F10 stays reserved.

Windows keeps dat beside the executable. Linux/macOS keep dat in SDL's user-data
directory. --settings DIR selects an isolated folder and seeds missing defaults;
--settings none disables writes. Linux --portable uses adjacent dat. Keep the
complete INIs: music mappings, timing and weapon metadata remain effective.
No settings migration or mid-mission resume is provided.

Please include the CI commit/run, OS/version, architecture, display mode and
exact route with a report. Useful manual coverage:

- Menus/transitions, F10 save/cancel, restarting and retaining options/controls.
- Movement, camera/views, all weapons/abilities, hits/damage and pickups.
- Music changes/loops, SFX quality and pause/focus/minimize/restore.
- Boss backgrounds/unlocks, completion/debriefing, ending and score persistence.
- Abort during combat, load another mission repeatedly, then final shutdown.

Check destroyed/unloaded objects stop updating/rendering. For speed comparisons,
match level, ship, camera, resolution and graphics settings. CD playback remains
deferred. MIDI uses GeneralUser GS, so timbre differs from the Windows system synth.
