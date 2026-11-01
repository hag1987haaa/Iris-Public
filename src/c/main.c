#include <pebble.h>
#include "blade_ctrl.h"

// --- 設定・構造体 ---
#define PERSIST_KEY_SETTINGS 1
#define PERSIST_KEY_ACTIVE_BIZ 2
#define PERSIST_KEY_SETTINGS_VERSION 100
#define CURRENT_SETTINGS_VERSION 7

typedef enum { WeatherClear, WeatherCloudy, WeatherRainy, WeatherSnowy, WeatherThunder } WeatherStatus;

typedef struct { 
  int watchface_mode; int theme; int anim_speed; int anim_freq; int anim_mode; int vibe_mode; int shutter_thick; 
  bool battery_color_sync; int shutter_color_mode; GColor custom_shutter_color; 
  WeatherStatus weather; WeatherStatus tomorrow_weather; 
  bool enable_business; int biz_start_h; int biz_end_h; 
  int biz_face; int biz_theme; int biz_shutter; int chime_interval;
  int biz_days; int biz_chime_interval; int biz_vibe_mode;
  int text_style;  // 0: Flat, 1: Drop Shadow, 2: Outline
  int biz_bg_mode; // 0: Solid Black, 1: Solid White, 2: Same as Normal Background
  int text_color;  // 0: Auto (Invert Bg), 1: Black, 2: White
  int biz_text_color; // 0: Auto (Invert Bg), 1: Black, 2: White
  int biz_blade_color; // 0: Solid Black, 1: Solid White
} ClaySettings;

static ClaySettings s_settings;
static ClaySettings s_pending_settings; 
static bool s_has_pending_settings = false; 

//ビジネスモードの状態管理
static bool is_biz_mode = false;        // 現在の「判定」結果
static bool s_active_biz_mode = false;  // 現在「表示」されているモード（シャッター裏ですり替える用）
static bool is_sleep_mode = false;      // スリープモード（強制ミュート）判定
static bool s_pending_biz_update = false;
//最後に時報が鳴った時間を記憶する変数！
static time_t s_last_chime_time = 0;

static Window *s_main_window;
static Layer *s_canvas_layer;
static char s_time_chars[4][2]; 
static char s_date_str[16];
static int32_t s_min_angle = 0, s_hour_angle = 0;
static GFont s_time_font = NULL, s_info_font;

static int s_unobstructed_y_offset = 0; 
static BatteryChargeState s_last_battery;
static char s_steps_str[16] = "00.0K"; 

static int s_current_f_size = 0;

// --- プロトタイプ宣言（コンパイラエラー回避！） ---
static void adjust_layout(int16_t unobstructed_h);
static void update_info(void); 
static void apply_theme_colors(void);
#if defined(PBL_COLOR)
static void unload_bg_bitmap(void);
static void check_and_request_missing_bg(void);
#endif

// --- 設定・カラー管理 ---
static void get_default_settings(ClaySettings *settings) {
  *settings = (ClaySettings){
    .watchface_mode = 0,      // Digital
    .theme = 0,               // White
    .anim_speed = 40,         // 400ms
    .anim_freq = 1,           // 1 min
    .anim_mode = 0,           // Full
    .vibe_mode = 2,           // Shake Only
    .shutter_thick = 1,       // Medium
    .battery_color_sync = true,
    .shutter_color_mode = PBL_IF_COLOR_ELSE(4, 0), // Pastel Rainbow (Color) / Solid Black (BW)
    .custom_shutter_color = GColorCyan,
    .weather = WeatherCloudy,
    .tomorrow_weather = WeatherClear,
    .enable_business = false,
    .biz_start_h = 9,         // 9:00
    .biz_end_h = 18,          // 18:00
    .biz_face = 1,            // Analog
    .biz_theme = 0,           // Black / Black
    .biz_shutter = 0,         // Wide
    .chime_interval = 60,     // Every 1 Hour
    .biz_days = 1,            // Weekdays (Mon-Fri)
    .biz_chime_interval = 60, // Every 1 Hour
    .biz_vibe_mode = 1,       // Periodic Only
    .text_style = 0,          // Flat
    .biz_bg_mode = 0,         // Solid Black
    .text_color = 0,          // Auto (Invert Bg)
    .biz_text_color = 0,      // Auto (Invert Bg)
    .biz_blade_color = 0      // Solid Black
  };
}

static void sanitize_settings(ClaySettings *s) {
  if (s->watchface_mode < 0 || s->watchface_mode > 1) s->watchface_mode = 0;
  if (s->theme < 0 || s->theme > 2) s->theme = 0;
  if (s->anim_speed < 10 || s->anim_speed > 100) s->anim_speed = 40;
  if (s->anim_freq < 0 || s->anim_freq > 60) s->anim_freq = 1;
  if (s->anim_mode < 0 || s->anim_mode > 3) s->anim_mode = 0;
  if (s->vibe_mode < 0 || s->vibe_mode > 3) s->vibe_mode = 2;
  if (s->shutter_thick < 0 || s->shutter_thick > 2) s->shutter_thick = 1;

  #if defined(PBL_COLOR)
    if (s->shutter_color_mode < 0 || s->shutter_color_mode > 6) s->shutter_color_mode = 4;
  #else
    if (s->shutter_color_mode < 0 || s->shutter_color_mode > 1) s->shutter_color_mode = 0;
  #endif

  if (s->biz_start_h < 0 || s->biz_start_h > 23) s->biz_start_h = 9;
  if (s->biz_end_h < 0 || s->biz_end_h > 23) s->biz_end_h = 18;
  if (s->biz_face < 0 || s->biz_face > 1) s->biz_face = 1;
  if (s->biz_theme < 0 || s->biz_theme > 3) s->biz_theme = 0;
  if (s->biz_shutter < 0 || s->biz_shutter > 2) s->biz_shutter = 0;
  if (s->chime_interval <= 0 || s->chime_interval > 60) s->chime_interval = 60;
  if (s->biz_days < 0 || s->biz_days > 1) s->biz_days = 1;
  if (s->biz_chime_interval <= 0 || s->biz_chime_interval > 60) s->biz_chime_interval = 60;
  if (s->biz_vibe_mode < 0 || s->biz_vibe_mode > 3) s->biz_vibe_mode = 1;
  if (s->text_style < 0 || s->text_style > 2) s->text_style = 0;
  if (s->biz_bg_mode < 0 || s->biz_bg_mode > 2) s->biz_bg_mode = 0;
  if (s->text_color < 0 || s->text_color > 2) s->text_color = 0;
  if (s->biz_text_color < 0 || s->biz_text_color > 2) s->biz_text_color = 0;
  if (s->biz_blade_color < 0 || s->biz_blade_color > 1) s->biz_blade_color = 0;
}

#if defined(PBL_COLOR)
#define PERSIST_KEY_BG_META 199
#define PERSIST_KEY_BG_CACHE_BASE 200
#define BG_CACHE_MAX_KEYS 24
#define BG_CACHE_CHUNK_SIZE 256
#define BG_CACHE_MAGIC 0x4952   // "IR" 高密度4bitキャッシュ形式マーカー

typedef struct {
  uint16_t magic;
  uint16_t full_w;
  uint16_t full_h;
  uint16_t cache_w;
  uint16_t cache_h;
  uint8_t palette[16];
} BgCacheMeta;

static GBitmap *s_bg_bitmap = NULL;
static int s_current_loaded_target = -1; // -1: none, 0: normal, 1: biz
static size_t s_image_rx_bytes = 0;
static bool s_is_rx_complete = false;
static bool s_is_full_res_loaded = false;
static bool s_pending_bg_swap = false;
static int s_pending_swap_target = 0;
static int s_pending_swap_w = 144;
static int s_pending_swap_h = 168;
static AppTimer *s_bg_req_retry_timer = NULL;
static AppTimer *s_rx_watchdog_timer = NULL;
static int s_pending_req_target = 0;

