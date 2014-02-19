#include <pebble.h>

#include "weather_layer.h"
#include "network.h"
#include "config.h"

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 66, 144, 168-62))

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data;

/* Global variables to keep track of the UI elements */
static Window *window;
static TextLayer *date_layer;
static TextLayer *time_layer;
static WeatherLayer *weather_layer;

// Need to be static because pointers to them are stored in the text layers
static char date_text[] = "DAY ## MMMM";
static char time_text[] = "00:00";

/* Preload the fonts */
GFont font_date;
GFont font_time;

// Lists of days and months
          // Translation for DAYS goes here, starting on SUNDAY :
const char *day_of_week[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

          // Translation for MONTHS goes here :
const char *month_of_year[] = { "Janv", "Fevr", "Mars", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"};   



static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  if (units_changed & MINUTE_UNIT) {
    // Update the time - Deal with 12 / 24 format
    clock_copy_time_string(time_text, sizeof(time_text));
	 APP_LOG(APP_LOG_LEVEL_INFO, "-------------- Time is %s --------------",time_text);
    text_layer_set_text(time_layer, time_text);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "VibeOnHour is set to %i", VibeOnHour);
	  	static int heure; 
		 heure = tick_time->tm_hour;
  		if (((units_changed & HOUR_UNIT) == HOUR_UNIT) && ((heure > 9) && (heure < 23)) && (VibeOnHour == 1)) {
    	vibes_double_pulse();
  		}
	  
  }
  
  if (units_changed & DAY_UNIT) { 
    // Get the day and month as int
  static int day_int;
	 day_int = tick_time->tm_wday;
	static int month_int;
	 month_int = tick_time->tm_mon;
    // Print the result
	snprintf(date_text, sizeof(date_text), "%s %i %s", day_of_week[day_int], tick_time->tm_mday, month_of_year[month_int]);
	 APP_LOG(APP_LOG_LEVEL_INFO, "Displayed date : [%s %i %s]", day_of_week[day_int], tick_time->tm_mday, month_of_year[month_int]);
	text_layer_set_text(date_layer, date_text);
  }

  // Update the bottom half of the screen: icon and temperature
  static int animation_step = 0;
  if (weather_data->updated == 0 && weather_data->error == WEATHER_E_OK)
  {
    // 'Animate' loading icon until the first successful weather request
    if (animation_step == 0) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING1);
	  //APP_LOG(APP_LOG_LEVEL_DEBUG, "---Loading...");
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

      // Figure out if it's day or night. Not the best way to do this but the webservice
      // does not seem to return this info.
      bool night_time = false;
      if (tick_time->tm_hour >= 19 || tick_time->tm_hour < 7)
        night_time = true;
    // APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "Set Weather icon. I do this every second, even though I don't fetch weather, and that's normal");
	weather_layer_set_icon(weather_layer, weather_icon_for_condition(weather_data->condition, night_time));
    }
  }

  // Refresh the weather info every 15 minutes
  if (units_changed & MINUTE_UNIT && (tick_time->tm_min % 15) == 0)
  {
	  APP_LOG(APP_LOG_LEVEL_INFO, "Main here, I just requested Weather for timely refresh");
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

	
  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  // And then every second
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
// Prepare Sync	
	//app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),sync_tuple_changed_callback, NULL, NULL);
	 APP_LOG(APP_LOG_LEVEL_DEBUG, "Init finished. Vibe is set to %i", VibeOnHour);
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
