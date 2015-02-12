#include <pebble.h>

#include "weather_layer.h"
#include "network.h"
#include "config.h"

#define DEBUG 1
#ifndef DEBUG
  #pragma message "---- COMPILING IN RELEASE MODE ----"
  #undef APP_LOG
  #define APP_LOG(...)
#endif

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 66, 144, 168-62))

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data;

/* Global variables to keep track of the UI elements */
static Window *window;
static TextLayer *date_layer;
static TextLayer *time_layer;
static WeatherLayer *weather_layer;

// static char date_text[] = "XXX 00";
static char time_text[] = "00:00";

// locale-dependent variables
static int locale_number(char *locale);
//static void load_locale(char *locale_string, char *day_var[], char *month_var[]);
static char locale[] = "XX_XX";
static int locale_int;
static char *day_of_week[7];
static char *month_of_year[12];

static int locale_number(char *locale){  
  // Return locale position in the locale_packages array
  // Only matches 2 first letters
  if(locale)  // ADD YOUR LOCALE ID HERE
          switch(locale[0]) {
            case 'd':   switch(locale[1]) {
                          case 'e': return 2; // de (german) = 2
                        }
                        break;
            case 'e':   switch(locale[1]) {
                          case 'n': return 0; // en (english) = 0
                        }
                        break;
            case 'f':   switch(locale[1]) {
                          case 'r': return 1; // fr (french) = 1
                        }
                        break;
          }
        APP_LOG(APP_LOG_LEVEL_WARNING,"Locale number unknown");
        return 0;
}

/* Preload the fonts */
GFont font_date;
GFont font_time;

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
      // Get the day and month as int
    static int day_int;
      day_int = tick_time->tm_wday;
    static int month_int;
      month_int = tick_time->tm_mon;

    switch (locale_int) { // ADD YOUR DATE FMT HERE
      case 1:
      case 2:
        snprintf(date_text, sizeof(date_text), "%s %01i %s", day_of_week[day_int], tick_time->tm_mday, month_of_year[month_int]); // EU - DAY ## MNTH
        break;
      case 0:
      default:
        snprintf(date_text, sizeof(date_text), "%s %01i", day_of_week[day_int], tick_time->tm_mday); // UK/US/Other - DAY ##
        break;
    }
    text_layer_set_text(date_layer, date_text);
  }

  // Update the bottom half of the screen: icon and temperature
  static int animation_step = 0;
  if (weather_data->updated == 0 && weather_data->error == WEATHER_E_OK)
  {
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
  }
  else {
    // Update the weather icon and temperature
    if (weather_data->error) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_PHONE_ERROR);
    }
    else {
      // Show the temperature as 'stale' if it has not been updated in 30 minutes
      bool stale = false;
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

  // Refresh the weather info every 15 minutes
  if (units_changed & MINUTE_UNIT && (tick_time->tm_min % 15) == 0)
  {
    request_weather();
  }
}

static void init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  weather_data = malloc(sizeof(WeatherData));
  init_network(weather_data);

  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));

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

  // Get language locale
  strcpy(locale,i18n_get_system_locale());
  locale_int = locale_number(locale);
  APP_LOG(APP_LOG_LEVEL_INFO,"Got system locale: %s", locale);
  memcpy(day_of_week, locale_packages[locale_int][0], sizeof(day_of_week));
  memcpy(month_of_year, locale_packages[locale_int][1], sizeof(month_of_year));

  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  // And then every second
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();

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