static void rx_watchdog_cb(void *data) {
  s_rx_watchdog_timer = NULL;
  s_image_rx_bytes = 0;
}

static void clear_bg_cache_persist(void) {
  if (persist_exists(PERSIST_KEY_BG_META)) {
    persist_delete(PERSIST_KEY_BG_META);
  }
}

static void write_cache_key_if_changed(int pkey, const void *buf, size_t len) {
  uint8_t existing_buf[BG_CACHE_CHUNK_SIZE];
  if (persist_exists(pkey) &&
      persist_read_data(pkey, existing_buf, len) == (int)len &&
      memcmp(buf, existing_buf, len) == 0) {
    return;
  }
  persist_write_data(pkey, buf, len);
}

static inline uint8_t get_block_avg_color6(const uint8_t *src, int width, int height, int bx, int by) {
  int r_sum = 0, g_sum = 0, b_sum = 0, cnt = 0;
  for (int dy = 0; dy < 2 && (by + dy) < height; dy++) {
    const uint8_t *row = src + (by + dy) * width + bx;
    for (int dx = 0; dx < 2 && (bx + dx) < width; dx++) {
      uint8_t c = row[dx];
      r_sum += (c >> 4) & 0x03;
      g_sum += (c >> 2) & 0x03;
      b_sum += c & 0x03;
      cnt++;
    }
  }
  if (cnt == 0) return 0;
  int r = (r_sum + (cnt >> 1)) / cnt;
  int g = (g_sum + (cnt >> 1)) / cnt;
  int b = (b_sum + (cnt >> 1)) / cnt;
  return (uint8_t)((r << 4) | (g << 2) | b);
}

static inline int color6_dist_sq(uint8_t c1, uint8_t c2) {
  int dr = (int)((c1 >> 4) & 0x03) - (int)((c2 >> 4) & 0x03);
  int dg = (int)((c1 >> 2) & 0x03) - (int)((c2 >> 2) & 0x03);
  int db = (int)(c1 & 0x03) - (int)(c2 & 0x03);
  return dr * dr + dg * dg + db * db;
}

static void save_bg_cache_to_persist(int width, int height) {
  if (!s_bg_bitmap) return;
  uint8_t *src = gbitmap_get_data(s_bg_bitmap);
  if (!src) return;

  uint16_t cw = width / 2;
  uint16_t ch = height / 2;

  // 1. 全画面の64色ヒストグラムを集計
  uint16_t hist[64];
  memset(hist, 0, sizeof(hist));
  for (int cy = 0; cy < (int)ch; cy++) {
    for (int cx = 0; cx < (int)cw; cx++) {
      hist[get_block_avg_color6(src, width, height, cx * 2, cy * 2)]++;
    }
  }

  // 2. 代表16色パレットの選出（色空間の多様性を確保しつつ頻出順）
  uint8_t palette[16];
  bool used[64];
  memset(used, 0, sizeof(used));
  int pal_count = 0;

  for (int pass = 0; pass < 2 && pal_count < 16; pass++) {
    int min_dist = (pass == 0) ? 2 : 0;
    while (pal_count < 16) {
      int best_c = -1;
      uint16_t best_cnt = 0;
      for (int c = 0; c < 64; c++) {
        if (used[c] || hist[c] == 0) continue;
        if (min_dist > 0) {
          bool too_close = false;
          for (int p = 0; p < pal_count; p++) {
            if (color6_dist_sq((uint8_t)c, palette[p] & 0x3F) < min_dist) {
              too_close = true;
              break;
            }
          }
          if (too_close) continue;
        }
        if (hist[c] > best_cnt) {
          best_cnt = hist[c];
          best_c = c;
        }
      }
      if (best_c < 0) break;
      used[best_c] = true;
      palette[pal_count++] = (uint8_t)(0xC0 | best_c);
    }
  }
  while (pal_count < 16) {
    palette[pal_count] = (pal_count > 0) ? palette[0] : 0xC0;
    pal_count++;
  }

  // 3. 64色 -> 16色パレットインデックス(0..15)の高速変換LUT作成
  uint8_t lut[64];
  for (int c = 0; c < 64; c++) {
    int best_p = 0, best_d = 999;
    for (int p = 0; p < 16; p++) {
      int d = color6_dist_sq((uint8_t)c, palette[p] & 0x3F);
      if (d < best_d) { best_d = d; best_p = p; }
    }
    lut[c] = (uint8_t)best_p;
  }

  // 4. 4bitパッキング（1バイトに2サンプル）してpersistへ保存（差分があるチャンクのみ書き込み）
  uint8_t chunk_buf[BG_CACHE_CHUNK_SIZE];
  size_t byte_idx = 0;
  int key_idx = 0;
  bool high_nibble = true;
  uint8_t cur_byte = 0;

  for (int cy = 0; cy < (int)ch; cy++) {
    for (int cx = 0; cx < (int)cw; cx++) {
      uint8_t p_idx = lut[get_block_avg_color6(src, width, height, cx * 2, cy * 2)] & 0x0F;
      if (high_nibble) {
        cur_byte = (uint8_t)(p_idx << 4);
        high_nibble = false;
      } else {
        chunk_buf[byte_idx++] = cur_byte | p_idx;
        high_nibble = true;
        if (byte_idx == BG_CACHE_CHUNK_SIZE) {
          if (key_idx < BG_CACHE_MAX_KEYS) {
            write_cache_key_if_changed(PERSIST_KEY_BG_CACHE_BASE + key_idx, chunk_buf, BG_CACHE_CHUNK_SIZE);
          }
          byte_idx = 0;
          key_idx++;
        }
      }
    }
  }

  if (!high_nibble) chunk_buf[byte_idx++] = cur_byte;
  if (byte_idx > 0 && key_idx < BG_CACHE_MAX_KEYS) {
    write_cache_key_if_changed(PERSIST_KEY_BG_CACHE_BASE + key_idx, chunk_buf, byte_idx);
  }

  BgCacheMeta meta = {
    .magic = BG_CACHE_MAGIC,
    .full_w = (uint16_t)width,
    .full_h = (uint16_t)height,
    .cache_w = cw,
    .cache_h = ch
  };
  memcpy(meta.palette, palette, 16);
  write_cache_key_if_changed(PERSIST_KEY_BG_META, &meta, sizeof(meta));
}

