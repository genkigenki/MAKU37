# MAKU37: MAKU on the M-Vave SMK-37

`BOARD=smk37 ./build.sh` builds `build/maku37.fwsc` (identity `SMK-37 Pro_900`). Everything sits behind
`BOARD_SMK37`; the FM-1 build is byte-identical. The decisions: [smk37-port.yaml](smk37-port.yaml). The board layer
(keys, pads, faders, wheels, display, LEDs, flash map, update path) comes from Felucca's SMK-37 target
(hugelton/Felucca#237), following the [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37) guide.

Status: builds and passes the host tests; **not yet tried on a unit**.

## Controls

| SMK-37 | MAKU37 |
| --- | --- |
| K1 | MASTER (output volume) |
| K2 / K3 / K4 | the focused part's three macros (SELECT, PRESETS = TONE, ALGORITHM); a verb held: RISE / HOLD / FADE |
| K5..K8 | volume of parts 1..4 (pulse, floor, haze, voice); the turned part gets the focus |
| top row: ARP, NOTE REPEAT, SCALE, CHORD, GLOBE, BT | the verbs SINK, CASCADE, SWELL, WOBBLE, LIFT, SCRAMBLE |
| PATCH | HOME: held, a new world; with OCTAVE-, the settings menu |
| PARA | the key (MAKU's ARP: tap steps the scale; held, a key sets the root) |
| FX | nothing (MAKU's SAVE) |
| SEQ | held: BPM, SWING, DUCK |
| SEQ PLAY | DRIFT on / off (lit while on); held: RATE, RANGE, REACH |
| SEQ REC | tap: like; held 2 s: every like forgotten |
| OCTAVE- / OCTAVE+ | a fifth down / up |
| KNOB BANK | the settings menu (again: closes it) |
| < / > | the world before (up to 8 back) / a new world |
| FADER BANK | nothing |
| 16 pads | the 16 likes: tap a lit one, its world as it was liked; the pulse focused: its 16 steps |
| faders 1..4 | CASCADE, SWELL, SCRAMBLE as standing amounts (a button press goes on top), SPACE (the space DRIFT adds) |
| pitch wheel | down: SINK, up: LIFT, back at rest when let go |
| mod wheel | WOBBLE, as far as it is up |
| 27 middle keys | as on the FM-1, with velocity |
| 10 outer keys | more notes for the focused part |

TRS MIDI: the SMK's jack is an output, so MIDI clock follows over USB only.

## Flash

MAKU writes only its settings and its likes. On the SMK: settings `0xAD000` / `0xAE000`, likes `0xA9000` / `0xAA000`
(the autosave's pair, as on the FM-1, under their own type), the owner tag `SMK37DATAMAKU` at `0xAF000`; another
firmware's store there (Felucca's too) is cleared, never read. Never written: `0x0-0x3FFF`, `0xC0000-0xC1FFF`
(Bluetooth pairing, the unit's calibration), `0xF4000-0xFDFFF`, `0xFF000-` (allow-list with compile-time checks and a
second guard in the RAM flash driver, `firmware/hal/fm1_flash_map.h`).

Before installing: a full flash backup, and the way back to stock (the guide's `docs/02-update-and-recovery.md`).
