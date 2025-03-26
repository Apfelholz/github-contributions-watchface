#include <pebble.h>

#define KEY_CONTRIBUTIONS 0
#define KEY_GITHUB_USERNAME 1
#define KEY_GITHUB_TOKEN 2
#define SETTINGS_KEY 3

static Window *s_main_window;
static TextLayer *s_time_layer;
static uint8_t s_contributions[49];
static AppTimer *s_timer;
static Layer *s_canvas_layer;

typedef struct ClaySettings {
  char github_username[32];
  char github_token[64];
  bool is_dithered;
} ClaySettings;

static ClaySettings settings;

static void fetch_contributions();

#ifdef PBL_COLOR
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect frame = grect_inset(bounds, GEdgeInsets(0));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  int cell_size = (bounds.size.w / 7) - 1;
  int corner_radius = 5;

  int x = 2;
  int y = 0;
  for (int week = 0; week < 7; week++) {
    for (int day = 0; day < 7; day++) {
      uint8_t contributions = s_contributions[week * 7 + day];
      GColor color = GColorWhite;
      if (contributions == 0) {
        color = GColorDarkGray;
      } else if (contributions < 3) {
        color = GColorDarkGreen;
      } else if (contributions < 6) {
        color = GColorIslamicGreen;
      } else if (contributions < 30) {
        color = GColorGreen;
      }

      GRect cell = GRect(x, y, cell_size, cell_size);
      graphics_context_set_fill_color(ctx, color);
      graphics_fill_rect(ctx, cell, corner_radius, GCornersAll);

      x += cell_size + 1;
    }
    x = 2;
    y += cell_size + 1;
  }
}
#else
/**
 * Helper function for set_pixel_color. Sets the individual bit in a byte
 */
static void byte_set_bit(uint8_t *byte, uint8_t bit, uint8_t value) {
  *byte ^= (-value ^ *byte) & (1 << bit);
}

/**
 * Set the pixel value to black or white
 */
static void set_pixel_color(uint8_t *data, GPoint point, uint8_t bytes_per_row,
                            uint8_t color) {
  size_t byte = point.y * bytes_per_row + point.x / 8;
  size_t bit = point.x % 8;
  byte_set_bit(&data[byte], bit, color);
}

/**
 * From https://en.wikipedia.org/wiki/Ordered_dithering
 * The 4x4 Bayer matrix
 */
// static uint8_t bayer_matrix_2x2[] = {
//     0,
//     127,
//     191,
//     63,
// };
static uint8_t bayer_matrix_4x4[] = {
    0, 128, 32, 159, 191, 64, 223, 96, 48, 175, 16, 143, 239, 112, 207, 80,
};
// static uint8_t bayer_matrix_8x8[] = {
//     0,  127, 31, 159, 7,  135, 39, 167, 191, 63,  223, 95,  199, 71,  231,
//     103, 47, 175, 15, 143, 55, 183, 23, 151, 239, 111, 207, 79,  247, 119,
//     215, 87, 11, 139, 43, 171, 3,  131, 35, 163, 203, 75,  235, 107, 195, 67,
//     227, 99, 59, 187, 27, 155, 51, 179, 19, 147, 251, 123, 219, 91,  243,
//     115, 211, 83,
// };

/**
 * Dithers an area of white using Bayer dithering
 * @param data The image data
 * @param cell The rectangle to apply dithering to
 * @param bytes_per_row The number of bytes per row (gotten from the GBitmap)
 * @param value The brightness of the average colour after dithering
 */
static void dither_rect(uint8_t *data, GRect cell, uint8_t bytes_per_row,
                        uint8_t value) {
  for (int y = cell.origin.y; y < cell.origin.y + cell.size.h; y++) {
    for (int x = cell.origin.x; x < cell.origin.x + cell.size.w; x++) {
      uint8_t threshold = bayer_matrix_4x4[(y % 4) * 4 + x % 4];
      if (value < threshold) {
        set_pixel_color(data, GPoint(x, y), bytes_per_row, 0);
      }
    }
  }
}

/**
 * Dithers an area of white using Bayer dithering
 * @param data The image data
 * @param cell The rectangle to apply dithering to
 * @param bytes_per_row The number of bytes per row (gotten from the GBitmap)
 * @param value The darkness of the average colour after dithering
 */
