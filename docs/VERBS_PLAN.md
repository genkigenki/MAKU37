# 動詞の再編（実装済み）

2026-10-11。ボタンの動詞を整理し直した設計。実装済みで、現行の仕様は AMBIENT.md。この文書は経緯として残す。
実装での差分：上段の LED は点けない。新パラメータの既定は STAY 64 / HARM 48 / PITCH 40 / EDGE 48 / THIN 64（耳で決め直す）。DRIFT の見出しに ON / OFF を出す。

## 方針

- **上段（FX SCL ENV LFO EDIT GLO）＝押している間だけの変化**。6つの動詞を1つずつ置く。離すと戻る。
- **下段（HOME SAVE ARP SEQ PLAY REC）＝残る操作**。世界・調・テンポ・DRIFT のように、触った結果が次の世界まで残るもの。
- KEEP は捨てる。DRIFT は PLAY のトグルにする。BREAK（今の PLAY）は上段の FX へ移す。
- 6つの動詞は**それぞれ別の軸**を1つだけ持ち、同じ資源（送り・LFO・エンベロープなど）を2つの動詞が触らない。

## 6つの動詞の軸

右へ回すほど「その方向へ深く」。重なりを避けるため、各資源の持ち主を1つに決める。

| キー | 動詞 | 軸 | 持ち主になる資源 | パラメータ（SELECT / PRESETS / ALGORITHM） |
|---|---|---|---|---|
| FX | **SINK**（潜る。旧 BREAK） | 奥行き：奥へ沈む | リバーブ・ディレイの送り、逆再生リバーブ、リバーブのフィードバック、キックの音量 | SINK / WASH / HUSH（今の PLAY のまま） |
| SCL | **CASCADE**（降り注ぐ） | 音域と音数：上へ広がる | アルペジオの発音確率、オクターブの重ね、和音の重ね | DENSE / SPAN / HARM |
| ENV | **SWELL**（膨らむ） | 時間：長く伸びる | アタック・リリース、和音が替わる頻度 | ATTACK / RELEASE / STAY |
| LFO | **WOBBLE**（揺れる） | 動き：周期的に揺れる | フィルターLFO、ピッチLFO、コーラス | RATE / DEPTH / PITCH |
| EDIT | **LIFT**（持ち上げる。旧 TWIST の作り直し） | 音色：前へ、明るく、硬く | 全トラックの TONE マクロ、歪み、マスターのハイパス | TONE / EDGE / THIN |
| GLO | **SCRAMBLE**（崩す） | 秩序：拍とフレーズが崩れる | アルペジオの跳躍と再抽選、キックのゴースト、揺れ（SWING） | ARP / KICK / SWING（今のまま） |

対になる組：SINK（奥へ）と LIFT（前へ）、SWELL（ゆっくり・長く）と SCRAMBLE（細かく・乱れる）、CASCADE（音が増える）と
WOBBLE（音は増えず揺れる）。

キーの印字との対応：FX＝エフェクト（空間）、SCL＝スケール（音の並び・音域）、ENV＝エンベロープ、LFO＝LFO、EDIT＝エンジンの
パラメータ（音色）、GLO＝全体（全トラックの拍とグルーヴ）。

### 今の動詞からの変更点

