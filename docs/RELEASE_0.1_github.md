<!-- Paste into the GitHub release body (v0.1). Generated from RELEASE_0.1.md: relative links made absolute. -->
幕間機。M-VAVE FM-1 のためのファームウェア。 / An interlude machine: firmware for the M-VAVE FM-1.

![MAKU on the FM-1: the pulse part has focus](https://raw.githubusercontent.com/okumurari2/maku/main/docs/screenshot-pulse.png)

## 日本語

電源を入れた瞬間から、その場限りのアンビエンスが鳴っています。設定してから鳴らすのではなく、起動したらもう鳴っている。
DJ イベントの転換や幕間に、ささっとつないで音を出すための機械です。

### できること

- **4つの役**：脈（キック）・床（ドローン）・霞（シマー）・声（アルペジオのフレーズ）。起動のたびに、音色・根音・スケール・テンポ（約70〜90 BPM）・つまみの初期値がランダムに決まります。音はつねにスケールの中にあります。
- **つまみ**：KNOB 1〜4 が各役の音量（回した役にフォーカスが移る）。SELECT / PRESETS / ALGORITHM がその役の3つのマクロで、右へ回すほど賑やかになります。
- **変化の動詞**（上段、押している間だけ効く）：FX 潜る / SCL 降り注ぐ / ENV 膨らむ / LFO 揺れる / EDIT 持ち上げる / GLO 崩す。
- **残る操作**（下段）：ARP（根音・スケール・音域）、SEQ（テンポ・スウィング・ダック）、PLAY（DRIFT：値が勝手に漂う）、OCT-/OCT+（五度圏で転調）。
- **HOME 長押し**で新しい世界。**REC** で今の世界を「いいね」に記録（最大16件）。いいねが増えるほど、新しい世界はいいねから（少し変えて）選ばれやすくなり（最大90%）、使う人の好みに育ちます。保存されるのはこのいいねだけです。
- **キー**はフォーカスした役の音を鳴らします（弾いた音は声のフレーズにも溶けます）。脈にフォーカスすると16ステップのキックシーケンサーになります。
- **MIDI**：USB と TRS の入力、クロックに追従、CC20 がキックの密度。USB はステレオのオーディオ入力も運びます。
- **ブラウザで試せます**：[MAKU emulator](https://okumurari2.github.io/maku/)（実機と同じコードを WebAssembly で動かしています）。

### インストール

> ファームウェアの書き換えは**自己責任**です。戻し方を先に確認してください。

1. 下の `maku-0.1.fwsc` をダウンロードします。
2. FM-1 を USB でつなぎ、コンピューターから書き込みます：
   `python3 tools/fm1_install.py maku-0.1.fwsc`（`pip3 install mido python-rtmidi` が必要。詳しくは [BUILDING.md](https://github.com/okumurari2/maku/blob/main/BUILDING.md)）。
   FM-1 Firmware Hub などのブラウザ版インストーラーからも書き込めます（Chrome / Edge）。
3. **戻すには**：インストーラーの「Return to official V15」、または M-VAVE 公式のアップデーター。どちらもいいねは消えます。

途中で止まって画面が真っ黒のままのときは、[README](https://github.com/okumurari2/maku/blob/main/README.md) の "If the FM-1 does not start" を見てください。

### 既知の制限・実機で未確認のこと

実機での動作は作者が確認済みですが、次はコードやホストでの測定から読んだ値で、実機では確かめていません。

- **処理負荷**：最悪の世界で実機の約51%という見積もり（ホストでの測定からの換算）。オーバーロード時は、ボイスを1つずつ減らす保護が働きます。
- **REC の書き込み**：フラッシュ書き込みで音が約40 ms 途切れます。許容できるかは実機で聴いて決める予定です。
- **いいねの保存**：MAKU / Felucca 間の更新では残るはずですが（コードからの判断）、実機では未検証です。
- **SINK の時間**：潜るのは約0.5秒、戻りは約3秒（コードからの値）。
- 起動時ランダムの範囲、いいねの定数（16件・増え方・変異率）は、使いながら耳で詰めていきます。

### ライセンス

GPL-3.0-only。[Felucca](https://github.com/hugelton/Felucca)（Leo Kuroshita / Hügelton Instruments）から、シーケンサー編集・プリセットと保存・音作りページ・Web エディタを捨てて作った派生版です。パッケージには JieLi AC79 SDK（Apache-2.0）のファイルが3つ入っているため、ライセンス文は [`LICENSES/`](https://github.com/okumurari2/maku/blob/main/LICENSES) にあります（パッケージの添付物にも `LICENSE` と `LICENSING.md` を付けています）。ロゴは AI 生成です。M-VAVE・FM-1 は各権利者の商標で、MAKU は関係も承認もありません。

## English

The moment it is powered on, a one-off ambience is already playing. There is no setup step. It is made for the interludes
of DJ events: plug it in and get some sound, fast.

### What it does

- **Four parts**: pulse (kick), floor (drone), haze (shimmer), voice (arpeggiated phrase). Every boot picks the sounds,
  root, scale, tempo (about 70-90 BPM) and every macro at random; notes always stay in the scale.
- **Knobs**: KNOB 1-4 are the volumes of the four parts (and move the focus). SELECT / PRESETS / ALGORITHM are the
  focused part's three macros; turning right always makes it livelier.
- **Six verbs of change** (upper row, work while held): FX sink, SCL cascade, ENV swell, LFO wobble, EDIT lift, GLO scramble.
- **Operations that keep** (lower row): ARP (root / scale / register), SEQ (tempo / swing / duck), PLAY (DRIFT: the values
  wander on their own), OCT-/OCT+ (modulate around the circle of fifths).
- **HOME held** makes a new world. **REC** keeps the world playing as a like (up to 16). The more likes there are, the more
  often a new world comes from them, a little changed (up to 90 %), so MAKU grows to your taste. The likes are all it stores.
- **Keys** play the focused part's sound (and melt into the voice's phrase); with the pulse focused they are a 16-step kick sequencer.
- **MIDI**: USB and TRS in, clock follow, CC20 = kick density. USB also carries the stereo audio input.
- **Try it in the browser**: the [MAKU emulator](https://okumurari2.github.io/maku/) runs the same code as the instrument, compiled to WebAssembly.

### Install

> Installing replaces the FM-1's firmware: **at your own risk**. Check the way back first.

1. Download `maku-0.1.fwsc` below.
2. Connect the FM-1 over USB and write the package from a computer:
   `python3 tools/fm1_install.py maku-0.1.fwsc` (needs `pip3 install mido python-rtmidi`; see [BUILDING.md](https://github.com/okumurari2/maku/blob/main/BUILDING.md)).
   A browser installer such as the FM-1 Firmware Hub (Chrome / Edge) can write it too.
3. **To go back**: the installer's "Return to official V15", or M-VAVE's own updater. Both erase your likes.

If an update is interrupted and the screen stays black, see "If the FM-1 does not start" in the [README](https://github.com/okumurari2/maku/blob/main/README.md).

### Known limits / not yet verified on hardware

Verified on hardware by the author, except for the following, which are read from the code or measured on the host:

- **CPU load**: about 51 % of the device in the worst world (converted from host measurements). Overload protection drops voices one by one.
- **REC write**: the flash write leaves a gap of about 40 ms in the sound; whether that is acceptable is to be decided by ear on the device.
- **Likes surviving an update**: they should survive MAKU / Felucca updates (read from the code), not yet tested on hardware.
- **SINK timing**: about 0.5 s down, about 3 s back (from the code).
- The boot-randomness ranges and the likes' constants (16, growth rate, mutation rate) will be tuned by ear with use.

### Licence

GPL-3.0-only. A derivative of [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita / Hügelton Instruments) with the
sequencer editing, presets and saving, sound-design pages and web editor removed. Every package contains three Apache-2.0
files of the JieLi AC79 SDK, so the licence texts are kept in the repository ([`LICENSES/`](https://github.com/okumurari2/maku/blob/main/LICENSES)). The logo is AI-generated. M-VAVE and FM-1 are
trademarks of their respective owners; MAKU is not affiliated with or endorsed by them.

### Files

| File | What |
| --- | --- |
| `maku-0.1.fwsc` | the installable package (identity `FM-1_901`) |
| `maku-0.1-app.bin` | the app alone |
| `SHA256SUMS` | checksums |
| `ATTRIBUTION.txt`, `LICENSE`, `LICENSING.md` | sample attribution and licence texts (the other licence texts: [`LICENSES/`](https://github.com/okumurari2/maku/blob/main/LICENSES) in the repository) |
