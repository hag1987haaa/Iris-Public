#include "blade_ctrl.h"

#define FRAME_RATE_MS 33

static Layer *s_canvas_layer = NULL;
static BladeCallbacks s_callbacks = {0};

static float s_progress = 0.0f;
static bool s_is_animating = false;
static bool s_vibrate_on_close = false;
static bool s_in_focus = true;

// 定期バイブ（インターバルチャイム）直後のモーター振動ノイズによるタップ誤検知フラグ
static bool s_block_tap_from_vibe = false;
static AppTimer *s_unblock_tap_timer = NULL;
static AppTimer *s_anim_timer = NULL;
static AppTimer *s_delayed_shutter_timer = NULL;

static GPathInfo s_path_info = {
  .num_points = 4,
  .points = (GPoint[4]){{0, 0}, {0, 0}, {0, 0}, {0, 0}}
};
static GPath *s_dynamic_path = NULL;

static void unblock_tap_cb(void *data) {
  s_block_tap_from_vibe = false;
  s_unblock_tap_timer = NULL;
}

static GPoint get_inner_vertex(int i, int r, int32_t angle, GPoint center) {
  int32_t theta = (i * TRIG_MAX_ANGLE / 12) + angle;
  return (GPoint){
    center.x + (r * cos_lookup(theta) / TRIG_MAX_RATIO),
    center.y + (r * sin_lookup(theta) / TRIG_MAX_RATIO)
  };
}

static GPoint get_ray_far_point(int i, int r, int32_t angle, GPoint center) {
  GPoint v = get_inner_vertex(i, r, angle, center);
  int32_t ray_angle = ((i * TRIG_MAX_ANGLE / 12) + angle) + (5 * TRIG_MAX_ANGLE / 24);
  return (GPoint){
    v.x + (400 * cos_lookup(ray_angle) / TRIG_MAX_RATIO),
    v.y + (400 * sin_lookup(ray_angle) / TRIG_MAX_RATIO)
  };
}

static GColor get_wing_fill_color(int wing_index, const BladeRenderConfig *cfg) {
  // ビジネスモード用の羽の色強制上書き
  if (cfg->is_biz_mode) {
    return (cfg->biz_blade_color == 1) ? GColorWhite : GColorBlack;
  }

  int pct = cfg->battery_pct;
  int seq = (8 - wing_index + 12) % 12;
  bool is_empty = false;

  if (cfg->shutter_color_mode != 5 && cfg->shutter_color_mode != 6 && cfg->battery_color_sync) {
    int full_blades = (pct >= 50) ? 12 : (pct * 12) / 50;
    if (seq < (12 - full_blades)) is_empty = true;
  }

  if (is_empty) return (cfg->theme == 0) ? GColorLightGray : GColorDarkGray;

  switch (cfg->shutter_color_mode) {
    case 0: return GColorBlack;
    case 1: return GColorWhite;
    case 2: return cfg->custom_color;
    case 3: {
      static const GColor rainbow[12] = {
        GColorYellow, GColorIcterine, GColorKellyGreen, GColorIslamicGreen,
        GColorCyan, GColorBlue, GColorDukeBlue, GColorPurple,
        GColorMagenta, GColorRed, GColorOrange, GColorChromeYellow
      };
      return rainbow[wing_index];
    }
    case 4: {
      static const GColor pastel[12] = {
        GColorPastelYellow, GColorRajah, GColorInchworm, GColorMintGreen,
        GColorCeleste, GColorBabyBlueEyes, GColorElectricBlue, GColorLavenderIndigo,
        GColorRichBrilliantLavender, GColorBrilliantRose, GColorMelon, GColorChromeYellow
      };
      return pastel[wing_index];
    }
    case 5:
      if (pct > 80) return GColorIslamicGreen;
      else if (pct > 50) return GColorKellyGreen;
      else if (pct > 25) return GColorYellow;
      else if (pct > 5) return GColorOrange;
      else return GColorRed;
    case 6:
      if (pct > 80) return GColorMintGreen;
      else if (pct > 50) return GColorInchworm;
      else if (pct > 25) return GColorPastelYellow;
      else if (pct > 5) return GColorMelon;
      else return GColorBrilliantRose;
    default:
      return GColorBlack;
  }
}