| 動詞 | 変更 |
|---|---|
| SINK | PLAY → FX へ。中身は変えない（約1.5秒で沈み、約3秒で戻る、は上段の共通の立ち上がりより遅いが、SINK の性格なので残す） |
| CASCADE | ARP → SCL へ。**ECHO を外す**（ディレイ・リバーブの送りは SINK の資源）。代わりに **HARM**：アルペジオの音に、スケールの3度上（度数 +2）を確率で重ねる（`maku_note` で求めるので必ず調の中）。0 で重ねない、127 で毎回 |
| SWELL | ENV のまま。**REV を外す**（逆再生リバーブは SINK の資源。SINK は潜るときにすでに逆再生を深める）。代わりに **STAY**：床と霞の次の和音が打たれず、今の和音が鳴り続ける確率（KEEP の HOLD の役を引き継ぐ。持続する声部は既存の `stay_a` / `stay_b` に足す、打つ声部は KEEP の `dc[]` の繰り返しを流用） |
| WOBBLE | LFO のまま。**CHORUS を DEPTH にまとめ**（DEPTH がフィルターLFOの深さとコーラスの送りを両方上げる）、空いた3つ目を **PITCH**：ピッチLFO（`P_LD_PIT`）の深さ。テープの揺れのような音程の揺れ。上限は控えめに（半音の数分の1） |
| LIFT | EDIT のまま、TWIST を作り直して改名。**DENS と WASH を外す**（音数は CASCADE、空間は SINK の資源）。TONE：全トラックの TONE マクロ（PRESETS）への足し分（今の TWIST の TONE と同じ仕組み）、EDGE：キック以外の歪み（`P_DIST`）を上げる、THIN：マスターのハイパス（`perf_k[0]` の正の側）。ハイパスでキックも細くなる、ライザーの効き方 |
| SCRAMBLE | SEQ → GLO へ。中身は変えない |
| KEEP | 捨てる（`kr` `kv` `kw` `kcount` `kn` `kp` `kact` と `maku_arp_keep` のループの部分、`maku_boost` の KEEP の ECHO）。`dc[]` と `dvalid` は SWELL の STAY が使うので残す |
| DRIFT | FX → PLAY のトグルへ（下記） |

### 上段の共通の決まり

- 押している間だけ効き、量は約0.3秒で立ち上がり、約0.6秒で消える（`maku.va`）。SINK だけは今の自前の速さのまま。
- 押している間、画面の3区画がその動詞の3つのパラメータになり、SELECT / PRESETS / ALGORITHM で動かせる。値は次の世界まで残る。
- 複数を同時に押せる。画面とつまみは最後に押したもの。
- LED：押している間と、離して量が消えるまで点く（`va > 0`）。（任意。他の LED と食い違うなら省く）

## 下段

| キー | 役 | 振る舞い |
|---|---|---|
| HOME | 振り直し | 今のまま（長押しで新しい世界、+OCT− で設定） |
| SAVE | （空き） | 機能なし。将来の予約 |
| ARP | **調**（旧 SCL の役） | 今の SCL レイヤーをそのまま移す：タップで MAJ → MIN → LYD … とスケールが巡る。押している間は弾いた鍵が根音、SELECT / PRESETS / ALGORITHM が ROOT / SCALE / REG |
| SEQ | **テンポ**（旧 GLO の役） | 今の GLO レイヤーをそのまま移す：押している間 BPM / SWING / DUCK、黒鍵でミュート、C4 で全解除、F4 でタップテンポ |
| PLAY | **DRIFT のオン・オフ** | 下記 |
| REC | （機能なし） | 今のまま |

ARP を調に、SEQ をテンポにするのは、ARP＝音の並び、SEQ＝時間、の印字に寄せるため。

### PLAY：DRIFT のトグル

- **タップ**（押して離すまでに SELECT / PRESETS / ALGORITHM を回さなかった）で DRIFT がオン ↔ オフ。切り替わるのは離した瞬間。
- **押している間**は画面とつまみが DRIFT の RATE / RANGE / REACH になる（今の FX と同じ表示）。押している間に回したら、離しても
  オン・オフは変わらない（値を変えただけ）。
- **点灯がオンの印**：DRIFT がオンの間、PLAY の LED が点く（緑）。オフで消える。MAKU では常に再生中なので、今の「再生中は緑」
  は使わず、PLAY の LED は DRIFT の状態だけを示す。
- オンにした瞬間の値が錨（`dra`）。オンの間は歩き続け、オフにしても値はその場に残る（今の「離した場所に残る」と同じ）。
- **手で動かした値は錨を取り直す**：DRIFT がオンのまま、マクロを手で回したら、そのマクロの錨は回した値になる。SEQ（テンポ）
  で BPM / SWING / DUCK を変えたら、それらの錨も取り直す。（今は押している間につまみが DRIFT のものなので起きなかったが、
  トグルになると、歩きが手で回した値を RANGE の内へ引き戻してしまう）
- 新しい世界（HOME）でオフに戻すか：**オフに戻す**（世界ごとにまっさらから、の原則）。

