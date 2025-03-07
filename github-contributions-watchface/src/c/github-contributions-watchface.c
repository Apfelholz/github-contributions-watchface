#include <pebble.h>

#define KEY_CONTRIBUTIONS 0

static Window *s_main_window;
static TextLayer *s_time_layer;
static int s_contributions[7][7];
static AppTimer *s_timer;
static uint8_t s_buffer[7 * 7 * 4];
static Layer *s_canvas_layer; 

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
      int contributions = s_contributions[week][day];
      GColor color = GColorWhite;
      if (contributions == 0) {
        color = GColorDarkGray;
      } else if (contributions < 3) {
        color = GColorDarkGreen;
      } else if (contributions < 6) {
        color = GColorIslamicGreen;
      } else if (contributions < 30) {
        color = GColorGreen;
      } else {
        color = GColorWhite;
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
  // Send a message to the JavaScript to fetch contributions
  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    dict_write_uint8(out_iter, KEY_CONTRIBUTIONS, 0);
    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }

  // Schedule the next fetch in 5 minutes
  s_timer = app_timer_register(1 * 10 * 1000, fetch_contributions, NULL);
}

static void main_window_load(Window *window){
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_set_frame(text_layer_get_layer(s_time_layer), GRect(1, bounds.size.h - 35, bounds.size.w - 2, 30));

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_canvas_layer);
}

// Callback-Funktion zum Empfangen der Nachricht von JavaScript
static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // Überprüfe, ob die Nachricht die erwarteten binären Daten enthält
  Tuple *data_tuple = dict_find(iter, KEY_CONTRIBUTIONS);
  if (data_tuple) {
    // Lese die binären Daten (Uint8Array) und speichere sie in den Puffer
    uint8_t *data = data_tuple->value->data;

    // Entpacke die Daten und speichere sie im 2D-Array contributions[7][7]
    int index = 0;
    for (int week = 0; week < 7; week++) {
      for (int day = 0; day < 7; day++) {
        // Entpacke den 32-Bit Integer (4 Bytes)
        s_contributions[week][day] = 
          (data[index] << 24) | 
          (data[index + 1] << 16) | 
          (data[index + 2] << 8) | 
          data[index + 3];

        index += 4;  // Wechsle zu den nächsten 4 Bytes
      }
    }

    // Log the contributions array
    for (int week = 0; week < 7; week++) {
      for (int day = 0; day < 7; day++) {
        APP_LOG(APP_LOG_LEVEL_INFO, "s_contributions[%d][%d] = %d", week, day, s_contributions[week][day]);
      }
    }

    layer_mark_dirty(s_canvas_layer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
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

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler); // Subscribe to the tick timer service

  app_message_register_inbox_received(inbox_received_callback);
  
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Start fetching contributions
  fetch_contributions();
  layer_mark_dirty(s_canvas_layer);
}

static void deinit() {
  tick_timer_service_unsubscribe(); // Unsubscribe from the tick timer service
  window_destroy(s_main_window);
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}