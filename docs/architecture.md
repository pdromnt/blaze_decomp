# Native boundary and resource contracts

## Execution roles

- `game-recomp/original/preserved/` (private): immutable canonical executable/data; full-game oracle
  previously observed in active Stage 1. SHA-256 of `blaze.exe`:
  `02332ea9c4746ce6255f044097c4b095f44c539f265927a3ca76ca01737fa74d`.
- `game-recomp/recomp/` and its x86 differential tools (private): preserved extraction/diagnostic code.
  No whole-game hybrid or CPU/x87 runtime was implemented at takeover.
- `source/`: ordinary C candidate; `blaze-native` links only `blaze_game` and
  Win32 presentation/input and WinMM audio libraries. No generated execution, guest memory,
  general x87 mechanism, guest ABI dispatch, or fallback to the original.

The old translation/fallback strategy applies only to oracle experiments.
Shipping reconstruction proceeds from actual application dependencies.

## First connected family: resource-backed screens

`BlazeApp` owns the font, current screen archive, extracted ROP table, framebuffer
and current `BlazeLevel`. Every archive owns one byte buffer and one metadata
array. Sprite pixel offsets reference that buffer; there are no serialized
pointer fields or mutable guest mirrors. A replacement is parsed before the old
owner is released. Closing the app releases level arrays/archives, screen, font
and ROP. The backend owns HWND/HDC lifetimes. No callbacks survive destruction.

Recovered native path replaces file loading/registration (`0x424d80`, `0x424ec0`,
`0x424a40`), archive release (ownership formerly split across pattern slots and
archive groups), unscaled direct/ROP blitting (`0x427e60`, `0x428030`), font drawing
(`0x419500`), and opening/title/selection behavior (`0x402ad0`, `0x402bb0`,
`0x4030f0`). The candidate calls ordinary C functions throughout this path;
original bodies remain independently executable in tests.

Selection, Options and Ranking now share the native outgoing snapshot transition
from `0x4191c0`: capture at tick 0, vertical squash at ticks 0..8 (y=15*t,
scale=258-32*t), black through tick 16, incoming menu draw from tick 17. The app
owns one immutable capture, frees it at tick 9, on an early exit or shutdown, and
constructs incoming archives before replacing their previous owner. Assets are
read on entry; their palette and music become active at tick 16. `0x427d00`
restarts vertical row accumulation after clipping, unlike `0x429220`; the native
capture operation preserves that distinction. All 25 transition frames are checked
against the canonical clipping/scaling/direct-blit bodies in the existing x86
renderer fixture. The oracle alone maps those instruction bodies.

Options (`0x403770`) retain one applied and one bounded pending value per setting.
Tick 16 refreshes all pending values; X discards edits, EXIT commits. Screen
confirmation applies/saves the display preview immediately; other confirmations
replay the loaded Option track without a new RNG roll. Ranking (`0x403540`) reads
the five configured scores. Option entry returns to Options; title EOF enters
Ranking and its dismissal/EOF runs dispatcher state 12's tea/title sequence.
Ending score insertion and awarded unlocks persist through a native platform callback.

Native platform callbacks take ordinary display/settings values. Windows owns
its HWND and three INI paths; the app borrows those services. Atomic replacement
updates options in `dat/Blz.ini`, scores/unlocks in `dat/BlzScr.ini` and controls
in `dat/BlzCtrl.ini`. An explicit `--settings directory` isolates the three
files; hidden runs otherwise disable persistence.
Full-screen uses borderless monitor bounds with aspect-preserving scale or a
centered 320x240 image, replacing legacy DirectDraw exclusive mode switches.
The two scaled full-screen entries share the GDI backend; original exclusive
mode/driver parity is not claimed. Startup remains windowed by policy.

Cadence now reads `[Enh] Time` as a millisecond interval, as WinMain `0x402920`
passes to `0x4179b0` and the `0x417bf0` callback. The shipped 40 means 25 ticks/sec;
the original missing-key default is 25 ms (40 ticks/sec). Native startup clamps
invalid/extreme values to 1..60000 ms. Monotonic platform deadlines include work
time, use QPC rather than coarse GetTickCount64, pump messages while waiting,
rebase on overruns and reset across focus
pause. They replace the incorrect unconditional Sleep(25). There is no busy wait,
catch-up burst, frame skipping or change to per-tick gameplay/RNG. Exact original
legacy scheduler quirks and heavy-load timing parity are not claimed. The backend
requests 1 ms wait resolution only for its running loop and balances it at exit.
Read-only inspection of the user's running original also verified a 40 ms period.
The matched 251-frame hidden/menu diagnostic previously spaced frames 30.017..
49.140 ms apart; QPC/wait changes yielded 38.075..41.781 ms, both taking 10 s.
These are bounded CPU/GDI submission intervals, not visible display/GPU timing
or a matched heavy-combat performance claim.

Equipment presentation retains its update-time native draw command. Original
`0x407740` computes the mount before pass-1 pickups, and `0x416050` presents the
old equipment then. Reading the live selected weapon during the later native
frame draw mixed a new pickup sprite with an old mount rectangle. The bounded
command now retains sprite, position and half-scale together until next update;
world destruction clears it with the other draw streams. It is presentation data,
not another writable player owner. Original mount prefixes and blits independently
verify shipped/synthetic offsets on every ship/weapon and pickup/next-frame order.
LIGHTNING `0x4120b0` deliberately uses hull x+48/y+16 rather than the configured
mount; this original behavior is preserved. Other weapon update/creation drawing
is compared using both real INI mounts and synthetic stress offsets.

Remaining feature: campaign save/load. Controller hardware acceptance is pending. Unsupported gameplay routes still fail loudly.

## Audio and pause ownership

