# MAKU とは / About MAKU

幕間機。M-VAVE FM-1 のためのファームウェア。 / An interlude machine: firmware for the M-VAVE FM-1.

## 日本語

### 思想

DJ イベントの転換や幕間に、ささっとつないで、とりあえずの音を出す機械。**電源を入れた瞬間から、その場限りのアンビエンスが鳴っている。**

- 設定してから鳴らすのではない。起動したらもう鳴っている。
- 保存しない。起動のたびにランダムな「世界」（音色・根音・スケール・テンポ・つまみの初期値）から始まる。
- 1つのパラメータに細かく触れられることに価値はない。つまみは「回せば良い方向に動く」ことだけを約束する（右へ回すほど賑やかになる）。
- 画面遷移もメニューもない。ボタン1つが動詞1つで、押せばすぐに音で返ってくる。
- 外れない。音はつねにスケールの中にあり、4つの役は周期をずらしてあるので、ずっと聴いていても繰り返しに気づきにくい。

### 現在の実装（v0.1）

- **4つの役**：脈（キック）・床（ドローン）・霞（シマー）・声（アルペジオのフレーズ）。KNOB 1〜4 が音量で、回した役にフォーカスが移る。SELECT / PRESETS / ALGORITHM がその役の3つのマクロ。
- **変化の動詞**：上段の6つ（潜る・降り注ぐ・膨らむ・揺れる・持ち上げる・崩す）は、押している間だけ効く。下段は残る操作（調・スケール、テンポ、漂う、五度圏での転調）。
- **好みが育つ**：REC で気に入った世界を「いいね」に記録（最大16件）。HOME 長押しの振り直しは、いいねが増えるほど、いいねから（少し変えて）選ばれる。使う人ごとに、その人の好みに育つ。保存されるのはこのいいねだけ。
- **音**：物理モデル（PHYS）と粒状（GRAIN）の声を、打つ・柔らかく打つ・ふくらむ・擦る・置く、の出し方と組にしたカタログ。コーラス、テープ・ディレイ、ROOM リバーブ、逆再生リバーブ。
- **ブラウザで試せる**：エミュレータは実機と同じコードを WebAssembly で動かす。
- 実機で動作確認済み。Felucca（GPL-3.0-only）から、シーケンサー編集・保存・音作りページ・Web エディタを捨てて作った派生版。

### これから（未決・構想）

- 起動時ランダムの範囲を、耳で詰める。
- 4つの音量を同時に上げたとき、音同士が避け合う動き（ダッキングの拡張）。
- キックのゴーストを LED で暗く見せる。
- いいねの定数（16件・増え方・変異率）を、使ってみて決める。REC を続けて押したときの凹み（約40 ms をフェードで包む）の許容を実機で確かめる。
- 使わない音源の削除（フラッシュが約28 KB空く。テストの整理が先）。
- 実機での CPU 負荷の測定。

## English

### Philosophy

A machine for DJ-event interludes: plug it in and get some sound, fast. **The moment it is powered on, a one-off ambience is already playing.**

- You do not set up and then play; it plays from power-on.
- Nothing is saved. Every boot starts from a random "world" (sounds, root, scale, tempo, every macro).
- Fine control of one parameter has no value. A knob promises only that turning it moves things in a good direction (right = livelier).
- No page navigation, no menu. One button is one verb, and it answers with sound at once.
- Hard to get wrong. Notes always stay in the scale, and the four parts run on lengths that drift out of phase, so the loop is hard to notice.

### What it does now (v0.1)

- **Four parts**: pulse (kick), floor (drone), haze (shimmer), voice (arpeggiated phrase). KNOB 1-4 are their volumes and move the focus; SELECT / PRESETS / ALGORITHM are the focused part's three macros.
- **Verbs of change**: the upper row's six (sink, cascade, swell, wobble, lift, scramble) work while held. The lower row keeps things: key and scale, tempo, drift, modulation around the circle of fifths.
- **Growing to taste**: REC keeps the world playing as a like (up to 16). Each HOME-held re-roll draws more often from your likes, a little changed, the more you have. Everything it stores is these likes.
- **Sound**: a catalog of physical-model (PHYS) and granular (GRAIN) voices, each paired with an articulation (struck, soft, swell, bowed, held); chorus, tape delay, ROOM reverb and a reverse reverb.
- **Try it in the browser**: the emulator runs the same code as the instrument, compiled to WebAssembly.
- Verified on hardware. A derivative of Felucca (GPL-3.0-only) with the sequencer editing, saving, sound-design pages and web editor removed.

### Future work (open questions)

- Tune the boot-randomness ranges by ear.
- Let the parts duck each other when all four volumes are up.
- Show the kick's ghost notes dimly on the LEDs.
- Settle the likes' constants (16, growth rate, mutation rate) by use; check on hardware that the dip of a second REC before any new world (~40 ms inside a fade) is acceptable.
- Remove the unused engines (about 28 KB of flash; the tests need tidying first).
- Measure the CPU load on the device.

Details: [SPEC.md](SPEC.md) (English), [AMBIENT.md](AMBIENT.md) (Japanese), [decisions.md](decisions.md).
