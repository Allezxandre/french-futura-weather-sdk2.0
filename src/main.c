#include <pebble.h>

#include "weather_layer.h"
#include "network.h"

// #define DEBUG 1
#ifndef DEBUG
  #pragma message "---- COMPILING IN RELEASE MODE ----"
  #undef APP_LOG
  #define APP_LOG(...)
#endif

#define TIME_FRAME      (GRect(0, 0, 144, 72))
#define DATE_FRAME      (GRect(1, 72, 143, 20))

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data;

/* Global variables to keep track of the UI elements */
static Window *window;
static TextLayer *date_layer;
static TextLayer *time_layer;
static WeatherLayer *weather_layer;

/* Timers */
static AppTimer* update_timer;
static void update_and_animate(void *data);
static int multiplier = 1; // used for failed updates

/* Bluetooth connection */
static bool bluetooth_connected;
static bool stale = false;
void bluetooth_connection_handler(bool connected){
  bluetooth_connected = connected;
  if (connected)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Back online. Let's update");
    if (update_timer != NULL) {
      app_timer_cancel(update_timer);
      update_timer = NULL;
      update_timer = app_timer_register(5000,update_and_animate,NULL);
    } else {
      update_timer = app_timer_register(5000,update_and_animate,NULL);
    }
    stale = false;
  } else {
    stale = true;
    weather_layer_set_temperature(weather_layer, weather_data->temperature, stale);
  }
}

// static char date_text[] = "XXX 00";
static char time_text[] = "00:00";

/* Preload the fonts */
GFont font_date;
GFont font_time;

static void animate_update(){
  // Update the bottom half of the screen: icon and temperature
  static int animation_step = 0;
  if (weather_data->updated == 0 && weather_data->error == WEATHER_E_OK) {
    // 'Animate' loading icon until the first successful weather request
    if (animation_step == 0) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING1);
    }
    else if (animation_step == 1) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING2);
    }
    else if (animation_step >= 2) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING3);
    }
    animation_step = (animation_step + 1) % 3;
  } else {
    if (weather_data->error) { // Error -> Update the weather icon and temperature
      weather_layer_set_icon(weather_layer, WEATHER_ICON_PHONE_ERROR);
      if (bluetooth_connected && (multiplier <= 10)) { // If bluetooth is connected, schedule refresh in 1 minute
          if (update_timer != NULL) {
            app_timer_cancel(update_timer);
            update_timer = NULL;
            update_timer = app_timer_register(multiplier * 60000,update_and_animate,NULL);
          } else {
            update_timer = app_timer_register(multiplier * 60000,update_and_animate,NULL);
          }
          multiplier *= 2.5;
      }
    } else { // No Error
      multiplier = 1;
      // Show the temperature as 'stale' if it has not been updated in 30 minutes
      if (weather_data->updated > time(NULL) + 1800) {
        stale = true;
      }
      weather_layer_set_temperature(weather_layer, weather_data->temperature, stale);

      // Day/night check
      bool night_time = false;
      if (weather_data->current_time < weather_data->sunrise || weather_data->current_time > weather_data->sunset)
        night_time = true;
      weather_layer_set_icon(weather_layer, weather_icon_for_condition(weather_data->condition, night_time));
    }
  }
}

static void update_and_animate(void *data) {
  if (bluetooth_connected) {
    weather_data->updated = 0;
    animate_update();
    request_weather(NULL);
  }
  update_timer = NULL;
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  if (units_changed & MINUTE_UNIT) {
    // Update the time - Fix to deal with 12 / 24 centering bug
    time_t currentTime = time(0);
    struct tm *currentLocalTime = localtime(&currentTime);

    // Manually format the time as 12 / 24 hour, as specified
    strftime(   time_text, 
                sizeof(time_text), 
                clock_is_24h_style() ? "%R" : "%I:%M", 
                currentLocalTime);

    // Drop the first char of time_text if needed
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }

    text_layer_set_text(time_layer, time_text);
  }
  if (units_changed & DAY_UNIT) {
    // Update the date - Without a leading 0 on the day of the month
    static char date_text[] = "DAY 00 MOIS";
    // Get locale
    char *sys_locale = setlocale(LC_ALL, "");
    // Set the DateLayer
    if (strcmp("fr_FR", sys_locale) == 0) {
      strftime(date_text, sizeof(date_text), "%a %e %b", tick_time);
    } else if (strcmp("de_DE", sys_locale) == 0) {
      strftime(date_text, sizeof(date_text), "%a %e %b", tick_time);
    } else if (strcmp("es_ES", sys_locale) == 0) {
      strftime(date_text, sizeof(date_text), "%a %e", tick_time);
    } else {
      // Fall back to English (the font doesn't support Chinese)
      strftime(date_text, sizeof(date_text), "%a %e", tick_time);
    }
    // Put layer
    text_layer_set_text(date_layer, date_text);
  }

  if (bluetooth_connected)
  {
    animate_update();
  } else if (weather_data->updated == 0) {
    weather_layer_set_icon(weather_layer, WEATHER_ICON_PHONE_ERROR);
  }
  // Refresh the weather info every 15 minutes
  if (units_changed & MINUTE_UNIT && (tick_time->tm_min % 20) == 0)
  {
    request_weather(NULL);
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Received TAP");
  if (update_timer != NULL) {
    app_timer_cancel(update_timer);
    update_timer = NULL;
    update_timer = app_timer_register(2000,update_and_animate,NULL);
  } else {
    update_timer = app_timer_register(2000,update_and_animate,NULL);
  }
}

static void init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  weather_data = malloc(sizeof(WeatherData));
  init_network(weather_data);

  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_UBUNTU_REGULAR_18));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_UBUNTU_CONDENSED_70));

  time_layer = text_layer_create(TIME_FRAME);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, font_time);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  date_layer = text_layer_create(DATE_FRAME);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, font_date);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  // Add weather layer
  weather_layer = weather_layer_create(GRect(0, 90, 144, 80));
  layer_add_child(window_get_root_layer(window), weather_layer);

  // Subscribe to the Bluetooth service 
  bluetooth_connected = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  // And then every second
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  // subscribe to TAP
  accel_tap_service_subscribe(tap_handler);
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();

  if (update_timer != NULL) {
     app_timer_cancel(update_timer);
     update_timer = NULL;
  }

  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  weather_layer_destroy(weather_layer);

  fonts_unload_custom_font(font_date);
  fonts_unload_custom_font(font_time);

  free(weather_data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}