The Win32 comparison backend's `BlazeWindowsAudio` owns one decoded bank, four waveOut buffers and a music
owner. The music owner exclusively manages two MIDI slots and the deferred CD
backend on one CRT worker thread. The app owns a non-owning `BlazeMedia` callback table;
its world references that table, including after continue. All audio calls take
native sound/slot IDs. The platform outlives app shutdown; stop/reset precedes
buffer/sample/device destruction. No audio thread mutates world state.

Music callbacks enqueue owned native load/play/stop requests in order. Complete
paths are copied before submission; no worker borrows app/world/track-name state.
File reads and every MCI operation, including loop status/restarts, run on the
worker. The short queue/publication lock never covers I/O or driver calls. The
worker pumps its own MCI window messages. A separate PCM worker primes/submits
the four 256-sample waveOut buffers and wakes on completion events. The previous
main-thread 40 ms refill could starve this 46 ms queue. One authoritative mixer
has a short lock around play/clear/mix; driver calls never hold it. Silence clears
voices and requests a worker reset. Close signals and joins that worker before
reset/unprepare/device close and bank destruction, including partial-open failure.
Muted diagnostics alone mix synchronously. Neither PCM service nor frame
submission waits for music. Actual driver transitions can still leave a music gap.

The SDL backend now owns `BlazeSdlAudio`: one `BlazeSdlPcm` and the independently
retained Windows MIDI worker. It does not link `windows_audio.c` or enter WaveOut.
The PCM stream requests mono S16/22050 input and SDL converts for the device.
Its callback mixes bounded 256-sample chunks without touching game state or I/O;
the stream lock serializes mixer mutation. Sound callbacks take that same lock.
Silence clears voices and queued stream data without taking a device lock under
the stream lock. Destroying the stream stops/joins callbacks and closes its device
before bank release. Partial opens are closable. A submission failure propagates
through an atomic flag to the application. Muted diagnostics alone keep the old
synchronous 256-sample pump. The controls editor accepts an idle callback/context;
it no longer requires a `BlazeWindowsAudio` owner.

Unbound-stream tests compare exact multirate/clipped samples and all voice state
with the existing mixer across callback chunk boundaries. Dummy and silent WASAPI
checks cover 400 ms without main pumping, queued-sound reset and six active
close/reopen cycles on x64/x86. This verifies transport, not audible balance or a
portable MIDI synth. Music request/EOF semantics remain those of the existing worker.

The worker alone owns playback state. `BlazeMusicStatus` is a read-only publication
for completion/diagnostics, protected by the same short lock. Submission intent
sequences invalidate stale EOF immediately; preloading an inactive slot preserves
the active song's EOF. Normal game code never waits for acknowledgement. Shutdown
cancels queued requests, joins the in-flight operation, closes devices on their
own thread and releases worker/event handles before freeing audio storage. An
in-flight driver call may delay final shutdown, after gameplay/window exit.

The 18 channels follow root 0xdbc4..0xdc90 in stride-12 order. Each rotates four
voices as original 0x427a30 did. Existing `se/` files match PE WAVE resources;
shipping needs no executable access. RIFF/PCM fields decode explicitly; mono16
samples have one owner, with direct Q32 resampling to 22050Hz and saturating mix.
The light effect is `se_ligh.wav`, not the different `se_lig.wav`.

Native music selection replaces 0x418fb0/0x418c60; playback/stop/end handling
replaces 0x418d40/0x418e80/0x418ed0 through WinMM. Modes 0/3 consume one original
random roll per load, even muted. The inclusive 32767 endpoint selects missing
third key `0` deliberately. Fixed modes 1/4 and 2/5 use choices 0 and 1. First
filename duplicate wins (the shipped Option1/2 keys repeat). Stage and boss slots
are loaded at START; warning stops stage music and starts boss slot at age 120.
Selection/Option/Ranking load their cues at original tick 16; leaving earlier
skips that music selection and its RNG roll. Title EOF now enters state 14
Ranking through the native ended callback. Ending loads group 5 without looping
and returns on real EOF or a confirm after its final image.

Escape toggles manual reason 2; loss of focus/minimize adds reason 1. Simulation and
RNG remain frozen until all reasons clear; music and active voices continue.
The original pause vtable entries resolve to 0x4176e0 (window caption only) and
0x401110 (pause overlay/presentation), with no audio suspension. Pause text and shortcuts
come from 0x401110; combined reasons follow 0x4177b0's platform ownership. Resume
has no catch-up ticks. Windows sequencer can report PAUSE at EOF; the native
backend handles it as finished playback because gameplay pause does not suspend
the device. F2 stops/silences, then destroys the world; F9 and final
shutdown stop playback and release buffers. Device failures are reported.

Tests compare ordered SFX and music stop/play requests in existing original-code
weapon/actor/damage/pickup/game-over fixtures. Focused native tests cover PCM,
resampling/clipping, RNG/INI edges, deferred option changes, combined pause
reasons and repeated paused combat teardown. A silent real-device fixture checks
PCM queue/reset, two MIDI slots, looping, EOF, replacement and
close. The device fixture also covers ordered rapid scene changes, stale EOF,
20 pending-close/reopen cycles with stable handle counts, and real-device close
with queued work. It measures caller/pump latency separately from driver work;
this is a silent transport check, not combat frame-time or audible-gap evidence.
Physical CD, human listening and interactive focus/minimize remain open.

## DAR evidence and decoding

Static evidence: canonical instruction CSV plus live Ghidra `0x424a40` and both
blitter bodies. All eleven shipped archives parse to their exact byte lengths.

- Signature `DAR:8`; byte 5 selects header version 0/1; LE16 count at 6.
- Palette descriptor at 8, 256 BGR0 entries at 12; records start at `0x40c`.
- Version 0 has an eight-byte record header. Version 1 prefixes each header
  with LE16 length, excluding that length word itself.
- Record words: width, storage height, row stride, signed transparency value.
  Headers over 13 bytes also store signed origin X/Y and visible height.