static bool load_bg_cache_from_persist(void) {
  if (!persist_exists(PERSIST_KEY_BG_META)) return false;

  BgCacheMeta meta;
  if (persist_read_data(PERSIST_KEY_BG_META, &meta, sizeof(meta)) != (int)sizeof(meta)) {
    return false;
  }
  if (meta.magic != BG_CACHE_MAGIC) return false;

  uint16_t width = meta.full_w;
  uint16_t height = meta.full_h;
  uint16_t cw = meta.cache_w;
  uint16_t ch = meta.cache_h;
  if (width == 0 || height == 0 || cw == 0 || ch == 0) return false;

  if (s_bg_bitmap == NULL) {
    s_bg_bitmap = gbitmap_create_blank(GSize(width, height), GBitmapFormat8Bit);
  }
  if (s_bg_bitmap == NULL) return false;

  uint8_t *dst = gbitmap_get_data(s_bg_bitmap);
  if (!dst) return false;

  memset(dst, meta.palette[0], (size_t)width * (size_t)height);

  uint8_t chunk_buf[BG_CACHE_CHUNK_SIZE];
  size_t chunk_offset = BG_CACHE_CHUNK_SIZE;
  int key_idx = 0;
  bool high_nibble = true;
  uint8_t cur_byte = 0;

  // パス1: 4bitパレットデータを展開して2x2ブロック基準点を配置（全画面完全カバー）
  for (int cy = 0; cy < (int)ch; cy++) {
    for (int cx = 0; cx < (int)cw; cx++) {
      uint8_t p_idx;
      if (high_nibble) {
        if (chunk_offset >= BG_CACHE_CHUNK_SIZE) {
          if (key_idx >= BG_CACHE_MAX_KEYS) return false;
          int pkey = PERSIST_KEY_BG_CACHE_BASE + key_idx;
          if (!persist_exists(pkey)) return false;
          persist_read_data(pkey, chunk_buf, BG_CACHE_CHUNK_SIZE);
          chunk_offset = 0;
          key_idx++;
        }
        cur_byte = chunk_buf[chunk_offset++];
        p_idx = (cur_byte >> 4) & 0x0F;
        high_nibble = false;
      } else {
        p_idx = cur_byte & 0x0F;
        high_nibble = true;
      }

      uint8_t color = meta.palette[p_idx];
      int base_y = cy * 2;
      int base_x = cx * 2;
      for (int dy = 0; dy < 2 && (base_y + dy) < (int)height; dy++) {
        uint8_t *row = dst + (base_y + dy) * width + base_x;
        for (int dx = 0; dx < 2 && (base_x + dx) < (int)width; dx++) {
          row[dx] = color;
        }
      }
    }
  }

  // パス2: サブピクセル・バイリニア＋ディザ補間で2x2ブロック境界のモザイク感を完全平滑化
  for (int cy = 0; cy < (int)ch - 1; cy++) {
    int by = cy * 2;
    if (by + 1 >= (int)height) continue;
    for (int cx = 0; cx < (int)cw - 1; cx++) {
      int bx = cx * 2;
      if (bx + 1 >= (int)width) continue;

      uint8_t c00 = dst[by * width + bx];
      uint8_t c10 = dst[by * width + (bx + 2)];
      uint8_t c01 = dst[(by + 2) * width + bx];
      uint8_t c11 = dst[(by + 2) * width + (bx + 2)];

      int r00 = (c00 >> 4) & 3, g00 = (c00 >> 2) & 3, b00 = c00 & 3;
      int r10 = (c10 >> 4) & 3, g10 = (c10 >> 2) & 3, b10 = c10 & 3;
      int r01 = (c01 >> 4) & 3, g01 = (c01 >> 2) & 3, b01 = c01 & 3;
      int r11 = (c11 >> 4) & 3, g11 = (c11 >> 2) & 3, b11 = c11 & 3;

      // 右隣ピクセル (bx+1, by)
      dst[by * width + bx + 1] = (uint8_t)(0xC0 | (((r00 + r10) >> 1) << 4) | (((g00 + g10) >> 1) << 2) | ((b00 + b10) >> 1));
      // 下隣ピクセル (bx, by+1)
      dst[(by + 1) * width + bx] = (uint8_t)(0xC0 | (((r00 + r01 + 1) >> 1) << 4) | (((g00 + g01 + 1) >> 1) << 2) | ((b00 + b01 + 1) >> 1));
      // 斜め中央ピクセル (bx+1, by+1)
      dst[(by + 1) * width + bx + 1] = (uint8_t)(0xC0 | (((r00 + r10 + r01 + r11 + 2) >> 2) << 4) | (((g00 + g10 + g01 + g11 + 2) >> 2) << 2) | ((b00 + b10 + b01 + b11 + 2) >> 2));
    }
  }

  s_current_loaded_target = 0;
  s_is_rx_complete = true;
  s_is_full_res_loaded = false;
  return true;
}

static void unload_bg_bitmap(void) {
  if (s_bg_req_retry_timer) {
    app_timer_cancel(s_bg_req_retry_timer);
    s_bg_req_retry_timer = NULL;
  }
  if (s_rx_watchdog_timer) {
    app_timer_cancel(s_rx_watchdog_timer);
    s_rx_watchdog_timer = NULL;
  }
  if (s_bg_bitmap) {
    gbitmap_destroy(s_bg_bitmap);
    s_bg_bitmap = NULL;
  }
  s_current_loaded_target = -1;
  s_image_rx_bytes = 0;
  s_is_rx_complete = false;
  s_is_full_res_loaded = false;
}

static void request_bg_image_from_phone(int target);

static void bg_req_retry_cb(void *data) {
  s_bg_req_retry_timer = NULL;
  request_bg_image_from_phone(s_pending_req_target);
}

static void request_bg_image_from_phone(int target) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_int(iter, MESSAGE_KEY_RequestBgImage, &target, sizeof(int), true);
    app_message_outbox_send();
  } else {
    s_pending_req_target = target;
    if (s_bg_req_retry_timer) app_timer_cancel(s_bg_req_retry_timer);
    s_bg_req_retry_timer = app_timer_register(250, bg_req_retry_cb, NULL);
  }
}

static void check_and_request_missing_bg(void) {
  if (!s_active_biz_mode) {
    if (s_settings.theme != 2) {
      return; // Theme == 0 (Solid White) / 1 (Solid Black): 写真は要求しない
    }
  } else {
    if (s_settings.biz_bg_mode != 2 || s_settings.theme != 2) {
      // biz_bg_mode != 2: 写真は要求しない。通常画像がRAMにあればそのまま温存！
      return;
    }
  }

  // すでに目的の画像（target 0）がロード済みなら何もしない（RAM温存で100%鮮明画像を継続！）
  if (s_bg_bitmap != NULL && s_is_rx_complete && s_current_loaded_target == 0) {
    return;
  }

  // 時計内キャッシュ（全画面カバー）から0.01秒で即座に復元
  if (s_bg_bitmap == NULL || !s_is_rx_complete) {
    load_bg_cache_from_persist();
  }

  if (!s_image_rx_bytes) {
    if (s_bg_bitmap != NULL && s_is_rx_complete) {
      s_pending_req_target = 0;
      if (s_bg_req_retry_timer) app_timer_cancel(s_bg_req_retry_timer);
      s_bg_req_retry_timer = app_timer_register(600, bg_req_retry_cb, NULL);
    } else {
      request_bg_image_from_phone(0);
    }
  }
}
#endif

static void draw_text_styled(GContext *ctx, const char *text, GFont font, GRect rect, 
                             GTextOverflowMode overflow, GTextAlignment align, 
                             GColor text_color, int style_mode) {
  if (style_mode > 0) {
    graphics_context_set_text_color(ctx, gcolor_legible_over(text_color));
    static const int8_t dx[4] = {-1, 1, 0, 0};
    static const int8_t dy[4] = {0, 0, -1, 1};
    if (style_mode == 1) {
      graphics_draw_text(ctx, text, font, GRect(rect.origin.x + 1, rect.origin.y + 1, rect.size.w, rect.size.h), overflow, align, NULL);
    } else {
      for (int i = 0; i < 4; i++) {
        graphics_draw_text(ctx, text, font, GRect(rect.origin.x + dx[i], rect.origin.y + dy[i], rect.size.w, rect.size.h), overflow, align, NULL);
      }
    }
  }
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, text, font, rect, overflow, align, NULL);
}

static void apply_theme_colors() {
  GColor bg;
  if (s_active_biz_mode) {
    if (s_settings.biz_bg_mode == 2) {
      // Same as Normal Background
      bg = (s_settings.theme == 0) ? GColorWhite : GColorBlack;
    } else {
      // 0: Solid Black, 1: Solid White
      bg = (s_settings.biz_bg_mode == 0) ? GColorBlack : GColorWhite;
    }
  } else {
    // 通常時の背景色（Theme 0: White, Theme 1: Black, Theme 2: Custom Photo -> 透過下地は黒）
    bg = (s_settings.theme == 0) ? GColorWhite : GColorBlack;
  }
  window_set_background_color(s_main_window, bg);
#if defined(PBL_COLOR)
  check_and_request_missing_bg();
#endif
}

