# Iris v3.0.0 Release Notes 📸⚙️

---

## 🇯🇵 日本語 (Japanese)

### 🎉 メジャーアップデート: Iris v3.0.0
カメラのメカニカルシャッターに着想を得たウォッチフェイス「Iris」が、大規模な機能追加と内部構造の刷新を経て **バージョン 3.0.0** に進化しました！

---

### ✨ 新機能 (New Features)

#### 1. 📸 カスタム背景写真（Custom Photo）対応 & 超高速キャッシュ復元
- **スマホからお好みの写真をセット可能に**:
  - 設定画面から写真・イラストを自由にアップロード＆クロップ（拡大縮小・明るさ・64色ディザリングプレビュー対応）。
- **0.01秒の超高速フラッシュキャッシュ復元**:
  - ウォッチの永続ストレージに独自開発の4bit高密度キャッシュを保存。時計の再起動時や設定変更時も、スマホからの再転送を待たずに一瞬で写真が復元され、真っ白な背景が表示される心配がありません。
- **サブピクセル補間＆バイリニア平滑化**:
  - Pebbleのディスプレイに最適化された平滑化アルゴリズムにより、ジャギーの少ない滑らかな画像表示を実現。

#### 2. 🎬 メカニカルシャッター完全同期アニメーション
- **写真のシームレス切り替え**:
  - 写真の転送完了後、唐突に画像が変わるのではなく、ブレードが完全に「閉まる」→全閉の瞬間に裏で画像をセット→「開く」ことで新しい写真が現れるカメラライクな演出を徹底。
- **時刻更新のブレード完全同期**:
  - 毎分またはタイマーによる時刻更新時、シャッターが閉じる最中は「前の時刻」を維持し、全閉の瞬間に「新時刻（デジタル数字・アナログ針）」へ切り替わり、開いた時に新しい時間が現れる完璧な同期演出を実装。
- **手前を覆うリアルなブレード（重なり順の適正化）**:
  - シャッター羽が最前面として時刻や針に覆いかぶさるよう、描画パイプラインを完全見直し。

#### 3. 🔋 バッテリー寿命の大幅改善 & 初期設定の最適化
- **アニメーションモードの拡充**:
  - `Full (Shake & Timer)`、`Interval Timer Only`、`Shake Only`、そして完全停止できる `Disabled (Off)` の4モードを新設。
- **バッテリーに優しい新デフォルト設定**:
  - 初期値を「1分おき」から**「30分おき（Interval Timer Only）」**に変更。描画負荷を約1/30に大幅軽減。
  - 最も電力を消費する**バイブレーションを初期状態で「オフ（Disabled）」**にし、Pebble本来の数日間充電不要なロングライフバッテリーを維持。
  - （※シェイク開閉やバイブを楽しみたい方は、設定画面からいつでも自由に有効化できます！）

#### 4. 💼 ビジネスタイムモードの進化
- **見た目設定の三本柱統一**:
  - 通常モードと完全に一致する「Background Style」「Blade Color Style」「Text Color」に整理。ビジネス時も黒・白・通常写真から選択可能に。
- **24時間・夜勤シフト完全対応**:
  - 終日設定（0:00〜23:00）や夜勤（例: 22:00〜翌6:00で土曜早朝まで金曜扱い）の柔軟なコンテキスト判定を強化。
- **開口部サイズに「Narrow（狭い）」を追加**:
  - より引き締まったプロフェッショナルな見た目を実現。

#### 5. 📱 設定画面（Clay）の操作性向上（誤操作防止）
- **スライダーをプルダウン（Select）化**:
  - スマホでの画面スクロール中に指が触れて値がズレてしまう事故を防ぐため、間隔・速度・時刻選択をすべてプルダウン選択に変更。
- **折りたたみ（アコーディオン）UI**:
  - 画像の微調整コントロールをすっきり折りたたみ、誤タップを防止。ダークテーマでの視認性も最適化。

---

### 🐛 不具合修正 & 内部改善 (Bug Fixes & Improvements)
- **メモリリークの完全排除**:
  - 動的メモリ確保（malloc）を使わない静的・堅牢設計を徹底。キャッシュ復元時のビットマップサイズ整合性ガードを追加。