- Each row contains LE16 transparent skip/literal count pairs, literal palette
  indices, and DWORD padding. Storage rows are bottom-up. Cropped rendering
  starts from `storage_height - visible_height` at the destination bottom.
- Blending indexes the exact original table as `(source << 8) | destination`.
  `0x420060` copies WINGLROP/ROP unchanged. Its 65536 bytes are committed under
  `source/resources/`, verified at configuration and reproducible by the optional
  hash-gated extraction script. Neither build nor application reads the PE.

The native blitter matches 20,390 whole-frame comparisons against both original
machine-code bodies, across every sprite, clipping positions and varied initial
destination pixels. This establishes those drawing operations, not the whole
renderer or frame/state transition parity.

## Level data ownership and remaining dependency

`0x407a60` is the loading screen; it calls `0x407a10`, which calls `0x407220`
(file-backed loader) and `0x416670` (player/effect pool initialization).
`0x404f10` displays START while running `0x4050a0`, the world update/render loop.
These distinctions were verified in Ghidra; old broad gameplay labels were
insufficient to identify the first simulation frame.

Native file loading owns object/map DARs, three uint32 tile arrays and 128 spawn
records. Layer widths are `2560/(layer+2)`, heights `70/(layer+5)`; map files are
134812 bytes, layer-major. Spawn files are 2560 bytes: 128 groups of five LE32
words: drop flags, actor type, pending flag, x and y (`0x4075b8`). Words 3/4
become coordinates through unsigned wrapping `<<5`. Type zero is valid; consume
the pending flag, preserve the actor type and pass the drop flags separately.
Coordinates use the original
32-pixel tile units. No host struct dumps or aliased reads.

Construction checks real assets and unwinds on failure. Missing/corrupt files
fail explicitly instead of recreating the original loader's synthetic-map
fallback. No gameplay route is declared successful on that basis.

The level loader covers file-backed work, not original actor reset/player setup.
Player initialization `0x416670`, HUD reset `0x413710` and weapon selection
`0x415f50` now execute as native C. The player's single bounded numeric record
allocation preserves verified pool offsets and unknown fields. Slots formerly
holding parent/player pointers are unused; functions receive the actual level,
loadout and world references. These records contain no CPU registers, flags,
guest stack, callable guest addresses or instruction bodies. Reset preserves
unspecified fields as the original does; world destruction clears the complete
record owner and actor registry before releasing level assets.

Full-record differential comparisons cover all ships, modes, weapons and stage
sprite tables, including populated/empty/matching three-slot weapon inventories.
The oracle's call boundary was checked against assembly: `0x416670` uses `RET 8`
with two unused stack arguments that its Ghidra prototype omits. No such guest
calling convention is exposed in shipping C.

`BlazeWorld` owns the player and actor registry, references the app's single
level owner, font, loadout and application RNG, and advances the recovered shared
input/weapon flow of `0x410920`. Movement `0x416290` uses the exact 320-entry signed integer
table committed under `source/resources/`; conversions and wrapping additions are explicit.
Original-code comparisons cover all directions/bounds and full no-weapon update
state, with only the input-poll boundary controlled by the test.

The app connects loading and START to the supported prefix of `0x4050a0`:
player updates, camera/scrolling, map layers, banking, health/energy bars and stock
icons. Native references and buffers remain owned until diagnostic abort or
shutdown. The original transition effects, scaled weapon icon and full HUD/world
frame parity are not yet recovered. Escape now pauses/resumes; F2 releases the
world and returns to title. BLAZE_ABORT remains a fixture-only diagnostic bit.

Primary action 10 now scans the 48-slot pool and constructs native shots with
ship-specific rays and original weapon-0 mount offsets. `0x4145f0` is ordinary
translation, both ordered actor-hit queries, impact emission and lifetime expiry.
The target active flag is tested even after the source shot becomes inactive;
both queries visit every eligible overlapping actor. The 32-slot spark pool
preserves busy-slot RNG consumption and the original update/draw order.
Controlled-seed original-code fixtures compare complete records, damage, RNG,
retirement frames and reuse. The application owns the single RNG state, its
256-entry startup noise table and four wave phase/step records. Startup seeds
from epoch seconds and consumes the original 272 rolls. The x86 fixture executes
the audited original initialization block at `0x4024a8..0x402706` for comparison.
World construction references this owner; teardown never reseeds or copies it.

The first Stage 1 flyer (type 22) is registered through a rotating native pool,
with an explicit one-based source handle. Construction, flight, pass selection,
score decay, timeout/miss accounting, hit-flash, contact damage, death, score and
drops are native. Explosion types 27/41, smoke type 26, score labels type 46 and
weapon pickups type 11 run through the same registration and lifetime chain.
Pickups use the existing native weapon-selection/inventory implementation.
Unsupported actor families retain their pending spawn; none is silently skipped.
Recovered Stage 1 combat now reaches the boss; full mission/campaign acceptance remains open.

Rotation `0x4284a0/0x429ea4` uses direct integer polygon edges, inverse texture
coordinates and a fixed quarter-wave table. Each archive owns immutable decoded
pixels derived from its validated DAR bytes, bounded to 64 MiB, freed with the
archive. No per-frame decode/allocation or writable asset mirror exists. Signed
shift/MulDiv rounding is expressed directly. The renderer fixture executes the
original FSIN/ftol initialization, geometry and rasterizer; only source/surface
acquisition is controlled. It establishes the operation, not whole-frame parity.

World-owned bounded draw lists are presentation commands valid for one frame,
not another writable game state. They retain the original final rendering of a
shot, spark or flyer after its active flag clears. Original ordering is actor
pass 0, player pools, actor palette reset/pass 1, foreground and HUD. Currently
supported flyers use pass 1 and explosions use pass 0. The draw-list watermark
preserves that layering around player/weapon presentation. Attached hit effects
follow the foreground, then HUD rendering. Abort clears records, registries,
commands and owner references before releasing the level; the app RNG remains
alive. Repeated combat abort/reload is covered in the native lifecycle check.

