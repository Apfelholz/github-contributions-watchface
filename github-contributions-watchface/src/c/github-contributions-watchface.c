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
} ClaySettings;

static ClaySettings settings;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect frame = grect_inset(bounds, GEdgeInsets(0));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  int cell_size = (bounds.size.w / 7) - 1;
  int coner_radius = 5;

  int x = 2;
  int y = 0;
  for (int week = 0; week < 7; week++) {
    for (int day = 0; day < 7; day++) {
      uint8_t contributions = s_contributions[week*7+day];
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
      graphics_fill_rect(ctx, cell, coner_radius, GCornersAll);

      x += cell_size + 1;
    }
    x = 2;
    y += cell_size + 1;
  }
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void fetch_contributions() {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    dict_write_cstring(out_iter, MESSAGE_KEY_KEY_GITHUB_USERNAME, settings.github_username);
    dict_write_cstring(out_iter, MESSAGE_KEY_KEY_GITHUB_TOKEN, settings.github_token);
    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending request: %d", result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error beginning message: %d", result);
  }

  s_timer = app_timer_register(1 * 20 * 1000, fetch_contributions, NULL);
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings(void) {
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *contributions_tuple = dict_find(iter, MESSAGE_KEY_KEY_CONTRIBUTIONS);
  Tuple *username_tuple = dict_find(iter, MESSAGE_KEY_KEY_GITHUB_USERNAME);
  Tuple *token_tuple = dict_find(iter, MESSAGE_KEY_KEY_GITHUB_TOKEN);

  if (username_tuple) {
    snprintf(settings.github_username, sizeof(settings.github_username), "%s", username_tuple->value->cstring);
  }

  if (token_tuple) {
    snprintf(settings.github_token, sizeof(settings.github_token), "%s", token_tuple->value->cstring);
  }

  if (username_tuple || token_tuple) {
    prv_save_settings();
    fetch_contributions();
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

  s_time_layer = text_layer_create(GRect(0, bounds.size.h - 35, bounds.size.w, 30));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
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
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
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