static void save_settings() { 
  sanitize_settings(&s_settings);
  persist_write_data(PERSIST_KEY_SETTINGS, &s_settings, sizeof(s_settings)); 
  persist_write_int(PERSIST_KEY_SETTINGS_VERSION, CURRENT_SETTINGS_VERSION);
}

static void load_settings() {
  get_default_settings(&s_settings);

  bool need_fresh_save = false;
  int version = persist_exists(PERSIST_KEY_SETTINGS_VERSION) ? persist_read_int(PERSIST_KEY_SETTINGS_VERSION) : 0;

  if (version == CURRENT_SETTINGS_VERSION && persist_exists(PERSIST_KEY_SETTINGS)) {
    int read_bytes = persist_read_data(PERSIST_KEY_SETTINGS, &s_settings, sizeof(s_settings));
    if (read_bytes != (int)sizeof(s_settings)) {
      // 保存データサイズ不一致（破損・古い設定）
      get_default_settings(&s_settings);
      need_fresh_save = true;
    }
  } else {
    // バージョン不一致（旧バージョンからのアップデート直後など）
    // 旧データによるズレを防ぐため、最新のデフォルト値でリセット！
    need_fresh_save = true;
  }

  sanitize_settings(&s_settings);

  if (need_fresh_save) {
    save_settings();
  }

  // 再起動しても、自分がビジネスモードだったかを思い出す！
  if (persist_exists(PERSIST_KEY_ACTIVE_BIZ)) {
    s_active_biz_mode = persist_read_bool(PERSIST_KEY_ACTIVE_BIZ);
    is_biz_mode = s_active_biz_mode;
  }
}

static int get_dict_int(Tuple *t) {
  if (!t) return 0;
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return t->value->int32;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t;
	
  // =========================================================================
  // スマホから「設定教えて！」とリクエストが来た時、全データを送り返す処理
  // =========================================================================
  if (dict_find(iter, MESSAGE_KEY_RequestSettings)) {
    DictionaryIterator *out_iter;
    app_message_outbox_begin(&out_iter);
    if (out_iter != NULL) {
      //時計が持っている設定をすべて辞書に詰め込む！
      dict_write_int32(out_iter, MESSAGE_KEY_WatchFaceMode, s_settings.watchface_mode);
      dict_write_int32(out_iter, MESSAGE_KEY_Theme, s_settings.theme);
      dict_write_int32(out_iter, MESSAGE_KEY_AnimSpeed, s_settings.anim_speed);
      dict_write_int32(out_iter, MESSAGE_KEY_AnimFreq, s_settings.anim_freq);
      dict_write_int32(out_iter, MESSAGE_KEY_AnimMode, s_settings.anim_mode);
      dict_write_int32(out_iter, MESSAGE_KEY_VibeMode, s_settings.vibe_mode);
      dict_write_int32(out_iter, MESSAGE_KEY_ShutterThickness, s_settings.shutter_thick);
      dict_write_int32(out_iter, MESSAGE_KEY_BatteryColorSync, s_settings.battery_color_sync ? 1 : 0);
      dict_write_int32(out_iter, MESSAGE_KEY_ShutterColorMode, s_settings.shutter_color_mode);
      
      //色(GColor)は特殊！スマホで読めるように24bitのHEX(16進数)カラーコードに戻してあげる
      int r8 = s_settings.custom_shutter_color.r * 85;
      int g8 = s_settings.custom_shutter_color.g * 85;
      int b8 = s_settings.custom_shutter_color.b * 85;
      dict_write_int32(out_iter, MESSAGE_KEY_CustomColor, (r8 << 16) | (g8 << 8) | b8);

      dict_write_int32(out_iter, MESSAGE_KEY_EnableBusiness, s_settings.enable_business ? 1 : 0);
      dict_write_int32(out_iter, MESSAGE_KEY_BizStartH, s_settings.biz_start_h);
      dict_write_int32(out_iter, MESSAGE_KEY_BizEndH, s_settings.biz_end_h);
      dict_write_int32(out_iter, MESSAGE_KEY_BizFace, s_settings.biz_face);
      dict_write_int32(out_iter, MESSAGE_KEY_BizTheme, s_settings.biz_theme);
      dict_write_int32(out_iter, MESSAGE_KEY_BizShutter, s_settings.biz_shutter);
      dict_write_int32(out_iter, MESSAGE_KEY_ChimeInterval, s_settings.chime_interval);
      dict_write_int32(out_iter, MESSAGE_KEY_BizDays, s_settings.biz_days);
      dict_write_int32(out_iter, MESSAGE_KEY_BizChimeInterval, s_settings.biz_chime_interval);
      dict_write_int32(out_iter, MESSAGE_KEY_BizVibeMode, s_settings.biz_vibe_mode);
      dict_write_int32(out_iter, MESSAGE_KEY_TextStyle, s_settings.text_style);
      dict_write_int32(out_iter, MESSAGE_KEY_BizBgMode, s_settings.biz_bg_mode);
      dict_write_int32(out_iter, MESSAGE_KEY_BizBladeColor, s_settings.biz_blade_color);
      dict_write_int32(out_iter, MESSAGE_KEY_TextColor, s_settings.text_color);
      dict_write_int32(out_iter, MESSAGE_KEY_BizTextColor, s_settings.biz_text_color);

      app_message_outbox_send();
    }
    #if defined(PBL_COLOR)
    // PKJSがreadyになったタイミングなので、フル解像度画像が未ロードなら背景チェックを実行
    check_and_request_missing_bg();
    #endif
    return;
  }
  // =========================================================================

  // =========================================================================
  // 画像クリアコマンドの受信
  // =========================================================================
  if ((t = dict_find(iter, MESSAGE_KEY_BgImageClear))) {
    #if defined(PBL_COLOR)
    int clear_target = get_dict_int(t);
    if (clear_target == 0) {
      clear_bg_cache_persist();
    }
    if (s_current_loaded_target == clear_target || s_current_loaded_target == -1) {
      unload_bg_bitmap();
      check_and_request_missing_bg();
      if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
    }
    #endif
    return;
  }

  // =========================================================================
  // 画像チャンクデータの受信（シングルバッファ＋静止中更新でRAM半減＆白画面ゼロ化）
  // =========================================================================
  if ((t = dict_find(iter, MESSAGE_KEY_BgImageData))) {
    #if defined(PBL_COLOR)
    Tuple *t_target = dict_find(iter, MESSAGE_KEY_BgImageTarget);
    Tuple *t_chunk_idx = dict_find(iter, MESSAGE_KEY_BgImageChunkIdx);
    Tuple *t_total_chunks = dict_find(iter, MESSAGE_KEY_BgImageTotalChunks);
    Tuple *t_width = dict_find(iter, MESSAGE_KEY_BgImageWidth);
    Tuple *t_height = dict_find(iter, MESSAGE_KEY_BgImageHeight);

    int target = t_target ? get_dict_int(t_target) : 0;
    int chunk_idx = t_chunk_idx ? get_dict_int(t_chunk_idx) : 0;
    int total_chunks = t_total_chunks ? get_dict_int(t_total_chunks) : 1;
    int width = t_width ? get_dict_int(t_width) : 144;
    int height = t_height ? get_dict_int(t_height) : 168;

    // 通信中断時でも5秒後にロックを自動解除するウォッチドッグタイマー
    if (s_rx_watchdog_timer) app_timer_cancel(s_rx_watchdog_timer);
    s_rx_watchdog_timer = app_timer_register(5000, rx_watchdog_cb, NULL);

    if (chunk_idx == 0) {
      s_image_rx_bytes = 0;
      s_is_rx_complete = false; // 受信中は書きかけのデータを出さない
      if (s_bg_bitmap != NULL) {
        GSize cur_size = gbitmap_get_bounds(s_bg_bitmap).size;
        if (cur_size.w != width || cur_size.h != height) {
          gbitmap_destroy(s_bg_bitmap);
          s_bg_bitmap = NULL;
        }
      }
      if (s_bg_bitmap == NULL) {
        s_bg_bitmap = gbitmap_create_blank(GSize(width, height), GBitmapFormat8Bit);
      }
    }

    if (s_bg_bitmap != NULL) {
      uint8_t *bitmap_data = gbitmap_get_data(s_bg_bitmap);
      size_t max_bytes = width * height;
      size_t chunk_len = t->length;

      if (bitmap_data && s_image_rx_bytes + chunk_len <= max_bytes) {
        memcpy(bitmap_data + s_image_rx_bytes, t->value->data, chunk_len);
        s_image_rx_bytes += chunk_len;
      }

      // 全チャンク受信完了！
      if (chunk_idx == total_chunks - 1) {
        if (s_rx_watchdog_timer) {
          app_timer_cancel(s_rx_watchdog_timer);
          s_rx_watchdog_timer = NULL;
        }
        s_image_rx_bytes = 0;

        // すぐに画面を書き換えず、まずブレードを閉じるアニメーションを発動！
        s_pending_bg_swap = true;
        s_pending_swap_target = target;
        s_pending_swap_w = width;
        s_pending_swap_h = height;

        blade_ctrl_trigger(false);
      }
    }
    #endif
    return;
  }
	
  s_pending_settings = s_settings;
  s_has_pending_settings = true; 

  if((t=dict_find(iter, MESSAGE_KEY_WatchFaceMode))) s_pending_settings.watchface_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_Theme))) s_pending_settings.theme = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_AnimSpeed))) s_pending_settings.anim_speed = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_AnimFreq))) s_pending_settings.anim_freq = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_AnimMode))) s_pending_settings.anim_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_VibeMode))) s_pending_settings.vibe_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_ShutterThickness))) s_pending_settings.shutter_thick = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BatteryColorSync))) s_pending_settings.battery_color_sync = get_dict_int(t) == 1;
  if((t=dict_find(iter, MESSAGE_KEY_ShutterColorMode))) s_pending_settings.shutter_color_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_CustomColor))) s_pending_settings.custom_shutter_color = GColorFromHEX(get_dict_int(t));
  if((t=dict_find(iter, MESSAGE_KEY_WeatherStatus))) s_pending_settings.weather = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_WeatherStatusTomorrow))) s_pending_settings.tomorrow_weather = get_dict_int(t);

  //ビジネスモード設定の受信
  if((t=dict_find(iter, MESSAGE_KEY_EnableBusiness))) s_pending_settings.enable_business = get_dict_int(t) == 1;
  if((t=dict_find(iter, MESSAGE_KEY_BizStartH))) s_pending_settings.biz_start_h = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizEndH))) s_pending_settings.biz_end_h = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizFace))) s_pending_settings.biz_face = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizTheme))) s_pending_settings.biz_theme = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizShutter))) s_pending_settings.biz_shutter = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_ChimeInterval))) s_pending_settings.chime_interval = get_dict_int(t);
	//追加した3つの設定を受信する！
  if((t=dict_find(iter, MESSAGE_KEY_BizDays))) s_pending_settings.biz_days = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizChimeInterval))) s_pending_settings.biz_chime_interval = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizVibeMode))) s_pending_settings.biz_vibe_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_TextStyle))) s_pending_settings.text_style = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizBgMode))) s_pending_settings.biz_bg_mode = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizBladeColor))) s_pending_settings.biz_blade_color = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_TextColor))) s_pending_settings.text_color = get_dict_int(t);
  if((t=dict_find(iter, MESSAGE_KEY_BizTextColor))) s_pending_settings.biz_text_color = get_dict_int(t);
  
  sanitize_settings(&s_pending_settings);
  blade_ctrl_trigger(false);
}