Player damage updates the existing HUD record, registers attached effects with a
player handle, and applies the original scroll recoil. Regeneration and its
8-slot presentation pool preserve roll order. The low-health trail uses its
original six bounded rectangle-history slots; it keeps a player handle and has
one authoritative writable record. No target pointer is narrowed into a word.
Hit effects, trail and regeneration retire through the same world lifecycle.
Weapon names/enabled flags now live only in the loadout shared by menus and
pickups; a temporary configuration copy was removed before integration.

Scaled direct/ROP/tint drawing (`0x429220` and its rasterizers) uses original
forward spans and bottom-up row selection. Fixture evidence preserves the
zero-length clipped upscale word write within framebuffer bounds and the tinted
upscale first-run termination from `0x42ba4d/58`. Those are encoded as visible
operations, without making palette pointers into coordinates. Palette mapping
keeps the original reserved slots, strict improvement and exact-match early exit.
Actual world palette and horizontal health-bar scales are compared separately.

A rare original pickup roll can produce weapon index 11 when RNG returns 32767.
The recovered configuration owns indices 0..10. Constructor 0x401b36 builds
exactly eleven stride-0x3c records; 0x401b60 initializes starfield pairs at 0xd90c.
The index-11 enabled lookup at 0x40d9f7 reads 0xd924, a starfield X coordinate,
and its power lookup aliases the font owner at 0xd904. This is an original
out-of-bounds bug, not another weapon. The candidate deliberately rejects roll
11 and rerolls through the same RNG operation; valid rolls/order are unchanged.
A forced seed 0xf01bf641 produces 32767 then 15401: the pickup safely selects
weapon 5 and can be consumed. This deviation is separate from parity claims.

Type-1 enemies and their type-43 shots share the world-owned actor pool; no
parallel projectile owner or mirrored state was introduced. The world references
its application's immutable trig table and clears that reference on teardown.
Enemy rotated commands stay in actor pass 1, after player/shot presentation;
they do not enter the primary-shot command list. Type 43 accelerates by dividing
trig components by `(32-age)*2` for ages 0..31, then retains its final velocity.
It presents and applies contact before strict viewport retirement. Interception
uses the existing second primary hit query (flag 0x80), not the first (flag 2).

The direct C turning operation (`0x419740`) compares candidate endpoints from
the player center to the actor center, using wrapped integer differences and
Euclidean distance. Its two strict inequalities can both hold, cancelling the
adjustments in the dead band. C uses `sqrt`; 40,960 bounded turning decisions
and connected lifecycle frames agree with the independently executed original
helper and original pow/CRT dependencies. Those machine-code routines live only
in the x86 fixture. This is bounded decision evidence, not exhaustive floating
point equivalence for arbitrary integer coordinates or rounding environments.

Type 0 uses the same actor owner and direct turning operation (step 3). Ages
0..64 move before drawing, then age 64 reverses vx from -4 to +4. Later frames
present first, emit type 20 on even ages, then move. Steering updates direction
but does not replace this horizontal velocity. The final dead/expired frame can
still present, fire and drop a weapon before the shared lifetime tail runs.

Type 20 uses expansion -6 and flags 0x90. Initial velocity divides the exact
trig table by `(16-difficulty)*2`; it moves, submits blended presentation, applies
13 contact damage, then runs strict viewport/death/lifetime retirement. Both
primary queries select it, applying 4 then 5 damage even after the first query
clears the shot's active flag. RNG/impact commands must retain that order.
Type-0 health/score/drop and type-20 records share existing pool storage; neither
introduces a new allocation or executable-code callback.

Type 8 selects its original movement/firing variant and banks from previous
vertical velocity before updating motion. Its native children are type-3 shots,
type-16 missiles and type-37 beams, all in the existing actor pool. Type 2 shares
type-3 firing, uses direct distance steering and emits the original three hit
sparks with four ordered RNG rolls each. No enemy execution callback remains.
Missiles track vertical position for five ages, accelerate horizontally through
age 65, then retain velocity; flag 0x80 does not request viewport retirement.

Attached registration (`0x407630`) passes a checked one-based actor-slot handle
in record offset 0x34. The offsets at 0x38/0x3c translate the parent's rectangle
as a point before centering the beam; they do not resize that rectangle. One
world owns parent and effect records. There are no copied parent objects or
narrowed native pointers. Legacy slot identity is preserved: reuse before the
beam observes retirement makes it follow the new occupant. During ages 0..15,
the original beam charges without checking parent activity; subsequent phases
present/contact first and retire after observing an inactive parent. Teardown
clears all these records and references immediately, independently of that delay.

Beam extension submits at most 32 blended segments, stopping at the first
player contact. Its final phase shrinks only Y and remains blended. The bounded
actor command array now holds 256*32 commands; positive `height_scale` overrides
uniform Y scaling, while zero retains prior uniform commands. Commands remain
transient presentation, not writable copies of actor state. Original-code state
comparisons cover parent death/reuse and final frames; 64 additional pixel
comparisons cover the actual vertical fade rasterization at viewport edges.

Death is a world-to-application transition, not a reset-health shortcut.
`0x4050a0` enters state 8 after actor pass 1, sets the HUD timer to 24 and leaves
the world owned. `0x404a70` updates that world on phases 2/3 of each four-frame
group for its first 32 ticks, with two camera-jitter rolls before the update
and three ordered explosion bursts after restoring camera coordinates. During
these pulses normal camera/scroll translation is frozen; player/effect updates
retain their ordinary code paths. After tick 31 the world stops updating.