static void next_frame_handler(void *data) {
  s_anim_timer = NULL;
  if (!s_is_animating) return;

  BladeAnimConfig cfg = {0};
  if (s_callbacks.get_anim_config) {
    s_callbacks.get_anim_config(&cfg);
  }

  float old = s_progress;
  float total_duration_ms = (float)cfg.anim_speed * 10.0f;
  if (total_duration_ms < 100.0f) total_duration_ms = 100.0f;
  float frames_needed = total_duration_ms / (float)FRAME_RATE_MS;
  float step = 2.0f / frames_needed;
  if (step < 0.05f) step = 0.05f;
  s_progress += step;

  if (old < 1.0f && s_progress >= 1.0f) {
    if (s_callbacks.on_closed_midpoint) {
      s_callbacks.on_closed_midpoint();
    }
  }

  if (old < 0.9f && s_progress >= 0.9f && s_vibrate_on_close) {
    vibes_short_pulse();
  }
  if (s_progress >= 2.0f) {
    s_progress = 0.0f;
    s_is_animating = false;
    s_vibrate_on_close = false;
  } else {
    s_anim_timer = app_timer_register(FRAME_RATE_MS, next_frame_handler, NULL);
  }
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

void blade_ctrl_trigger(bool manual) {
  if (s_is_animating) return;

  BladeAnimConfig cfg = {0};
  if (s_callbacks.get_anim_config) {
    s_callbacks.get_anim_config(&cfg);
  }

  // アニメーションモード判定（ビジネスモード切り替えや設定変更時は強制的にアニメーション許可）
  if (!cfg.has_pending_swap) {
    // 3: Disabled (一切アニメーションしない)
    if (cfg.anim_mode == 3) return;
    // 1: Interval Only (手首シェイクでは動かさない)
    if (manual && cfg.anim_mode == 1) return;
    // 2: Shake Only (タイマー等の自動では動かさない)
    if (!manual && cfg.anim_mode == 2) return;
  }

  s_is_animating = true;
  s_progress = 0.0f;

  // 手首シェイク時のバイブ制御（スリープモードなら絶対鳴らさない！）
  if (manual && !cfg.is_sleep_mode && (cfg.vibe_mode == 0 || cfg.vibe_mode == 2)) {
    s_vibrate_on_close = true; // 0: Shake&Chime, 2: Shake Only
  } else {
    s_vibrate_on_close = false;
  }

  if (s_anim_timer) app_timer_cancel(s_anim_timer);
  s_anim_timer = app_timer_register(FRAME_RATE_MS, next_frame_handler, NULL);
}

static void handle_tap(AccelAxisType axis, int32_t direction) {
  // 定期バイブ（インターバルチャイム）のモーター振動ノイズならブロック
  if (s_block_tap_from_vibe) return;

  // 手首を振った瞬間にも最新のコンテキスト（Quiet Time状態など）を再評価！
  if (s_callbacks.on_focus_gained) {
    s_callbacks.on_focus_gained();
  }

  BladeAnimConfig cfg = {0};
  if (s_callbacks.get_anim_config) {
    s_callbacks.get_anim_config(&cfg);
  }

  // モード切り替え（ビジネス ⇔ 通常）の保留があるなら、シェイクで即シャッター変形！
  if (cfg.has_pending_biz) {
    blade_ctrl_trigger(false);
    return;
  }

  // ビジネスモード中はシェイクアニメーションを無視（真面目な時計に徹する）
  if (cfg.is_biz_mode) return;

  // 画像チャンク受信中などの外部ブロック条件
  if (cfg.block_tap_external) return;

  blade_ctrl_trigger(true);
}

// 遅延シャッター関数（トースト通知が消えた直後に起動）
static void delayed_shutter_cb(void *data) {
  s_delayed_shutter_timer = NULL;
  BladeAnimConfig cfg = {0};
  if (s_callbacks.get_anim_config) {
    s_callbacks.get_anim_config(&cfg);
  }
  if (cfg.has_pending_biz) {
    blade_ctrl_trigger(false);
  }
}

// フォーカス監視ハンドラ
static void focus_handler(bool in_focus) {
  s_in_focus = in_focus;

  if (in_focus) {
    if (s_callbacks.on_focus_gained) {
      s_callbacks.on_focus_gained();
    }

    BladeAnimConfig cfg = {0};
    if (s_callbacks.get_anim_config) {
      s_callbacks.get_anim_config(&cfg);
    }

    // モード切り替えがある場合、ポップアップが消えたら間髪を容れずスッと変形（150ms）
    if (cfg.has_pending_biz) {
      if (s_delayed_shutter_timer) app_timer_cancel(s_delayed_shutter_timer);
      s_delayed_shutter_timer = app_timer_register(150, delayed_shutter_cb, NULL);
    }
  }
}

void blade_ctrl_init(Layer *canvas_layer, const BladeCallbacks *callbacks) {
  s_canvas_layer = canvas_layer;
  if (callbacks) {
    s_callbacks = *callbacks;
  } else {
    s_callbacks = (BladeCallbacks){0};
  }
  if (!s_dynamic_path) {
    s_dynamic_path = gpath_create(&s_path_info);
  }
}

void blade_ctrl_deinit(void) {
  if (s_unblock_tap_timer) {
    app_timer_cancel(s_unblock_tap_timer);
    s_unblock_tap_timer = NULL;
  }
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
  if (s_delayed_shutter_timer) {
    app_timer_cancel(s_delayed_shutter_timer);
    s_delayed_shutter_timer = NULL;
  }
  if (s_dynamic_path) {
    gpath_destroy(s_dynamic_path);
    s_dynamic_path = NULL;
  }
  s_canvas_layer = NULL;
}

void blade_ctrl_subscribe_events(void) {
  accel_tap_service_subscribe(handle_tap);
  app_focus_service_subscribe(focus_handler);
}

void blade_ctrl_unsubscribe_events(void) {
  app_focus_service_unsubscribe();
  accel_tap_service_unsubscribe();
}

void blade_ctrl_handle_tick(int tm_min, bool is_chime_time) {
  BladeAnimConfig cfg = {0};
  if (s_callbacks.get_anim_config) {
    s_callbacks.get_anim_config(&cfg);
  }

  // アニメーション制御
  if (cfg.has_pending_biz) {
    if (s_in_focus) {
      blade_ctrl_trigger(false); // 見えてる時だけ！
    } else if (s_callbacks.on_update_no_anim) {
      s_callbacks.on_update_no_anim(); // 見えない時は裏で時間だけ進めて「おあずけ」！
    }
  } else if (!cfg.is_biz_mode) {
    bool should_animate = false;

    // モード0 (Full): anim_freq間隔、またはインターバルチャイム時
    if (cfg.anim_mode == 0) {
      if ((cfg.anim_freq > 0 && tm_min % cfg.anim_freq == 0) || is_chime_time) {
        should_animate = true;
      }
    }
    // モード1 (Interval Only): インターバルタイマー時のみアニメーション
    else if (cfg.anim_mode == 1 && is_chime_time) {
      should_animate = true;
    }

    if (should_animate && s_in_focus) {
      blade_ctrl_trigger(false);
    } else if (s_callbacks.on_update_no_anim) {
      s_callbacks.on_update_no_anim();
    }
  } else {
    if (s_callbacks.on_update_no_anim) {
      s_callbacks.on_update_no_anim();
    }
  }
}

void blade_ctrl_vibe_short_pulse_safe(void) {
  vibes_short_pulse();
  // モーター振動の誤検知だけをピンポイントで防ぐ（700msだけタップをブロック）
  s_block_tap_from_vibe = true;
  if (s_unblock_tap_timer) {
    app_timer_cancel(s_unblock_tap_timer);
  }
  s_unblock_tap_timer = app_timer_register(700, unblock_tap_cb, NULL);
}

bool blade_ctrl_is_animating(void) {
  return s_is_animating;
}

bool blade_ctrl_is_in_focus(void) {
  return s_in_focus;
}

int blade_ctrl_get_aperture_radius(int screen_w, int shutter_thick) {
  float ev = (s_progress > 1.0f) ? (2.0f - s_progress) : s_progress;
  int max_r = screen_w / 2;
  int base_r;

  if (PBL_IF_ROUND_ELSE(true, false)) {
    int wide_r = max_r - (max_r / 9);
    int step = max_r / 7;
    if (shutter_thick == 0) base_r = (int)(wide_r * 0.9659f);
    else if (shutter_thick == 1) base_r = (int)((wide_r - step) * 0.9659f);
    else base_r = (int)((wide_r - step * 2) * 0.9659f);
  } else {
    #if defined(PBL_PLATFORM_EMERY)
      if (shutter_thick == 0) base_r = 110;
      else if (shutter_thick == 1) base_r = 94;
      else base_r = 76;
    #else
      if (shutter_thick == 0) base_r = 81;
      else if (shutter_thick == 1) base_r = 68;
      else base_r = 55;
    #endif
  }

  return (int)(base_r / 0.9659f * (1.0f - ev));
}

void blade_ctrl_draw(GContext *ctx, GPoint center, int r, const BladeRenderConfig *render_cfg) {
  if (!s_dynamic_path || !render_cfg) return;

  float ev = (s_progress > 1.0f) ? (2.0f - s_progress) : s_progress;
  int32_t angle = (int32_t)(ev * TRIG_MAX_ANGLE / 8);

  graphics_context_set_stroke_width(ctx, 1);

  for (int i = 0; i < 12; i++) {
    GColor fill = get_wing_fill_color(i, render_cfg);
    graphics_context_set_fill_color(ctx, fill);

    GColor stroke = gcolor_legible_over(fill);
    if (PBL_IF_COLOR_ELSE(true, false)) {
      if (gcolor_equal(stroke, GColorBlack)) stroke = GColorDarkGray;
      else stroke = GColorLightGray;
    }
    graphics_context_set_stroke_color(ctx, stroke);

    s_dynamic_path->points[0] = get_inner_vertex(i, r, angle, center);
    s_dynamic_path->points[1] = get_ray_far_point(i, r, angle, center);
    s_dynamic_path->points[2] = get_ray_far_point(i + 1, r, angle, center);
    s_dynamic_path->points[3] = get_inner_vertex(i + 1, r, angle, center);

    gpath_draw_filled(ctx, s_dynamic_path);
    gpath_draw_outline(ctx, s_dynamic_path);
  }
}
