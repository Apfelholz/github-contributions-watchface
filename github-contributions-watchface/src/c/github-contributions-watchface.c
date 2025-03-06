#include <pebble.h>

#define KEY_CONTRIBUTIONS 0

static Window *s_main_window;
static TextLayer *s_time_layer;
static int s_contributions[7][7];
static AppTimer *s_timer;
static char contributions_text[1024];
static uint8_t s_buffer[7 * 7 * 4];

static void update_layer(Layer *layer, GContext *ctx) {
  int index_ptr = *(int *)layer_get_data(layer);
  int index = index_ptr;
  int i = index / 7;
  int j = index % 7;
  int contributions = s_contributions[i][j];
  int green_value = contributions > 255 ? 255 : contributions; // Cap the green value at 255

  graphics_context_set_fill_color(ctx, GColorFromRGB(0, green_value, 0));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void update_background_color() {
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      GRect frame = GRect(j * (bounds.size.w / 7), i * (bounds.size.h / 7), bounds.size.w / 7, bounds.size.h / 7);
      Layer *bitmap_layer = layer_create_with_data(frame, sizeof(int));
      int *index_ptr = (int *)layer_get_data(bitmap_layer);
      *index_ptr = i * 7 + j;

      layer_set_update_proc(bitmap_layer, update_layer);
      layer_add_child(window_layer, bitmap_layer);
    }
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
  s_timer = app_timer_register(5 * 60 * 1000, fetch_contributions, NULL);
}

static void main_window_load(Window *window){
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  update_background_color();
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
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
    update_background_color();
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
  update_time();

  app_message_register_inbox_received(inbox_received_callback);
  
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Start fetching contributions
  fetch_contributions();
}

static void deinit() {
  tick_timer_service_unsubscribe();
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