At tick 32 one immutable rendered-image allocation is captured in the existing
bounded span format. It is the sole snapshot owner, not a parallel game-state
copy. The snapshot shrinks/moves with the original phase schedule and uses a
red conversion table; font caption colors and prompt blinking retain their
phase counters. Conversion deliberately uses font-source colors and the active
destination palette, even for captured scene pixels. `blaze_color_map` handles
that cross-palette operation directly; uniform-color helpers remain wrappers
with the same source and destination palette.

Input suppression compares the complete mask. Continue needs an edge after
phase 48; decline is checked after increment, so an X edge at phase 48 works.
Continue wins a simultaneous Z+X edge. It clears score; score attack also clears
weapon stock. At the next loading tick, assets are transactionally reopened and
the world resets in place, carrying the authoritative player record through
reset to preserve current ship/weapon and the original stock increment. RNG
stays application-owned. Decline, timeout, diagnostic abort and shutdown release
the captured image and world owners. Player 0x5c is Z+X weapon stock, not lives.
The original world's second unused argument is proven by its RET 8/callers;
that calling convention exists only at the x86 oracle boundary.

Game-over dispatcher comparisons execute the original routine while controlling
its world/platform boundaries; independent actor/world-prefix and rasterizer
comparisons remain. They do not establish full original scene/startup/backend
copy timing or audible parity. Existing loading/START presentation remains a
bounded native candidate rather than a whole-scene equivalence claim.

Stage 1 type 12 creates type 21 inside its constructor. The parent slot is
already active while both calls observe the same actor cursor; child registration
consumes an attempt before outer registration increments it again. Full pools
consume attempts as in the original, including a valid parent with no room for
its attachment. Parent handles name actual registry slots; the unrelated legacy
source identifier on ordinary spawns still follows the original scan offset.
No additional registry, reservation layer or copied parent is introduced.

Type 21 updates in pass 0, copies its parent's rectangle plus the stored offset,
uses the parent's hit palette, presents, then retires if that slot is inactive.
Thus a parent killed in pass 1 remains visible through its child's final pass-0
presentation. Reuse before observation follows the new occupant. Original-code
checks cover nested cursor wrap/full pools, reuse, primary kills, early/late
death, drops, firing and expiry. Type 12 retains the original five-shot order,
shared jitter roll and strict age-500 escape boundary.

Stage 2 waves reuse this owner and explicit stage/type support gate. Its carrier
has 400 base health, child offset (-134,-32), child sprite 0x29c, spread sprite
0x6b and phase-gated type-6 shots at (+32,-32). Type 19 combines two movements
and uses the original wrapped squared center distance (0x4199a0), not Euclidean
distance, before clamping to camera X and world Y bounds. Type 31 turns toward
player center minus (128,64), with step 16; it creates type 35 and attached type
10. The latter centers in the followed parent rectangle, uses the existing
startup noise table and submits up to 32 segments before contact/retirement.
Type 35 expands after countdown 16; type 44 creates eight type-43 shots on death.
All use existing actor slots, checked parent handles, player collision/damage,
ordered draw/audio commands and shared death/drop tails. No parallel mutable
actor representation or allocation owner was introduced. Stage 4 onward stays
rejected until its own connected behavior is recovered.

FERIA (Stage 2 type 4) creates children 7/9/38 and three type-45 targeting markers
before enlarging its collision rectangle by (96,16) on each side. Its clamp uses
sprite dimensions after a bound is crossed, as in the original. Separate native
phase control preserves both moves in phase 2, repeated pursuit/clamp steps,
triangular spread order, health-800 phase selection and RNG order. Type 7 starts
animation at zero, banks from parent velocity and keeps expansion 32. Type 2's
Stage 2 sprite/mask differ from Stage 1. New type 33 waits, aims at age 24, sounds
at 32, then checks damage before moving/steering; its draw is additive. Existing
weak-point damage transfer, parent-slot lifetime, boss debris and report handling
serve this family without another owner. The death translation moves rectangle
coordinates without overwriting the stored velocity. Warning caption selects
TYPE 01 FERIA. Oracle-only relocation of the original state-index table enables
independent execution of this stage's switch; no loader entered shipping.

Stage 3 reuses the same actor pool with its own sprites, masks and carrier
health/child/emission choices. Type 23 creates an enemy trail in the player's
existing 0x2ddc allocation; it is not another actor registry or a serialized copy.
Eight 0xd4 records each retain eight rectangles/angles. Constructor preserves old
angle history, fills rectangle history and refuses a busy slot without mutation.
Update steers twice by 3 during age%32>8, moves, shifts history, draws oldest to
newest with contact damage 28, restores the latest rectangle and retires after
age 48. World updates these after G-BRASTER, before actor pass 1; stock clears
this same pool, and world teardown/reset removes all active records. Type 24
uses steering step 3; broader coverage corrected shared type 33 to step 2 from
assembly. Type 36 preserves the original right-aligned LASER& warning and
segment-growth/fade/contact order.

SESIRU creates body 7, four turrets 5 and blade 9, retaining initial mask 0xf06
through its long flyby. At age>1329 it enters mask 0xb04, registers targets and
selects among seven attacks. Its inclusive RNG can also select unhandled phase
8, where the original keeps steering without cycling; that quirk is preserved.
These use attached missiles 30/effect 10, beams
18/29, projectiles 34/16/17, trails 23 and hazards 36. They share existing
registration, collision, score/death/report and resource owners. Turrets resize
to sprite 0x140 after aiming; body uses Stage 3 sprite 0x2cc. Stretched draw
commands use height_scale=-1 for an explicitly empty Y dimension, distinguishing
the beam's zero-height opening from uniform scaling without another state copy.
Original execution remains test-only, including the added SESIRU switch-index
relocation and hash-checked trail constructor/update bodies.

The first 32 child frames use an unscaled per-row sine displacement. Its exact
quarter-wave bytes are immutable numeric constants, reflected by symmetry;
negative products explicitly round down. Clipping restarts the wave at the
bottom visible row, preserving the original renderer. `BlazeActorDraw.wave_age`
is transient presentation data, not actor state. Direct/tinted full-frame pixel
comparisons cover both child frames, all reveal phases and viewport clipping.

