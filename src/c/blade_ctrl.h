#pragma once
#include <pebble.h>

// ブレードのアニメーション・イベント制御用の設定パラメータ
typedef struct {
  int anim_speed;          // アニメーション速度 (例: 40 = 400ms)
  int anim_freq;           // 自動アニメーション間隔(分)
  int anim_mode;           // 0: Full, 1: Interval Only, 2: Shake Only, 3: Disabled
  int vibe_mode;           // 0: Shake&Chime, 1: Periodic Only, 2: Shake Only, 3: Disabled
  bool is_sleep_mode;      // スリープモード中か（バイブ強制ミュート）
  bool is_biz_mode;        // ビジネスモード表示中か（シェイク・定期アニメ無効化）
  bool has_pending_swap;   // 設定変更やモード切替の保留があるか（アニメモードに関わらず強制変形）
  bool has_pending_biz;    // ビジネスモード切替保留があるか（フォーカス復帰・Tick時の遅延変形判定用）
  bool block_tap_external; // 外部要因（画像チャンク受信中など）によるタップブロック
} BladeAnimConfig;

// ブレードの描画用パラメータ
typedef struct {
  bool is_biz_mode;        // ビジネスモード表示中か
  int biz_blade_color;     // 0: Solid Black, 1: Solid White
  int theme;               // 通常時の背景テーマ (0: White, 1: Black)
  int shutter_color_mode;  // 0..6 (Black, White, Custom, Rainbow, Pastel, Battery Vivid, Battery Pastel)
  GColor custom_color;     // カスタムカラー
  bool battery_color_sync; // バッテリー残量連動ゲージ表示
  int battery_pct;         // バッテリー残量 (0..100)
} BladeRenderConfig;

// メイン側と連携するためのコールバック群
typedef struct {
  // 現在のアニメーション制御パラメータを取得する
  void (*get_anim_config)(BladeAnimConfig *out_cfg);
  // ブレードが完全に閉じた瞬間 (progress >= 1.0) に呼ばれる（時刻更新・設定やモードの裏すり替え用）
  void (*on_closed_midpoint)(void);
  // アニメーションを伴わずに表示情報だけ更新する時に呼ばれる
  void (*on_update_no_anim)(void);
  // 画面フォーカス復帰時に呼ばれる（コンテキストモード再評価用）
  void (*on_focus_gained)(void);
} BladeCallbacks;

// 初期化・終了処理（ウインドウのロード・アンロード時に呼び出す）
void blade_ctrl_init(Layer *canvas_layer, const BladeCallbacks *callbacks);
void blade_ctrl_deinit(void);

// イベント購読の開始・解除（mainの初期化・終了時に呼び出す）
void blade_ctrl_subscribe_events(void);
void blade_ctrl_unsubscribe_events(void);

// シャッターアニメーションを手動(manual=true)または自動(manual=false)でトリガーする
void blade_ctrl_trigger(bool manual);

// 毎分のTickイベントに応じたブレード制御（アニメ発火または通常更新の振り分け）
void blade_ctrl_handle_tick(int tm_min, bool is_chime_time);

// 定期バイブ（時報）を鳴らしつつ、モーター振動による誤タップ検知を700ms間ブロックする
void blade_ctrl_vibe_short_pulse_safe(void);

// 現在アニメーション中かどうかを取得する
bool blade_ctrl_is_animating(void);

// 現在の画面フォーカス状態を取得する
bool blade_ctrl_is_in_focus(void);

// 現在のアニメーション進行度に応じた開口部半径 r を計算する
int blade_ctrl_get_aperture_radius(int screen_w, int shutter_thick);

// 12枚のアイリスブレードを描画する
void blade_ctrl_draw(GContext *ctx, GPoint center, int r, const BladeRenderConfig *render_cfg);