static void stripe_rect(uint8_t *data, GRect cell, uint8_t bytes_per_row,
                        uint8_t value) {
  for (int y = cell.origin.y; y < cell.origin.y + cell.size.h; y++) {
    for (int x = cell.origin.x; x < cell.origin.x + cell.size.w; x++) {
      if ((x + y) % value) {
        set_pixel_color(data, GPoint(x, y), bytes_per_row, 0);
      }
    }
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect frame = grect_inset(bounds, GEdgeInsets(0));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  int cell_size = (bounds.size.w / 7) - 1;
  int corner_radius = 5;

  // Draw rounded squares first as a mask because I don't want to bother doing
  // the math for the corner radii
  int x = 2;
  int y = 0;
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int week = 0; week < 7; week++) {
    for (int day = 0; day < 7; day++) {
      GRect cell = GRect(x, y, cell_size, cell_size);
      graphics_fill_rect(ctx, cell, corner_radius, GCornersAll);
      x += cell_size + 1;
    }
    x = 2;
    y += cell_size + 1;
  }

  x = 2;
  y = 0;
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *data = gbitmap_get_data(fb);
  uint16_t bytes_per_row = gbitmap_get_bytes_per_row(fb);
  for (int week = 0; week < 7; week++) {
    for (int day = 0; day < 7; day++) {
      uint8_t contributions = s_contributions[week * 7 + day];
      uint8_t value = 5;
      if (contributions == 0) {
        value = 4;
      } else if (contributions < 3) {
        value = 3;
      } else if (contributions < 6) {
        value = 2;
      } else if (contributions < 30) {
        value = 1;
      }

      if (value != 5) {
        GRect cell = GRect(x, y, cell_size, cell_size);
        if (settings.is_dithered) {
          value = value * 255 / 5;
          dither_rect(data, cell, bytes_per_row, value);
        } else {
          stripe_rect(data, cell, bytes_per_row, 6 - value);
        }
      }

      x += cell_size + 1;
    }
    x = 2;
    y += cell_size + 1;
  }
  graphics_release_frame_buffer(ctx, fb);
}
#endif

static void update_time(struct tm *tick_time) {
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M",
           tick_time);

  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    update_time(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    fetch_contributions();
  }
}

static void fetch_contributions() {
  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    dict_write_cstring(out_iter, MESSAGE_KEY_KEY_GITHUB_USERNAME,
                       settings.github_username);
    dict_write_cstring(out_iter, MESSAGE_KEY_KEY_GITHUB_TOKEN,
                       settings.github_token);
    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending request: %d", result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error beginning message: %d", result);
  }
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings(void) {
  // Make dithering the default
  settings.is_dithered = true;
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *contributions_tuple = dict_find(iter, MESSAGE_KEY_KEY_CONTRIBUTIONS);
  Tuple *username_tuple = dict_find(iter, MESSAGE_KEY_KEY_GITHUB_USERNAME);
  Tuple *token_tuple = dict_find(iter, MESSAGE_KEY_KEY_GITHUB_TOKEN);
  Tuple *is_dithered_tuple = dict_find(iter, MESSAGE_KEY_KEY_IS_DITHERED);

  if (username_tuple) {
    snprintf(settings.github_username, sizeof(settings.github_username), "%s",
             username_tuple->value->cstring);
  }

  if (token_tuple) {
    snprintf(settings.github_token, sizeof(settings.github_token), "%s",
             token_tuple->value->cstring);
  }

  if (is_dithered_tuple) {
    settings.is_dithered = is_dithered_tuple->value->int32;
  }

  if (is_dithered_tuple || username_tuple || token_tuple) {
    prv_save_settings();
    if (username_tuple || token_tuple) {
      fetch_contributions();
    }
  }

  if (contributions_tuple) {
    const int length = 49;
    uint8_t *con = contributions_tuple->value->data;
    memcpy(s_contributions, con, length);
    layer_mark_dirty(s_canvas_layer);
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_time_layer =
      text_layer_create(GRect(0, bounds.size.h - 35, bounds.size.w, 30));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_font(s_time_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  update_time(tick_time);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_time_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  s_timer = app_timer_register(1000, fetch_contributions, NULL);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(
      s_main_window,
      (WindowHandlers){.load = main_window_load, .unload = main_window_unload});
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(512, 512);
  prv_load_settings();

  fetch_contributions();
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
  if (s_timer) {
    app_timer_cancel(s_timer);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