## マスターのフィルター（`perf_k[0]`）の取り合い

`perf_k[0]` は負でローパス、正でハイパス。今は DRIFT の空間（`drs`）が負の側に書いている。LIFT の THIN が正の側を使うので、
`perf_k[0] = THIN の量 − drs × 40 / 127` の和にし、−127..127 に収める。DRIFT で暗くなった世界でも、LIFT を押せば明るく細くなる。

## 実装の手引き（ファイル）

- `firmware/src/maku.c`
  - `enum VB`：`VB_KEEP` を外し、`VB_TWIST` を `VB_LIFT` に、`VB_PLAY` を `VB_SINK` に改名。DRIFT は `VB_DRIFT` のまま（パラメータの
    置き場として）。`maku_block` の `for (i = VB_KEEP; ...)` の起点を、上段の最初の動詞に変える（SINK は自前の `dive` なので外す）
  - DRIFT のトグルの状態を1つ（例 `maku.dr_tog`）。`maku_drift` は `hv` の代わりにそれを見る。錨の取り直しの関数（マクロ1つ / テンポ側）
  - `maku_macro_set`：TWIST の足し分を、LIFT の TONE（j = 1）だけにする
  - CASCADE：`maku_boost` の ECHO を外し、`maku_arp_keep`（名前も変える）で HARM の3度を `out->note[]` に重ねる。アルペジオの声の取り分
    （`maku_voice_cap`、2 か 1）を超えないよう、SPAN のオクターブと合わせた数を取り分で切る
  - SWELL：`maku_swell_rvs` を外す（`fx.c` の呼び出しも）。STAY を `stay_a` / `stay_b` と `dc[]` の分岐に足す
  - WOBBLE：`maku_over` の枠を1トラック5つに増やし（`ov[]` を 12 → 15）、`P_LD_PIT` を足す。CHORUS は DEPTH の量で上げる
  - LIFT：EDGE を `maku_over` で `P_DIST`（トラック2〜4）に、THIN を `perf_k[0]` に
  - 世界の既定値（`ui.c` の `maku.vp[...] = ...`）：`VB_PLAY` を `VB_SINK` に。新しいパラメータの既定は、STAY 64、HARM 48、PITCH 40、
    EDGE 48、THIN 64 を目安に、耳で決める
- `firmware/src/ui_input.c`：`maku_buttons` の表を新しい割り当てに（FX→SINK、SCL→CASCADE、ENV→SWELL、LFO→WOBBLE、EDIT→LIFT、
  GLO→SCRAMBLE）。`maku.brk` は FX から。PLAY のタップと押している間の扱い。`maku_verb_now` に PLAY 押下中の DRIFT を足す。
  `ui_leds` で MAKU のときの PLAY の LED を DRIFT の状態に
- `firmware/src/ui_layer.c`：MAKU のときだけ、GLO レイヤーを SEQ ボタン、SCL レイヤーを ARP ボタンで開く。`LAYERS[].btn` を直接
  読んでいる所（`ly_bit`、`layer_btn`）を、MAKU なら置き換えたボタンを返す小さな関数に通す。フッターのキーヒント（`KC_GLO` /
  `KC_SCL` の DONE）も置き換える。`ly_avail` は今のまま（LAYER_GLO と LAYER_SCL だけ）
- `firmware/src/ui_maku.c`：`MAKU_VB` の名前と、KEEP の LEN 表示を外す。キーの印字順の注釈を直す
- テスト：`tests/maku_test.c`（KEEP の試験は外し、HARM の調内、STAY、DRIFT のトグルと錨の取り直し、`perf_k[0]` の和）、
  `tests/ui_test.c` と `tests/ui_render.c`（GLO・SCL の試験はボタンを SEQ・ARP に）
- 文書：AMBIENT.md のボタンの表と「動詞の中身」、README.md の Buttons の行、SPEC.md、decisions.md に1行
- エミュレーター（`web/emu`）：ボタンの見た目の名前は印字のままなので変えない。説明文に動詞名があれば直す

## 決めずに残したこと（耳で決める）

- 新しいパラメータ（HARM / STAY / PITCH / EDGE / THIN）の上限と既定値
- 上段の LED を押している間点けるか
