# MAKU spec (read this first)

A self-contained summary of the current spec, written so a reader with no other context (a chat, a new session) can get
the whole picture from this one file. The full spec is [AMBIENT.md](AMBIENT.md) (Japanese) and wins on any conflict.
Last synced with AMBIENT.md: 2026-10-11.

## What it is

MAKU (幕間機) is firmware for the M-VAVE FM-1 (a 240x240 screen, 4 knobs + master, 2 rows of buttons, a key bed),
derived from Felucca (Leo Kuroshita, GPL-3.0-only). It is an always-on ambience machine for DJ-event interludes: power
it on and a new, random, in-key world is already playing. Nothing is saved but the likes: REC keeps the world playing
now, and every HOME hold then draws a new world, more and more often from the likes, a little changed, so the machine
grows towards its player's taste. Status: verified on hardware by the author; a few specifics (the likes surviving an update, the REC
write's silence, the CPU estimate) are still read from the code or estimated, and are marked as such below.
Names kept from Felucca for compatibility: the USB device name, the loader and package identities, the `FELUCCA_*` build options
and the file names. Release packages are `maku-X.Y.fwsc`, identity `FM-1_9XY` (0.1 -> `FM-1_901`).

## Principles

- It plays from power-on; there is no setup step. The first ~0.8 s swell in from silence (a MASTER knob left up does not shout out of
  the small speaker; the emulator has no swell).
- No saving; boot parameters are random. The one exception: the likes (REC), below.
- Fine control of one parameter has no value; turning a knob should move things in a good direction (right = livelier).
- No page navigation and no menu (only a small settings menu on HOME + OCT-); one button, one verb, an immediate sound.

## The four parts

| # | Part | Content | Steps |
| --- | --- | --- | --- |
| 1 | pulse (脈) | kick, DRUM engine; the PULSE kick crossfades into ECG-monitor beeps as TONE rises (from 80, pure at 110+; from 100 the beep climbs up to +4 semitones with an octave above fading in). Its tuning is fixed, it does not follow ROOT | 16 |
| 2 | floor (床) | drone chords, PHYS (9 voicings of engine + preset + articulation) | 57 |
| 3 | haze (霞) | shimmer, GRAIN (5 voicings) | 13 |
| 4 | voice (声) | arpeggiated phrase, PHYS (10 voicings); 32-place phrase table with rests, random runs of 3..8 | 29 |

A boot picks the voicings, root, scale, tempo (about 70-90 BPM) and every macro at random. Step counts 57 / 13 / 29 are
coprime and none is a multiple of 16, so the parts keep drifting out of phase. Notes are always in the scale
(`maku_note()` only returns scale degrees). The kick ducks the other parts (depth set by DUCK).

Articulations (set per world, `maku_voices.c`): STRK struck, SOFT soft strike, SWEL swell (~2 s, held to the next note),
BOW bowed, HOLD (~1.4 s, held until the chord changes). The floor and haze are held (tie) in most worlds and at least one
of them always is. Voice shares per part: floor 3 (a held PHYS floor 2), held haze 2, voice 2, kick 1 (8 voices total).
Above kick density 96 the shares change to make room for the hats: kick 2, floor 2, voice 1.

## Controls

- **KNOB 1-4**: volume of parts 1-4, and the turned part gets the focus. **MASTER**: output volume (hardware).
- **SELECT / PRESETS / ALGORITHM**: the focused part's three macros, each moving several parameters; right = livelier.

| Part | SELECT | PRESETS (tone) | ALGORITHM |
| --- | --- | --- | --- |
| 1 pulse | DENS (kick density) | TONE | WASH |
| 2 floor | OPEN (chord voices 2-4; below 64: let the chord ring, up to 70 % for a held floor, a struck chord is left out up to 55 %) | TONE | SWAY |
| 3 haze | GRAIN (density / size; below the middle: sparser, up to 90 % of the notes spared) | TONE | AIR |
| 4 voice | LOOSE (re-rolls, octave up; below the middle: sparser, up to 65 %) | TONE | TRAIL |

- **Keys** play the focused part's sound, unquantized (not snapped to the scale). A key just played is also the arp's next
  note, and every played note melts into the voice's phrase (the latest 8 are remembered and fade, an out-of-scale note twice as fast; each phrase step
  plays one of them with 35 %). When the pulse has focus the keys are a 16-step kick sequencer
  (white keys = steps, ACC = accent; the black keys are eight noise pads). Hand-set kicks always play; DENS only adds ghosts. While SEQ is held the keys keep their old jobs: black keys mute, C4
  unmutes all, F4 taps the tempo.