The Stage 1 boss uses the same nested registry for body 7, turret 5, blade 9,
laser mount 15, weak points 38 and targeting overlays 45. Turret shots, homing
shots 6, supply carrier 17, laser 28, death effect 14 and debris 47 execute native
functions. Weak-point damage transfers to the parent before resetting that
record's health to 200. Child removal follows each original policy: phase-100
turret/blade/weak-point retirement differs from body/mount activity checks.
The registry remains the sole mutable owner; full pools consume the original
attempts, including partially populated boss families.

Warning 13 starts encounter phase 1 on Stages 1..4; the final stage preserves
its background instead. The world captures presentation phase
before actor updates; phases 1..17 tint the background and enter phase 18 on
the last fade tick. The phase-18 switch uses texture 0x2b6 for Stage 1 and
0x2cd for Stage 3. Stage 2 uses 80 moving 0x144 streak sprites; Stage 4 uses
40 of sprite 0x145. The switch also has Stage 5's 80-streak branch, but its
normal warning deliberately leaves phase 0, retaining its map; no transition
was invented to activate the dormant branch.

Application-owned wave records advance X by field 2 and Y by field 3, without
consuming RNG rolls. These steps were previously swapped. Four exact integer
cosine displacements position the warped DAR backdrops. The 80 original 0xd90c
streak records now share that application owner and survive mission teardown.
Their constructor runs from CRT seed 1 before the time-seeded gameplay stream;
each tick moves X by -48, and X below -128 consumes X then Y respawn rolls.
Update runs after camera clipping and before actor/weapon RNG, including during
player death. World-owned draw positions are bounded to one frame, contain the
camera-adjusted presentation only, never copy back, and clear at teardown.
`quad.c` directly interpolates four textured triangles
with original edge/clipping/paired-pixel rounding; it does not execute MMX or
emulate instruction state. Opaque-backdrop pixels match the original MMX path
for 192 clipped, folded and offscreen geometries. Full original scene timing,
palette initialization and alternate renderer paths remain separate acceptance.

The bounded original 0x405340..0x40564f switch and 0x401d20 constructor now
verify all five branches across 384 frames, camera offsets, wraps and repeated
stage changes: authoritative star/wave state, RNG ordering and composed pixels.
These tests intercept drawing at previously verified native raster primitives;
they do not compare a complete original scene. Native ERISU death/report checks
also verify bottom-right unlock pixels flashing only at odd report ticks for
MISS 0 with LIGHTNING locked, then the award on exit; nonzero misses and already
unlocked weapons suppress the notice, matching the original 0x403f20 comparison.

Boss death retains its parent for the original phase-101 movement while effect
14 submits flashes/debris and requests application state 4 at age 96. The native
stage-clear dispatcher retains the world during its opening pulses, then reports
misses/score, awards the exact zero-miss bonus and requests the next stage.
Resource replacement and player reset use the existing transactional continuation
path. Final stage completion enters the native state-10 ending. Weapon unlocks
are app-owned state and save through the platform when awarded.
Oracle tests intercept settings/audio/presentation boundaries, never writing
original settings or running the original application. Progressive text timing
is a controlled boundary in dispatcher tests; glyph rasterizers compare separately.


## Shared controls and player weapon pools

`weapons.c` recovers base held fire, FORCE JET, LIGHTNING, FIRE STORM,
NOVA WAVE, S-LASER, THUNDERBOLT, X/double-X and Z+X stock. Existing
player records are the only mutable owners; new helpers receive the world and
records directly. No allocations or guest ABI dispatch are introduced. The
X history occupies 0x4228..0x42d3, stock state/random pairs 0x3ff0..0x4227 and
FORCE JET's segment history 0x15bc..0x17f3. Reuse, reset and teardown remain with
the original player owner. Transient bounded draw lists preserve the original
weapon/primary/radial/X/player/spark ordering using segment counts.

The shared input path retains previous-mask guards, double-X inventory/energy
restoration, held/release thresholds and stock consumption. Energy exhaustion
permits the triggering attack before switching to base weapon outside recharge
modes. Stock retires both enemy-projectile actors and all eight 0x2ddc records
before its later bidirectional hit phase. X's active word is reset each tick;
held X increments its age until the original cap refuses further activation.

Original-code comparisons check numeric state, RNG, damage, draw order and
retirement across ships/modes and partial/full pools. Full application/palette/
scene parity remains separate. Other selected weapon actions are explicit
boundaries; the shipping target contains no oracle/generated execution fallback.

LIGHTNING uses charge kind 2 in 0x42d4..0x4367. Each activation consumes one
energy unit; the post-foreground update clamps charge to 64, selects the original
threshold/alternate frames and clears the one-frame active flag. Release first
clears its eight flags and target slots, then only charge >10 can spend six units
and activate 0x2da8. That pulse advances/draws/hits only on even world ticks,
retiring after four advances; odd ticks preserve it unchanged. Other charge
kinds remain gated with their unrecovered weapon families (kind 0 is now
connected to NOVA WAVE below).

FIRE STORM's ten 0x2ba0 records retain failed/full-pool energy consumption, the
constructor's translation of its previous rectangle, and current-mount anchoring
in each update. Sustained fire spends energy every call but emits the mirrored
triangular spreads only on even ticks. Original signed table division, scaled
presentation, damage-before-retirement and the 17-update lifetime are preserved.
Both families reuse existing actor collision/spark ownership and teardown. Tests
supply original draw/audio boundaries, not replacement weapon execution. The
app's LIGHTNING pickup fixture explicitly supplies the session unlock prerequisite;
it does not establish natural unlock progression or persistent settings parity.