//究極の「空気を読む」コンテキスト判定ロジック（夜勤・24時間完全対応版）
static void evaluate_context_modes() {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  
  bool new_biz = false;
  bool new_sleep = false;
  
  if (s_settings.enable_business && quiet_time_is_active()) {
    int h = t->tm_hour;
    int s = s_settings.biz_start_h;
    int e = s_settings.biz_end_h;
    
    //24時間対応＆日またぎ考慮の判定
    bool in_hours = false;
    if (s == e || (s == 0 && e == 23)) {
      in_hours = true; // 開始と終了が同じ、または0〜23なら24時間いつでも有効！
    } else {
      bool end_is_full_day = (e == 23);
      if (s < e) {
        in_hours = (h >= s && (end_is_full_day ? true : h < e));
      } else {
        in_hours = (h >= s || (end_is_full_day ? true : h < e));
      }
    }
    
    if (in_hours) new_biz = true;  // ビジネスモード
    else new_sleep = true;         // スリープモード
    
    // 「夜勤の土曜早朝」を金曜扱いにするロジック！
    int logical_wday = t->tm_wday; // 一旦、今日のカレンダー上の曜日を入れる

    // もし「日またぎ設定(s > e)」で、今の時間が「終了時間より前(h < e)」なら…
    if (s > e && h < e) {
      // タイムマシン発動！曜日を「1日前」にずらす！（例：土曜(6)なら金曜(5)として扱う）
      logical_wday = (logical_wday + 6) % 7; 
    }

    // 平日設定(1)で、論理的な曜日が日曜(0)か土曜(6)なら、やっぱり休日のスリープモードにする
    if (new_biz && s_settings.biz_days == 1 && (logical_wday == 0 || logical_wday == 6)) {
      new_biz = false;
      new_sleep = true;
    }
  }
  
  // モードが変わった瞬間を検知して、マジック変形を発動！
  if (new_biz != s_active_biz_mode && !s_pending_biz_update) {
    is_biz_mode = new_biz;
    s_pending_biz_update = true;
    if (!blade_ctrl_is_animating()) {
      blade_ctrl_trigger(false);
    }
  }
  
  is_sleep_mode = new_sleep;
}

// --- ブレード制御モジュール連携コールバック ---
static void get_blade_anim_config(BladeAnimConfig *out_cfg) {
  out_cfg->anim_speed = s_settings.anim_speed;
  out_cfg->anim_freq = s_settings.anim_freq;
  out_cfg->anim_mode = s_settings.anim_mode;
  out_cfg->vibe_mode = s_settings.vibe_mode;
  out_cfg->is_sleep_mode = is_sleep_mode;
  out_cfg->is_biz_mode = s_active_biz_mode;
#if defined(PBL_COLOR)
  out_cfg->has_pending_swap = (s_pending_biz_update || s_has_pending_settings || s_pending_bg_swap);
  out_cfg->block_tap_external = (s_image_rx_bytes > 0);
#else
  out_cfg->has_pending_swap = (s_pending_biz_update || s_has_pending_settings);
  out_cfg->block_tap_external = false;
#endif
  out_cfg->has_pending_biz = s_pending_biz_update;
}