- **Kick density** (DENS): 4-39 a hint on step 1, 40-67 step 9 joins, 68-95 steps 5 and 13, **96+ four on the floor is
  certain**; above 96 hats are added on the same DRUM lanes (open hat on the off-beats from 96, closed hat from 104; with the PULSE kick at TONE 95+ these hats are high sine pips,
  not cymbals).
- **HOME**: held = a new world (random, or from the likes: below). HOME + OCT-: settings menu (colour, LEDS, screen off,
  calibration); HOME closes it. LEDS is AUTO by default: DIM LO on the battery, DIM HI while a USB host is connected.
  On the battery at 1 bar of 3 or less the top left says BATTERY LOW (0 bars: BATTERY EMPTY) for ~2 s, after the level has held for 5 s, again
  at once on dropping a bar and every 2 minutes; nothing on the screen otherwise. Where each of these is in the code: `grep -rn MANNER: firmware`
  (docs/UI_RULES.md 10).

### Upper row: six verbs of change (a press rises, holds while down and for HOLD, then fades; RISE / HOLD / FADE in beats on SELECT / PRESETS / ALGORITHM while held; how far each goes is fixed)

| Button | Verb | Axis | What it does | Parameters |
| --- | --- | --- | --- | --- |
| FX | SINK 潜る | depth | kick fades out, reverb / delay / reverse sends deepen, reverb feedback towards 0.995, dry voices step back; defaults dive ~0.4 s, back ~3 s after HOLD | RISE / HOLD / FADE |
| SCL | CASCADE 降り注ぐ | range, note count | the arp plays every step (rests too), octave +1 / +2 and a third above stack | RISE / HOLD / FADE |
| ENV | SWELL 膨らむ | time | the floor and the haze strike their chord again; attack / release up towards their top; held voices keep the chord, struck ones repeat it | RISE / HOLD / FADE |
| LFO | WOBBLE 揺れる | motion | filter LFO faster and deeper, chorus send up, pitch wobble | RISE / HOLD / FADE |
| EDIT | LIFT 持ち上げる | timbre | all TONE macros to the top, distortion (not the kick), master high-pass up | RISE / HOLD / FADE |
| GLO | SCRAMBLE 崩す | order | the arp jumps around and stutters (x2..x4), more re-rolls, kick ghosts and rolls, beats left out, more swing | RISE / HOLD / FADE |

Pairs: SINK vs LIFT, SWELL vs SCRAMBLE, CASCADE vs WOBBLE. A verb's amount rises over RISE (1/4 .. 8 beats), stays full while held and for HOLD (0, 1/2 .. 32 beats) after, then fades over FADE (2 .. 32 beats, an S curve); the beat is the song tempo. The times stay across worlds (not across power-off); the upper row's LEDs are lit while a verb goes and breathe while it fades. docs/VERBS_TUNE_PLAN.md.

### Lower row: operations that keep

| Button | What it does |
| --- | --- |
| ARP | tap steps the scale MAJ MIN LYD DOR MIX PHRY PEN MPEN; held, a pressed key sets the root and the knobs are ROOT / SCALE / REG (drone register) |
| SEQ | held: SELECT = BPM (follows MIDI clock), PRESETS = SWING, ALGORITHM = DUCK (64 = default dip) |
| OCT- / OCT+ | modulate a fifth down / up around the circle of fifths (a key-signature change: notes already sounding or in the phrase keep their pitch) |
| PLAY | tap = DRIFT on/off (green LED while on); held: RATE / RANGE / REACH. While on, the macros random-walk (small steps, every ~64 blocks, so values glide) within RANGE/2 of where it was switched on, except the kick's DENSITY, which never drifts; REACH 43+ also the space, 86+ also tempo / SWING / DUCK. A value turned by hand becomes the new anchor; a new world switches it off |
| SAVE | no function |
| REC | tap = like: the world playing now goes into the likes (REC lit until the next world); held ~2 s = every like forgotten |

MIDI: USB and TRS in, clock follow only; CC20 (any channel) is the kick density. USB also carries the stereo audio input.

## Likes: a machine that grows to taste (`firmware/src/maku_fav.c`)