NOVA WAVE connects charge kind 0 (charge capped at 32) to two 0xbc-byte records
at 0x3e3c/0x3ef8. Release clears charge flags/targets first, refuses if either
wave is active, then spends 12 energy, applies recoil and constructs both with
opposite numeric signs. Each record owns eight trail rectangles and the charge
scale. Its direct triangular angle calculation, movement, seven scaled trail
submissions/hit queries and 32-update retirement match the original. No native
pointers or parallel trail owners are introduced.

S-LASER owns 0x17f4..0x1887, including six mount rectangles. Its initial action
spends energy before filling history; sustain clears cooldown and spends energy
without resetting history. Update shifts those positions and submits 20 segments
for each of six rows, querying damage/impact RNG only for oldest/newest rows,
then clears the one-frame active flag. It runs between FIRE STORM and LIGHTNING;
NOVA runs between radial fire and X. The existing transient draw buffer is now
512 commands to cover the additional 120 S-LASER submissions alongside live
prior effects. World teardown remains the sole resource/pool removal boundary.

THUNDERBOLT connects charge kind 1 and 0x413c40/0x413c80. Eight lock flags,
ages and checked 1-based actor-slot handles occupy the existing 0x42d4 record.
Selection follows registry order, mask exclusions and original duplicate-slot
counting; inactivity, phase >99 or viewport margin retires a flag after its final
marker submission. Handles deliberately follow registry-slot reuse. They never
own an actor or store a host pointer. Reset placeholders use persistent font
sprite metadata so carried locks retire safely after a world restart.

Release spends 24 energy and copies flags/handles into 0x1f0..0xa6b before trying
the existing-active guard; it clears charge state afterward even if construction
is refused. The first update builds the bounded path through selected slots and
an exit goal, preserving eight turn operations and jitter per step, direct 70
damage and exact RNG order. Rectangles/angles then animate independently of
actors for 12 updates. The original counters clamp at 95 path / 7 hit entries,
including the final-store-versus-render-count distinction. Live targets may
retire while the owned cached path continues; no callback remains on them.
THUNDER draws after player effects and before sparks. Direct dispatcher action
6 and its no-op release 8 share this same native construction path.

## Final stages and ending ownership

ISABEL (Stage 4) and RAIYA (Stage 5) use ordinary C phase control and the existing
world actor registry. Constructors attach bodies, turrets, weak points and
presentation overlays using checked slot handles; effects/projectiles share that
registry. Updates, contact/interception, low-health phase changes, score/death,
report and parent retirement are connected. No parallel writable actor mirror or
CPU callback was added. The final paired laser uses the original attachment
Y offsets; shared earlier laser callers retain zero offsets.

The recovered 0x404310 ending is native app control flow: nine typed credit cards,
map palettes/boss images, score-attack insertion at tick 18, BLZ_END.DAR at 192,
FIN after 256 and one session unlock at exit. Card ticks 65..128 repeat before
the final-image interval. The app transactionally replaces its screen archive;
world/player score remains frozen under the same owner until teardown on exit,
F2 or shutdown. Score attack enters Ranking; normal play uses the tea/title path.
No original INI writes or obsolete high-score website launch are performed.
Automated native fixtures cover the ending; full original pixel parity is pending.

## User progress persistence

`BlazeApp` remains the sole writable score/unlock owner. `blaze_progress_save`
passes five scores, their five mode values and an eleven-bit unlock mask synchronously to the platform;
no host structure is serialized. Stage-clear unlock awards save before advancing,
ending score insertion saves at tick 18 and its final unlock saves before exit.
Without a callback, pure game/oracle fixtures retain their in-memory behavior.

Windows updates original `[Enh]` keys in `dat/Blz.ini`, `[Score]` keys in
`dat/BlzScr.ini`, and native `[Input]` keys in `dat/BlzCtrl.ini`, beneath the
executable directory. Each callback touches only its own file. There is no
parallel options snapshot or borrowed app pointer in the platform settings owner.
Both backends use `platform/settings.c` and `controls.c`; the old Win32 profile
writer and duplicated input storage are retired. The shared writer creates an
exclusive temporary, preserves unknown bytes while updating owned integer keys,
flushes/syncs and atomically replaces the file. Unknown keys, weapon/track
definitions and ID survive. ClearMode fields update with their corresponding
scores in the same atomic replacement. Failure removes the temporary and retains
the previous file; errors propagate to the app. Controls saves never rewrite
scores/options, so windowed startup cannot overwrite saved full-screen intent.

The public build seeds the two original-format INIs once into the real root
`dat/` directory beside existing game assets. Test fixtures share immutable files
through hard links (copy fallback) and own their default INIs.
Subsequent builds preserve the local INIs. Invalid/out-of-range numbers are
ignored, and title high score reflects the loaded ranking. Hidden defaults and
`--settings none` disable writes; asset-root INIs remain startup configuration.
`--settings directory` isolates all three writable paths. Fresh-app tests cover
awards, ending ranking/unlock, mixed option writes, unknown-key preservation,
separate controls, full-screen preference retention and locked-file recovery. This
stores progress only, not a resumable campaign or mid-mission state.

The optional `BLAZE_PLATFORM=SDL3` application uses the same `BlazeApp` and game
renderer. A streaming RGBA texture expands the authoritative indexed framebuffer;
no parallel game-state representation exists. SDL owns its window, renderer,
texture and current joystick; disconnect/applied device changes close that joystick.
Keyboard IDs remain the persisted VK format, translated explicitly to SDL keys;
unmapped IDs fail visibly instead of being ignored. SDL does not capture the
mouse or accept unfocused gameplay input. Windowed/letterboxed/centered display
and monotonic `Time` deadlines retain the existing simulation cadence. MIDI and
F10 now use SDL-compatible owners described below; case-aware assets and writable
user paths still block Unix applications. The default Win32 target remains available.