static void on_blade_closed_midpoint(void) {
  update_info();

#if defined(PBL_COLOR)
  // ブレードが完全に閉まりきった瞬間に新画像を表示フラグON＆永続キャッシュ保存！
  if (s_pending_bg_swap) {
    s_is_rx_complete = true;
    s_is_full_res_loaded = true;
    s_current_loaded_target = s_pending_swap_target;
    s_pending_bg_swap = false;
    if (s_pending_swap_target == 0) {
      save_bg_cache_to_persist(s_pending_swap_w, s_pending_swap_h);
    }
  }
#endif

  // 設定すり替えまたはビジネスモード切り替え時のマジック変形！
  if (s_has_pending_settings || s_pending_biz_update) {
    if (s_has_pending_settings) {
      s_settings = s_pending_settings;
      s_has_pending_settings = false;
      save_settings();
      evaluate_context_modes();
    }
    s_active_biz_mode = is_biz_mode;
    s_pending_biz_update = false;
    persist_write_bool(PERSIST_KEY_ACTIVE_BIZ, s_active_biz_mode);

    apply_theme_colors();
    GRect unob = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
    adjust_layout(unob.size.h);
  }

  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void handle_battery(BatteryChargeState state) { s_last_battery = state; layer_mark_dirty(s_canvas_layer); }

static void update_info(void) {
  time_t temp = time(NULL); struct tm *tick = localtime(&temp);
  static char s_h[4], s_m[4], s_d[16];
  
  bool h24 = clock_is_24h_style();
  strftime(s_h, sizeof(s_h), h24 ? "%H" : "%I", tick); 
  
  strftime(s_m, sizeof(s_m), "%M", tick); 
  if (s_h[0] == ' ') s_h[0] = '0';
  if (s_m[0] == ' ') s_m[0] = '0';
  
  s_time_chars[0][0] = s_h[0]; s_time_chars[0][1] = '\0';
  s_time_chars[1][0] = s_h[1]; s_time_chars[1][1] = '\0';
  s_time_chars[2][0] = s_m[0]; s_time_chars[2][1] = '\0';
  s_time_chars[3][0] = s_m[1]; s_time_chars[3][1] = '\0';

  s_min_angle = TRIG_MAX_ANGLE * tick->tm_min / 60;
  s_hour_angle = TRIG_MAX_ANGLE * (((tick->tm_hour % 12) * 60) + tick->tm_min) / 720;

  strftime(s_d, sizeof(s_d), "%a. %b %d", tick);
  for (int i=0; s_d[i]; i++) if(s_d[i]>='a' && s_d[i]<='z') s_d[i] -= 32;
  strncpy(s_date_str, s_d, sizeof(s_date_str));

  int steps = (int)health_service_sum_today(HealthMetricStepCount);
  snprintf(s_steps_str, sizeof(s_steps_str), "%02d.%dK", steps / 1000, (steps % 1000) / 100);
  
  layer_mark_dirty(s_canvas_layer); 
}

static void handle_tick(struct tm *tick, TimeUnits units) {

  evaluate_context_modes();

  //ビジネスモードかどうかで「時報間隔」と「バイブ設定」を自動で切り替える！
  int current_interval = s_active_biz_mode ? s_settings.biz_chime_interval : s_settings.chime_interval;
  int current_vibe_mode = s_active_biz_mode ? s_settings.biz_vibe_mode : s_settings.vibe_mode;

  bool is_chime_time = false;
  if (current_interval == 60) {
    is_chime_time = (tick->tm_min == 0);
  } else if (current_interval > 0) {
    is_chime_time = (tick->tm_min % current_interval == 0);
  }
  
  //スリープモードじゃなくて、バイブ設定がONなら鳴らす！
  if (is_chime_time && !is_sleep_mode) {
    //前回鳴らしてから50秒以上経過している時だけ許可！
    if (time(NULL) - s_last_chime_time > 50) {
      if (current_vibe_mode == 0 || current_vibe_mode == 1) {
        blade_ctrl_vibe_short_pulse_safe();
        s_last_chime_time = time(NULL); // 💡 ここで時間をメモ！
      }
    }
  }

  // ブレードのアニメーション制御・情報更新の振り分け
  blade_ctrl_handle_tick(tick->tm_min, is_chime_time);

  if (tick->tm_min == 0) {
    DictionaryIterator *iter; app_message_outbox_begin(&iter);
    if (iter != NULL) { dict_write_uint8(iter, 0, 1); app_message_outbox_send(); }
  }
}

static void adjust_layout(int16_t unobstructed_h) {
  GRect fb = layer_get_bounds(window_get_root_layer(s_main_window));
  s_unobstructed_y_offset = (fb.size.h - unobstructed_h) / 2;

  //ビジネスモードならシャッター太さを上書き！
  int current_shutter = s_active_biz_mode ? s_settings.biz_shutter : s_settings.shutter_thick;
  int f_size = 70;
  
  #if defined(PBL_PLATFORM_GABBRO)
    f_size = (current_shutter == 0) ? 100 : ((current_shutter == 1) ? 80 : 64);
  #elif defined(PBL_PLATFORM_EMERY)
    f_size = (current_shutter == 0) ? 100 : ((current_shutter == 1) ? 85 : 70);
  #elif defined(PBL_PLATFORM_CHALK)
    f_size = (current_shutter == 0) ? 70 : ((current_shutter == 1) ? 58 : 48);
  #else
    f_size = (current_shutter == 0) ? 70 : ((current_shutter == 1) ? 62 : 48);
  #endif

  if (s_current_f_size != f_size) {
    if (s_time_font) fonts_unload_custom_font(s_time_font);
    uint32_t res_id;

    #if defined(PBL_PLATFORM_GABBRO)
      switch (f_size) {
        case 100: res_id = RESOURCE_ID_FONT_TEKO_BOLD_100; break;
        case 80:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_80; break;
        case 64:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_64; break;
        default:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_80; break;
      }
    #elif defined(PBL_PLATFORM_EMERY)
      switch (f_size) {
        case 100: res_id = RESOURCE_ID_FONT_TEKO_BOLD_100; break;
        case 85:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_85; break;
        case 70:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_70; break;
        default:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_85; break;
      }
    #elif defined(PBL_PLATFORM_CHALK)
      switch (f_size) {
        case 70:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_70; break;
        case 58:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_58; break;
        case 48:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_48; break;
        default:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_58; break;
      }
    #else
      switch (f_size) {
        case 70:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_70; break;
        case 62:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_62; break;
        case 48:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_48; break;
        default:  res_id = RESOURCE_ID_FONT_TEKO_BOLD_62; break;
      }
    #endif

    s_time_font = fonts_load_custom_font(resource_get_handle(res_id));
    s_current_f_size = f_size; 
  }
}

#if !defined(PBL_PLATFORM_APLITE)
static void unobstructed_did_change(void *context) {
  GRect unob = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
  adjust_layout(unob.size.h); layer_mark_dirty(s_canvas_layer);
}
#endif

static void draw_weather_icon_raw(GContext *ctx, GPoint center, GColor color, WeatherStatus status) {
  graphics_context_set_fill_color(ctx, color); graphics_context_set_stroke_color(ctx, color);
  switch (status) {
    case WeatherClear:
      graphics_fill_circle(ctx, center, 3);
      for (int i = 0; i < 8; i++) {
        int32_t a = i * TRIG_MAX_ANGLE / 8;
        graphics_draw_line(ctx, (GPoint){center.x + (5*cos_lookup(a)/TRIG_MAX_RATIO), center.y + (5*sin_lookup(a)/TRIG_MAX_RATIO)}, (GPoint){center.x + (8*cos_lookup(a)/TRIG_MAX_RATIO), center.y + (8*sin_lookup(a)/TRIG_MAX_RATIO)});
      }
      break;
    case WeatherCloudy:
      graphics_fill_circle(ctx, (GPoint){center.x - 3, center.y + 1}, 3); graphics_fill_circle(ctx, (GPoint){center.x, center.y - 2}, 4); graphics_fill_circle(ctx, (GPoint){center.x + 3, center.y + 1}, 3);
      break;
    case WeatherRainy:
      graphics_fill_circle(ctx, (GPoint){center.x - 3, center.y + 1}, 3); graphics_fill_circle(ctx, (GPoint){center.x, center.y - 2}, 4); graphics_fill_circle(ctx, (GPoint){center.x + 3, center.y + 1}, 3);
      for (int i = 0; i < 3; i++) graphics_draw_line(ctx, (GPoint){center.x - 4 + i * 4, center.y + 3}, (GPoint){center.x - 2 + i * 4, center.y + 7});
      break;
    case WeatherSnowy:
      graphics_fill_circle(ctx, (GPoint){center.x, center.y + 2}, 3); graphics_fill_circle(ctx, (GPoint){center.x, center.y - 2}, 2); 
      break;
    case WeatherThunder:
      graphics_draw_line(ctx, (GPoint){center.x + 2, center.y - 4}, (GPoint){center.x - 2, center.y}); graphics_draw_line(ctx, (GPoint){center.x - 2, center.y}, (GPoint){center.x + 2, center.y}); graphics_draw_line(ctx, (GPoint){center.x + 2, center.y}, (GPoint){center.x - 2, center.y + 4});
      break;
  }
}

static void draw_weather_icon(GContext *ctx, GPoint center, GColor color, WeatherStatus status, int style_mode) {
  if (style_mode > 0) {
    GColor shadow = gcolor_legible_over(color);
    static const int8_t dx[4] = {-1, 1, 0, 0};
    static const int8_t dy[4] = {0, 0, -1, 1};
    if (style_mode == 1) {
      draw_weather_icon_raw(ctx, (GPoint){center.x + 1, center.y + 1}, shadow, status);
    } else {
      for (int i = 0; i < 4; i++) {
        draw_weather_icon_raw(ctx, (GPoint){center.x + dx[i], center.y + dy[i]}, shadow, status);
      }
    }
  }
  draw_weather_icon_raw(ctx, center, color, status);
}

static void draw_boot_icon_raw(GContext *ctx, int x, int y, int ex) {
  graphics_fill_rect(ctx, GRect(x,     y + 6, 9 + ex, 2 + ex), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x,     y,     3 + ex, 6 + ex), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 3, y + 2, 3 + ex, 4 + ex), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 6, y + 4, 3 + ex, 2 + ex), 0, GCornerNone);
}