- **The ring**: up to 16 likes (`MAKU_FAV_MAX`), oldest first; a 17th pushes the oldest out. A like holds what
  `maku_world` decides and nothing else: the seed, the floor / haze / voice (by the catalog's stable ids), ROOT, SCALE,
  BPM, the 12 macros and the floor's and haze's INSERT (type, RATE, DEPTH, FDBK), as they are when REC is pressed
  (knobs turned, DRIFT's wandering, a key changed by ARP or OCT included).
- **A new world** (HOME held) with N likes: with r = min(90 %, 15 % x N) (`MAKU_FAV_STEP`, `MAKU_FAV_CAP`) one of the
  likes, weighted by recency (1 .. N, oldest to newest), else a fresh random world. N = 0: exactly the old behaviour
  (the same seed, the same world, bit for bit). The screen says FROM A LIKE or NEW WORLD.
- **Mutation**: each part of the chosen like (the three voices, ROOT, SCALE, each of the 12 macros) is drawn anew with
  ~8 % (`MAKU_MUT_P`), from the catalog's weights and the macros' ranges a new world uses; BPM and the INSERTs stay.
  REC on a mutated world makes it a like of its own.
- The choice and the mutation run only on a re-roll (main loop, never the ISR) and draw from a stream derived from the
  seed, so the same likes and the same seed give the same world. Boot (and the emulator's `#seed=N`) stays random.
- **Stable ids**: the catalog is append-only; every row has an id that never changes meaning. A changed sound or weight
  is a new row with a new id (an old row is retired with weight 0 and still plays from a like). `MAKU_CATALOG_VERSION`
  is only the rating log's key. A stored like with an unknown id or an out-of-range value is skipped; the others stay.
- **Storage**: the device writes flash only when REC is pressed (a like or the clearing): its own storage object in the
  former autosave's A/B sectors (0xE5000 / 0xE6000): four page writes per press into a copy erased ahead of time (at power-on and at the next new world: the 4 KiB erase stops the audio for ~40 ms, so it is never done on the press unless REC is pressed again before either; then it is inside a short fade dip, audio.c SOFT),
  the two sectors taking turns (~200,000 presses at 100,000 cycles). The blob has a magic, version and checksum; a damaged
  or foreign one starts empty. Kept across Felucca / MAKU updates; lost when returning to the official V15. The emulator
  keeps it with its simulated flash in localStorage. Ships empty.

## Screen

240x240 split into four 120x120 quadrants matching the knobs' physical 2x2: top-left (MASTER's place) = waveform + BPM with the
key (ROOT SCALE) beside it + four numbered, per-channel-coloured volume bars + the focused part's number and name; the other three show the name, value and gauge of what SELECT / PRESETS / ALGORITHM
move now (the verb's own three while a verb is held). At most three parameters per mode, no paging.

## Sound design notes

Space (all in `fx.c`): ensemble chorus, tape delay, ROOM reverb with pre-delay and moving combs, SPRING, reverse reverb
(amount from the haze's AIR). INSERT (Felucca 1.5): a per-world PHASR / CHOR (floor) or FLANG / CHOR (haze), kept on trial.
Pitch levels: the floor and the haze each draw STILL 70 % / SWAY 22 % / WARP 8 % (never both WARP), which bounds the haze
GRAIN's RAND and a swept INSERT's DEPTH and picks the INSERT (AMBIENT.md).
CPU estimate: worst case ~51 % of the device (host-derived, not measured); overload protection drops voices.

## Kept / dropped

- Dropped: sequencer editing, saving (presets, projects, RESTORE LAST), the sound-editing pages and most engines (FM6,
  SAMPLE, SLICE, LOFI, WHEEL, VOICE, TRIO, PHASE, NOISE), the menu, the web editor and its protocol.
- Kept: the generator, the 4-track mixer, 8 voices, ducking, the space effects and limiter, MIDI clock in, the firmware
  update path (needed to return to the official firmware), the browser emulator.

## Open

Boot-randomness ranges (by ear), the likes' constants (16, 15 %, 90 %, 8 %, 2 s; by ear) and whether the dip of a second REC
before any new world is acceptable (on hardware), ducking between all four volumes, ghost kicks shown dimly on the LEDs, removing unused
engines from the build (~28 KB of flash, but ui_test.c depends on them), and everything on real hardware.

## Where things are

| Topic | File |
| --- | --- |
| The full spec: controls, verbs, screen, kept / dropped | [AMBIENT.md](AMBIENT.md) (Japanese) |
| Generator design (kick rungs, arp table, ducking); written for the earlier "mode" form, AMBIENT.md wins | [MAKU.md](MAKU.md) |
| How the verbs were regrouped (history) | [VERBS_PLAN.md](VERBS_PLAN.md) |
| Voice catalog, articulations, rating tool | [VOICES_RATING.md](VOICES_RATING.md) |
| Decisions, newest last | [decisions.md](decisions.md) |
| Code | `firmware/src/maku.c`, `maku_voices.c`, `maku_fav.c` (likes), `ui_maku.c`, `ui_input.c`, `fx.c` |
| Tests / build | `tests/maku_test.c`, `tests/run_tests.sh`; [../BUILDING.md](../BUILDING.md) |
| Browser emulator (with a -2..+2 rating bar that logs the sounding state; `#seed=N` replays a world) | `web/emu/`; live at https://okumurari2.github.io/maku/ |

When the spec changes, update AMBIENT.md first, then this page (and the "Last synced" date above).