Live Ghidra audit of the remaining legacy keys (2026-10-21):
- `Clear`: 0x401e8c reads it once into application offset 0x98. No subsequent
  consumer or write-back was found in the recovered instructions. Treat it as
  apparently vestigial, not a proven missing unlock or completion mechanism.
- `ClearMode1..5`: startup loads five values at 0x1580. Ending 0x404310 shifts
  them alongside scores, stores current mode (4 on its score-attack route), and
  saves both arrays at 0x40486c. Ranking 0x403540 draws scores and fixed rank
  labels without consuming these modes. Native `ranking_modes` now loads the
  same fields, shifts them with scores and assigns the qualifying current mode.
  The platform writes both arrays atomically to BlzScr.ini. Eleven bounded
  comparisons against original 0x4047a1..0x404809 cover every slot, ties, rejected
  scores and signed limits. Fresh-app reloads and a failed replacement/retry
  verify that the score/mode pair survives together. This closes the metadata
  gap without altering qualification or adding a new ranking display.
- `ID`: references 0x40717c/0x4071b9 belong to 0x406f80's legacy HTML export.
  It expands `dat/blz_scor.dat` into `html/hi_score.html`, reading/persisting an
  identifier for the old submission form. Its only caller is the ending's
  top score-attack result. It has no identified gameplay consumer; the export
  remains deferred. The template was inspected locally, with no server contact.

## Dormant scatter-burst pool and native controls

Player records 0xec..0x1ef (five stride-0x34 slots) are updated by original
0x4139e0 from 0x407740. Ticks 0..31 use angle/(age+8) velocity and alternating
rotated frames; ticks 32..48 discard one RNG roll, draw eight random-position
blended blasts and apply damage 4. The original rectangle is restored each tick
while sprite 0x7d and expansion -2 remain; increment to 49 retires the record.
Native `blaze_scatter_bursts_update` runs after G-BRASTER in the existing effect
stream, before pass-1 actors. There is no new allocation or mirror. Original
constructor/reset references (0x4018c0, 0x416460, 0x416670), update call and shipped
action dispatcher were inspected: no active creator was found. Forced valid
records verify full state, ordered draws, damage/RNG and expiry against canonical
0x4139e0; a world fixture verifies integration/teardown. No weapon action was
invented and the former active-pool blocker is removed by real native behavior.

`windows_input.h` owns platform controls under `WindowSettings`. Keyboard and
WinMM controller samples become the same native game masks. Range-aware stick
thresholds default to 25 percent of half travel, with POV diagonals and bounded
1..32 button bindings. Auto/explicit/off device selection, disconnected-device
retry and focus gating use no mouse capture or background input. Native joystick
restores an original dependency (0x4245c0 uses joyGetPosEx); F10's key-remapping
editor is an enhancement, not a claimed original UI reconstruction.

The Win32 editor borrows the parent HWND and edits one temporary settings value.
Save validates and atomically writes only `dat/BlzCtrl.ini`; Cancel
leaves the applied bindings untouched. A failed write restores prior bindings.
Gameplay uses pause reason 4 while the editor pumps audio/messages; closing it
releases child windows and resets device discovery. Hidden editor fixtures verify
capture/cancel/unbind/defaults/save without focus changes or desktop key injection.
Physical pad compatibility, unplug/replug and interactive editor/audio acceptance
remain manual checks. No controller emulation or unavailable-device success stub
is used; absent devices contribute no actions and keyboard remains usable.

## SDL MIDI ownership

`BlazeSdlMusic` owns one FluidSynth settings/synth pair, one loaded SoundFont,
two sample-timed SMF players and one bounded stereo SDL stream. A single worker
owns those objects and all MIDI I/O/rendering. The main thread submits owned-path
FIFO requests under a short publication lock; it never synthesizes or loads a
song. Loading the inactive slot preserves active playback/EOF. Play/stop or an
active-slot load invalidates old EOF immediately through intent sequence IDs.
Nonloop EOF uses the SMF tick boundary, then waits for SDL's queued input to drain;
it does not add FluidSynth's voice/reverb grace period to menu transitions.
Muted diagnostics parse slots without advancing wall-clock music, as before.

Stop clears queued music and consumes the old player's deferred all-sound-off
before another player can emit notes. Closing signals/joins the worker before
destroying its stream, players, synth and settings; queued commands are released.
PCM has a separate owner/stream and keeps running during MIDI I/O. Game pause
continues asynchronous music/SFX, matching the recovered behavior. CD is explicit
unsupported in this candidate. Win32 retains the independent original MCI worker
for comparison; no SDL media call re-enters it.

## SDL controls editor

`SdlPlatform` owns an embedded `BlazeSdlControls` editor and its temporary controls
value. F10 pauses simulation with reason 4 while the existing SDL event/presentation
loop keeps running. It neither edits the game frame nor creates child windows.
The fixed ASCII pixel menu uses SDL's built-in bitmap glyphs and restores render
scale/blend state after drawing. Mouse positions use the same centered 320x240
transform; keyboard/Tab navigation also works without a pointer or controller.

`sdl_keys.h` maps supported legacy VK-numbered persisted keys both ways; SDL
scancodes never enter the INI. Key capture excludes F10, Esc cancels capture, and
Backspace clears a binding. Pad capture uses only the selected preview device,
with 0..32 manual button selection also available. That preview handle belongs
only to the editor and closes on save/cancel/defaults/device change/exit.
Focus loss or device removal cancels capture; editing never samples game input.

Save atomically writes only BlzCtrl.ini using the existing writer before replacing
the applied value. Failure leaves the editor open and applied bindings intact.
Cancel leaves disk and live bindings untouched. Closing clears only pause reason
4, preserving user/focus pause, and gates held actions until neutral. It keeps
existing keyboard/pad samples rather than manufacturing a neutral state by
resetting/reopening them; applied device changes trigger fresh discovery.
No SDL editor path calls the Win32 controls editor.