static void draw_boot_icon(GContext *ctx, int boot_x, int boot_y, GColor color, int style_mode) {
  if (style_mode > 0) {
    graphics_context_set_fill_color(ctx, gcolor_legible_over(color));
    if (style_mode == 1) {
      draw_boot_icon_raw(ctx, boot_x + 2, boot_y + 4, 0);
    } else {
      draw_boot_icon_raw(ctx, boot_x, boot_y + 2, 2);
    }
  }
  graphics_context_set_fill_color(ctx, color);
  draw_boot_icon_raw(ctx, boot_x + 1, boot_y + 3, 0);
}

// 描画エンジン
static void update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer); GPoint c = grect_center_point(&b); c.y -= s_unobstructed_y_offset;

#if defined(PBL_COLOR)
  bool should_draw_bg = false;
  if (s_bg_bitmap != NULL && s_is_rx_complete) {
    if (!s_active_biz_mode) {
      if (s_settings.theme == 2 && s_current_loaded_target == 0) should_draw_bg = true;
    } else {
      if (s_settings.biz_bg_mode == 2 && s_settings.theme == 2 && s_current_loaded_target == 0) {
        should_draw_bg = true;
      }
      // biz_bg_mode == 0 (Solid Black), biz_bg_mode == 1 (Solid White): should_draw_bg = false
    }
  }

  if (should_draw_bg && s_bg_bitmap != NULL) {
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, s_bg_bitmap, b);
  }
#endif

  //ビジネスモードなら設定を上書きする
  int current_mode = s_active_biz_mode ? s_settings.biz_face : s_settings.watchface_mode;
  int current_shutter = s_active_biz_mode ? s_settings.biz_shutter : s_settings.shutter_thick;

  int r = blade_ctrl_get_aperture_radius(b.size.w, current_shutter);

  graphics_context_set_antialiased(ctx, !blade_ctrl_is_animating());

