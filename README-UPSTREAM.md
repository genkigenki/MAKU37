<p align="center"><img src="assets/logo.jpg" alt="MAKU" width="360"></p>

# MAKU

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)

**MAKU (幕間機)** is an always-on ambience machine firmware for the M-VAVE FM-1: power it on and a new, random,
in-key world is already playing. Four parts (a pulse, a floor, a haze, a voice), four volume knobs, three macros per
part, and buttons that are verbs (sink, cascade, swell, wobble, lift, scramble, drift). Nothing is saved but your
likes: every boot is a new world, HOME held makes another, and REC marks one you like; the more you like, the more
the new worlds come from your likes (a little changed), so MAKU grows to your taste.

電源を入れた瞬間から、その場限りのアンビエンスが鳴っている、DJイベントの幕間のためのファームウェアです。

**Try it in your browser: [MAKU emulator](https://okumurari2.github.io/maku/)** (GitHub Pages, built from `web/emu/`).

<p align="center"><img src="docs/screenshot-pulse.png" alt="MAKU's screen on the FM-1: part 1 (pulse) has focus; four volume bars, BPM, and the DENS / TONE / WASH macros" width="360"></p>

MAKU is built on [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments), GPL-3.0-only.
The sequencer editing, presets and projects, engine pages, web editor and the other Felucca features are removed (see
[docs/AMBIENT.md](docs/AMBIENT.md), "捨てるもの").

## Install

> Installing replaces the FM-1's firmware: **at your own risk**. Check the way back first (below). Verified on hardware
> by the author; what is still read from the code rather than tested is marked in [docs/SPEC.md](docs/SPEC.md).

1. Download `maku-X.Y.fwsc` from the [Releases](https://github.com/okumurari2/maku/releases) page.
2. Connect the FM-1 over USB and write the package from a computer:
   `python3 tools/fm1_install.py maku-X.Y.fwsc` (needs `pip3 install mido python-rtmidi`; see [BUILDING.md](BUILDING.md)).
   Felucca's own web installer installs Felucca's release, not MAKU. A browser installer that takes any FM-1 `.fwsc`
   (Chrome / Edge), such as the [FM-1 Firmware Hub](https://fm1.designburgapps.com), can write MAKU's package too.
3. **To go back**: the installer's **Return to official V15** (Felucca's web installer has it), or M-VAVE's own updater.
   Both erase your likes.

Build it yourself: [BUILDING.md](BUILDING.md).

## Docs

Start here:

- [docs/RELEASE_0.1.md](docs/RELEASE_0.1.md): release notes of v0.1 (Japanese / English)
- [docs/ABOUT.md](docs/ABOUT.md): the idea, what it does now and what is open (Japanese / English)
- [docs/SPEC.md](docs/SPEC.md): the current spec in one self-contained page (English), and a map of the docs
- [BUILDING.md](BUILDING.md): build and release

Reference and history:

- [docs/AMBIENT.md](docs/AMBIENT.md): the full spec (Japanese; wins on any conflict)
- [docs/VOICES_RATING.md](docs/VOICES_RATING.md): the voice catalog and how it is rated (Japanese)
- [docs/MAKU.md](docs/MAKU.md): design notes of the generator under it (Japanese, written for an earlier form)
- [docs/VERBS_PLAN.md](docs/VERBS_PLAN.md): how the verbs were regrouped
- [docs/decisions.md](docs/decisions.md): one line per decision, newest last
- [docs/UI_RULES.md](docs/UI_RULES.md): the UI rules to keep when building another FM-1 firmware from MAKU (Japanese)
- [docs/HUB_SUBMISSION.md](docs/HUB_SUBMISSION.md): draft text for listing on the FM-1 Firmware Hub

## What it does

- **Four parts**: 1 pulse (kick, DRUM), 2 floor (drone, PHYS), 3 haze (shimmer, GRAIN), 4 voice (arpeggiated phrase, PHYS);
  a boot picks the sounds, root, scale, tempo (about 70-90 BPM) and every macro at random
- **KNOB 1-4** set the volume of parts 1-4 and move the focus to that part; **SELECT / PRESETS / ALGORITHM** are the
  focused part's three macros, each moving several parameters at once, and turning right always makes it livelier
- **Keys** play the focused part's sound, unquantized; every played note also melts into the voice's phrase.
  When the kick part has focus the keys become a 16-step sequencer
- **Upper row, six verbs of change, each a press that lasts a few seconds**: FX sinks (depth), SCL cascades (more notes, higher, a third above), ENV swells
  (longer, the chords stay), LFO wobbles (filter and pitch), EDIT lifts (brighter, harder, thinner), GLO scrambles (the beat and
  the phrase come apart). A press rises (RISE), holds while it is down and for HOLD after, then fades slowly (FADE); the three
  knobs move those times, in beats, while it is held, and the LED shows what is going
- **Lower row, operations that keep**: ARP sets root / scale / register (a tap steps the scale), SEQ sets tempo / swing / duck,
  PLAY is a switch for DRIFT (the values wander on their own; hold it to set RATE / RANGE / REACH), SAVE does nothing;
  OCT- / OCT+ step the key around the circle of fifths
- **HOME** held: a new world, random or (more often the more likes there are, up to 90 %) one of your likes, a little changed.
  HOME + OCT-: the settings menu (colour, screen off, calibration)
- **REC** tapped: a like (the world playing now is kept, up to 16, REC lit); held ~2 s: every like forgotten. Besides the
  settings menu, the likes are the only thing written to flash, and only when REC is pressed (the erase is done ahead of time, so the press itself stops nothing; see docs/UI_RULES.md 10).
  Returning to the official firmware erases the likes; updates between MAKU / Felucca builds should keep them (read from
  the code; not yet verified on hardware). 公式V15に戻す・M-VAVE のアップデータを使うと、いいねは消えます
- **MIDI**: USB and TRS in, clock follow; CC20 is the kick density. USB also carries the stereo audio input

## Layout

| Path | What |
| --- | --- |
| `firmware/` | firmware sources: `src/` app, `hal/` hardware layer, `loader/` update loader |
| `tools/` | build script, generators, package maker, installer and sample uploader |
| `assets/` | UI font, icon names, CC0 instrument samples |
| `web/` | the web installer and the browser emulator (`web/emu/`); Felucca's web editor: [Felucca-WebApp](https://github.com/hugelton/Felucca-WebApp) |
| `tests/` | tests that run on the build machine |
| `LICENSES/` | licence texts of the bundled fonts, icons, ported DSP and SDK files |

## If the FM-1 does not start

If an update is interrupted and the FM-1 stays black, check whether a computer sees it as a USB device named
**WL80UBOOT** (or a USB mass-storage device with ID 4C4A:8057). That is the chip's built-in boot mode, and
the FM-1 can be brought back:

- First try another USB data cable, and close every other app that uses MIDI, then run the installer again.
- If it stays in boot mode, [FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter) reads and
  writes the FM-1's flash from a Mac through a Seeed XIAO RP2040 (three wires to the FM-1's USB lines). Back up
  the flash first, then write the official firmware (M-VAVE's FM-1.fwsc).
- Questions about MAKU: [Issues](https://github.com/okumurari2/maku/issues).

## Contributing

Issues are for reproducible bugs (one per issue). Pull requests are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md).
Felucca itself is supported through [GitHub Sponsors](https://github.com/sponsors/hugelton) and
[itch.io](https://hugelton.itch.io/felucca).

## AI disclaimer

MAKU, like Felucca, is developed with the assistance of AI coding agents, used for coding, testing, documentation,
translation and maintenance. **The MAKU logo (`assets/logo.jpg`) is AI-generated.** MAKU's design and direction are
decided by its maintainer. Felucca states that no generative AI is used to create music, icons, or visual artwork for
it, which does not extend to the MAKU logo; see
[On AI-Assisted Development and Responsibility](https://github.com/hugelton/Felucca/discussions/166).

## Credits

- **[Hügelton Instruments](https://hugelton.com)** (Leo Kuroshita, [@kurogedelic](https://github.com/kurogedelic)):
  Felucca itself, which MAKU is built on; the PHASE engine's waveforms (a C port of the oscillator of
  [CrispyZebra](https://github.com/hugelton/CrispyZebra), GPL-3.0); the DRUM voices and kits; the Hügelton Sample
  Pack (the drum samples, GPL-3.0-only, not CC0); the [Fukiai](https://github.com/hugelton/Fukiai) icon
  font ([MIT](LICENSES/MIT-Fukiai.txt))
- Fonts: [Inter Tight](https://github.com/rsms/inter-tight) by The Inter Project Authors, [SIL OFL 1.1](LICENSES/OFL-InterTight.txt);
  the browser emulator's labels: [DotGothic16](https://github.com/fontworks-fonts/DotGothic16) by The DotGothic16 Project Authors, [SIL OFL 1.1](LICENSES/OFL-DotGothic16.txt)
- Samples: [Versilian Studios](https://versilian-studios.com/) [VSCO-2 Community Edition](https://github.com/sgossner/VSCO-2-CE) and [VCSL](https://github.com/sgossner/VCSL), CC0 1.0: the SAMPLE sets, also SLICE's PIANO ([attribution](assets/samples-cc0/ATTRIBUTION.txt))
- VOICE engine: after [klattsch](https://github.com/tgies/klattsch) by Tony Gies (MIT); formant data from Klatt (1980) and Hillenbrand et al. (1995)
- PHYS engine: models ported from [DaisySP](https://github.com/electro-smith/DaisySP) by Electrosmith and Emilie Gillet ([MIT](LICENSES/MIT-DaisySP.txt)) and from Emilie Gillet's [eurorack](https://github.com/pichenettes/eurorack) code ([MIT](LICENSES/MIT-Rings.txt))
- FM6 engine: msfa from [Dexed](https://github.com/asb2m10/dexed) by Google Inc. and Pascal Gauthier ([Apache-2.0](LICENSES/Apache-2.0-msfa.txt))
- Browser emulator: after [X0X](https://github.com/charlesvestal/fm1-x0x) by [charlesvestal](https://github.com/charlesvestal) (GPL-3.0), a Felucca fork whose browser build showed the way
- Package format and boot files: [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK) ([Apache-2.0](LICENSES/Apache-2.0.txt); three of its files are in every package, none in this tree)
- Contributions: [keremimo](https://github.com/keremimo) (white-key scales, #2), [ChanceTheMaker](https://github.com/ChanceTheMaker)
  (TRS MIDI, bend, sustain and clock, palettes, favourites, editor display settings: #8, #10, #11, #12),
  [andreahaku](https://github.com/andreahaku) (sample recording and trim, #29; SLICE manual slices and tests, #27, #22),
  [spinkham](https://github.com/spinkham) (the boot fix for 0.9 projects, #111),
  [zednaked](https://github.com/zednaked) (ratchets, #100),
  [jasonpersinger](https://github.com/jasonpersinger) (the ROOM reverb click fix, #121)

## Licence

Free software: [GPL-3.0-only](LICENSE), the Hügelton Sample Pack included. The bundled fonts and the
ported DSP keep their own licences ([LICENSES/](LICENSES/)); details in [LICENSING.md](LICENSING.md).

M-VAVE and FM-1 are trademarks of their respective owners. Neither MAKU nor Felucca is affiliated with or endorsed by them.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (Felucca, the original work)
Copyright (C) 2026 okumurari (@okumurari2) (MAKU: the changes made to Felucca, see the git history)