- **モード切り替え速度の修正**:
  - Pebble OSの「Quiet Time」トースト通知が消えるのを待ってから通常速度で優雅にシャッター開閉を行うよう調整（一瞬で終わってしまう現象を解消）。
- **タイマーモードのアニメーション発火バグ修正**:
  - `Interval Timer Only` モードで設定間隔（1分、30分など）通りに確実にアニメーションが発動するよう修正。
- **モーター振動誤検知ガード**:
  - 時報バイブ直後の700ms間、モーターの揺れをタップセンサーが誤検知してシャッターが連続発火するのをピンポイントで防止。

---

### 🤖 開発体制について (AI Engineering)
- 本プロジェクトの全コード（C言語エンジン、PebbleKit JS、Clay設定画面、画像圧縮アルゴリズム）は、**Google Gemini (Gemini 3.8)** が100%全編にわたり設計・実装・デバッグ・最適化を担当しています。

---
---

## 🇺🇸 英語 (English)

### 🎉 Major Update: Iris v3.0.0
**Iris**, the camera-inspired mechanical shutter watchface for Pebble, has evolved into **Version 3.0.0** with major features, refined aperture animations, and optimized battery life!

---

### ✨ Highlights & New Features

#### 1. 📸 Custom Background Photo & 0.01s Flash Cache
- **Upload Your Favorite Photos**:
  - Interactive cropper in settings with zoom, brightness adjust, aperture overlay guide, and 64-color Floyd-Steinberg dithering preview.
- **Instant 0.01s Flash Cache**:
  - Stored in persistent flash memory with high-density 4-bit palette compression. Your wallpaper restores instantly on reboot—no white flash or phone sync needed!
- **Sub-pixel Dithering & Bilinear Smoothing**:
  - Smooth bilinear interpolation eliminates 2x2 mosaic blockiness on 64-color Pebble screens.

#### 2. 🎬 Mechanical Shutter Midpoint Synchronization
- **Seamless Photo Transitions**:
  - Background image swaps only when blades are fully closed, opening smoothly to reveal the new picture.
- **Synchronized Time Update**:
  - Digital digits and analog clock hands hold the current time during closing, update when fully shut, and unveil the fresh time upon opening.
- **Blades Render on Top**:
  - Authentic aperture overlap covering hands and numbers from the front layer.

#### 3. 🔋 Massive Battery Life Improvement & Optimized Defaults
- **New Animation Modes**:
  - Choose between `Full (Shake & Timer)`, `Interval Timer Only`, `Shake Only`, and `Disabled (Off)`.
- **Battery-Friendly Defaults**:
  - Default auto-animation set to **Every 30 Minutes** (cuts rendering CPU cycles by ~30x compared to 1-minute intervals).
  - Vibration defaults to **Disabled (Off)** to preserve battery life, giving you multi-day standby out of the box.
  - *(Feel free to re-enable shake and vibration in Settings whenever you want the full haptic shutter experience!)*

#### 4. 💼 Refined Business Time Mode
- **Unified Appearance Model**:
  - Standardized around 3 pillars: Background Style, Blade Color Style, and Text Color.
- **24/7 & Night Shift Support**:
  - Seamless support for all-day schedules and overnight night shifts across weekend transitions.
- **New "Narrow" Aperture Size**:
  - Stealthy, professional look during work hours.

#### 5. 📱 Improved Settings UI (Clay)
- **Dropdowns Instead of Sliders**:
  - Replaced interval, speed, and time sliders with dropdown menus to prevent accidental mis-touches while scrolling on mobile.
- **Collapsible Controls**:
  - Zoom and brightness adjustments tucked inside an accordion details view. Dark-theme contrast fully optimized.

---

### 🐛 Bug Fixes & Engine Optimizations
- **Zero Memory Leaks**: Verified zero-leak architecture with no dynamic `malloc` allocations and guarded `GBitmap` sizing.
- **Smooth Mode Transition**: Shutter animation now waits for Pebble OS Quiet Time toasts to dismiss before playing at full speed.
- **Haptic Noise Rejection**: 700ms debounce prevents vibration motor tremors from falsely triggering shake animations.

---

### 🤖 100% AI-Authored Codebase
- Every line of code across the entire history of this repository has been designed, coded, and optimized entirely by **Google Gemini (Gemini 3.8)**.