//文字色・針色を「背景と同化しない高コントラスト」に設定する
  GColor txt_color;
  if (s_active_biz_mode) {
    if (s_settings.biz_text_color == 1) {
      txt_color = GColorBlack;
    } else if (s_settings.biz_text_color == 2) {
      txt_color = GColorWhite;
    } else {
      // 0: Auto (背景色に応じて反転)
      if (s_settings.biz_bg_mode == 0) {
        txt_color = GColorWhite; // 黒背景 -> 白文字
      } else if (s_settings.biz_bg_mode == 1) {
        txt_color = GColorBlack; // 白背景 -> 黒文字
      } else {
        txt_color = (s_settings.theme == 0) ? GColorBlack : GColorWhite;
      }
    }
  } else {
    if (s_settings.text_color == 1) {
      txt_color = GColorBlack;
    } else if (s_settings.text_color == 2) {
      txt_color = GColorWhite;
    } else {
      // 0: Auto (背景色に応じて反転。Theme 0: 白背景->黒文字、Theme 1, 2: 黒背景/写真->白文字)
      txt_color = (s_settings.theme == 0) ? GColorBlack : GColorWhite;
    }
  }

  int32_t min_angle = s_min_angle;
  int32_t hour_angle = s_hour_angle;

  // --- 奥のコンテンツ描画（文字・針） ---
  // ブレード（シャッター羽）の手前に描画しておくことで、ブレードが最前面として被さりながら閉じる
  graphics_context_set_fill_color(ctx, txt_color);
  graphics_context_set_stroke_color(ctx, txt_color);
  graphics_context_set_text_color(ctx, txt_color);

  if (current_mode == 0) {
    // デジタル時計モード
    int f_h = s_current_f_size;
    int char_spacing = f_h / 2; 
    int gap = (f_h / 15 < 2) ? 2 : (f_h / 15);
    int c1 = c.x - (gap / 2) - (char_spacing / 2);
    int c2 = c.x + (gap / 2) + (char_spacing / 2);
    int cx[4] = {c1 - char_spacing, c1, c2, c2 + char_spacing};
    int text_y = c.y - (f_h / 2) - (f_h / 6);
    
    for (int i = 0; i < 4; i++) {
      draw_text_styled(ctx, s_time_chars[i], s_time_font, GRect(cx[i] - (f_h / 2), text_y, f_h, f_h + 20), GTextOverflowModeFill, GTextAlignmentCenter, txt_color, s_settings.text_style);
    }

    // デジタル上下の補助情報（開口部が通常表示サイズ付近の基準位置）
    int line_top_y = c.y - (r / 2); int line_bottom_y = c.y + (r / 2);

    draw_text_styled(ctx, s_date_str, s_info_font, GRect(0, line_top_y - 12, b.size.w, 15), GTextOverflowModeFill, GTextAlignmentCenter, txt_color, s_settings.text_style);
    int info_y = line_bottom_y - 2;
    int boot_x = c.x - 44; 
    draw_boot_icon(ctx, boot_x, info_y, txt_color, s_settings.text_style);

    draw_text_styled(ctx, s_steps_str, s_info_font, GRect(c.x - 30, info_y - 2, 45, 15), GTextOverflowModeFill, GTextAlignmentLeft, txt_color, s_settings.text_style);
    draw_weather_icon(ctx, (GPoint){c.x + 16, info_y + 4}, txt_color, s_settings.weather, s_settings.text_style);
    draw_text_styled(ctx, "/", s_info_font, GRect(c.x + 24, info_y - 2, 10, 15), GTextOverflowModeFill, GTextAlignmentLeft, txt_color, s_settings.text_style);
    draw_weather_icon(ctx, (GPoint){c.x + 36, info_y + 4}, txt_color, s_settings.tomorrow_weather, s_settings.text_style);

  } else {
    // アナログ時計モード（針＋情報）
    int safe_r = 36; 
    int h_orbit_r = safe_r + (r - safe_r) / 2;
    
    // 破線レーダー
    graphics_context_set_stroke_color(ctx, txt_color);
    graphics_context_set_stroke_width(ctx, 1); 
    int num_dashes = 36;
    for (int i = 0; i < num_dashes; i++) {
      int32_t a1 = TRIG_MAX_ANGLE * i / num_dashes;
      int32_t a2 = TRIG_MAX_ANGLE * i / num_dashes + (TRIG_MAX_ANGLE / (num_dashes * 2));
      GPoint p1 = { c.x + (h_orbit_r * sin_lookup(a1)) / TRIG_MAX_RATIO, c.y - (h_orbit_r * cos_lookup(a1)) / TRIG_MAX_RATIO };
      GPoint p2 = { c.x + (h_orbit_r * sin_lookup(a2)) / TRIG_MAX_RATIO, c.y - (h_orbit_r * cos_lookup(a2)) / TRIG_MAX_RATIO };
      graphics_draw_line(ctx, p1, p2);
    }
    
    // 分マーカー（長針）
    int m_d_start = h_orbit_r + 1;
    int m_d_end = b.size.w; 
    int32_t sin_m = sin_lookup(min_angle); int32_t cos_m = cos_lookup(min_angle);
    GPoint m_p_start = { c.x + (m_d_start * sin_m) / TRIG_MAX_RATIO, c.y - (m_d_start * cos_m) / TRIG_MAX_RATIO };
    GPoint m_p_end   = { c.x + (m_d_end * sin_m) / TRIG_MAX_RATIO, c.y - (m_d_end * cos_m) / TRIG_MAX_RATIO };
    if (s_settings.text_style > 0) {
      GColor shadow_color = gcolor_legible_over(txt_color);
      graphics_context_set_stroke_color(ctx, shadow_color);
      graphics_context_set_stroke_width(ctx, 7); 
      graphics_draw_line(ctx, m_p_start, m_p_end);
    }
    graphics_context_set_stroke_color(ctx, txt_color);
    graphics_context_set_stroke_width(ctx, 5); 
    graphics_draw_line(ctx, m_p_start, m_p_end);

    // 時マーカー（短針）
    GColor h_color = PBL_IF_COLOR_ELSE(GColorRed, txt_color);
    int32_t sin_h = sin_lookup(hour_angle); int32_t cos_h = cos_lookup(hour_angle);
    int h_start_r = safe_r + 2; int h_end_r = h_orbit_r - 2; 
    GPoint h_p_start = { c.x + (h_start_r * sin_h) / TRIG_MAX_RATIO, c.y - (h_start_r * cos_h) / TRIG_MAX_RATIO };
    GPoint h_p_end   = { c.x + (h_end_r * sin_h) / TRIG_MAX_RATIO, c.y - (h_end_r * cos_h) / TRIG_MAX_RATIO };
    if (s_settings.text_style > 0) {
      GColor shadow_color = gcolor_legible_over(h_color);
      graphics_context_set_stroke_color(ctx, shadow_color);
      graphics_context_set_stroke_width(ctx, 8); 
      graphics_draw_line(ctx, h_p_start, h_p_end); 
    }
    graphics_context_set_stroke_color(ctx, h_color);
    graphics_context_set_stroke_width(ctx, 6); 
    graphics_draw_line(ctx, h_p_start, h_p_end); 

    // アナログ補助情報
    int top_steps_y = c.y - 27; 
    int mid_date_y  = c.y - 8;  
    int bot_weather_y = c.y + 11; 

    int boot_x = c.x - 22; 
    draw_boot_icon(ctx, boot_x, top_steps_y + 2, txt_color, s_settings.text_style);
    draw_text_styled(ctx, s_steps_str, s_info_font, GRect(boot_x + 14, top_steps_y, 40, 15), GTextOverflowModeFill, GTextAlignmentLeft, txt_color, s_settings.text_style);

    draw_text_styled(ctx, s_date_str, s_info_font, GRect(0, mid_date_y, b.size.w, 15), GTextOverflowModeFill, GTextAlignmentCenter, txt_color, s_settings.text_style);

    draw_weather_icon(ctx, (GPoint){c.x - 10, bot_weather_y + 6}, txt_color, s_settings.weather, s_settings.text_style);
    draw_text_styled(ctx, "/", s_info_font, GRect(c.x - 4, bot_weather_y, 10, 15), GTextOverflowModeFill, GTextAlignmentCenter, txt_color, s_settings.text_style);
    draw_weather_icon(ctx, (GPoint){c.x + 10, bot_weather_y + 6}, txt_color, s_settings.tomorrow_weather, s_settings.text_style);
  }

  // --- 【最前面】シャッター羽（ブレード）の描画 ---
  BladeRenderConfig blade_render_cfg = {
    .is_biz_mode = s_active_biz_mode,
    .biz_blade_color = s_settings.biz_blade_color,
    .theme = s_settings.theme,
    .shutter_color_mode = s_settings.shutter_color_mode,
    .custom_color = s_settings.custom_shutter_color,
    .battery_color_sync = s_settings.battery_color_sync,
    .battery_pct = s_last_battery.charge_percent
  };
  blade_ctrl_draw(ctx, c, r, &blade_render_cfg);
}

static void main_window_load(Window *window) {
  Layer *wl = window_get_root_layer(window); GRect fb = layer_get_bounds(wl);
  s_info_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DENKICHIP_SUBSET_12));
  s_canvas_layer = layer_create(fb); 
  layer_set_update_proc(s_canvas_layer, update_proc); 
  layer_add_child(wl, s_canvas_layer);

  static const BladeCallbacks s_blade_callbacks = {
    .get_anim_config = get_blade_anim_config,
    .on_closed_midpoint = on_blade_closed_midpoint,
    .on_update_no_anim = update_info,
    .on_focus_gained = evaluate_context_modes
  };
  blade_ctrl_init(s_canvas_layer, &s_blade_callbacks);

#if !defined(PBL_PLATFORM_APLITE)
  unobstructed_area_service_unsubscribe(); 
  unobstructed_area_service_subscribe((UnobstructedAreaHandlers) { .did_change = unobstructed_did_change }, NULL);
#endif
  s_last_battery = battery_state_service_peek();
  
#if defined(PBL_COLOR)
  // 起動直後に時計本体の永続領域（persist）からキャッシュ画像を0.01秒で即座に復元！（白背景を完全防止）
  load_bg_cache_from_persist();
#endif

  apply_theme_colors(); 
  adjust_layout(fb.size.h); 
  update_info();
}

static void main_window_unload(Window *window) {
  blade_ctrl_deinit();
#if defined(PBL_COLOR)
  unload_bg_bitmap();
#endif
  if (s_time_font) fonts_unload_custom_font(s_time_font); 
  fonts_unload_custom_font(s_info_font);
  layer_destroy(s_canvas_layer);
#if !defined(PBL_PLATFORM_APLITE)
  unobstructed_area_service_unsubscribe(); 
#endif
}

int main(void) {
  load_settings(); 
  app_message_register_inbox_received(inbox_received_handler); 
  app_message_open(PBL_IF_COLOR_ELSE(4096, 256), 256);
  
  s_main_window = window_create(); 
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load, 
    .unload = main_window_unload
  }); 
  window_stack_push(s_main_window, true);
  
  blade_ctrl_subscribe_events();
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick); 
  battery_state_service_subscribe(handle_battery);
  
  app_event_loop();
  
  blade_ctrl_unsubscribe_events();
  battery_state_service_unsubscribe(); 
  tick_timer_service_unsubscribe(); 
  window_destroy(s_main_window);
}