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
	    static char date_text[] = "JJJ 00 XXX";
static char time_text[] = "00:00";

/* Preload the fonts */
GFont font_date;
GFont font_time;



/* Sync Callback
static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case VIBE_ON_HOUR_KEY:
      VibeOnHour = new_tuple->value->uint8;
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received VibeOnHour. Value is %i", VibeOnHour);
      break;
  }
}*/



static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  if (units_changed & MINUTE_UNIT) {
    // Update the time - Deal with 12 / 24 format
    clock_copy_time_string(time_text, sizeof(time_text));
    text_layer_set_text(time_layer, time_text);
	  APP_LOG(APP_LOG_LEVEL_INFO, "VibeOnHour is set to %i", VibeOnHour);
	  int heure = tick_time->tm_hour;
	  if (VibeOnHour == 1) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "VibeOnHour test succesfull!");
  if (((units_changed & HOUR_UNIT) == HOUR_UNIT) && ((heure > 9) && (heure < 23))) {
    vibes_double_pulse();
    APP_LOG(APP_LOG_LEVEL_INFO, "Hour changed -> Vibration complete");
  } else {APP_LOG(APP_LOG_LEVEL_INFO, "However, Hour Unit did not change, no vibration");}
	  
	                      }
  }
	
  if (units_changed & DAY_UNIT) {
    // Update the date - Without a leading 0 on the day of the month
    char day_text[4];
	char month_text[3];
	//strftime(date_text,sizeof(date_text), "%a %d %b", tick_time);
    //text_layer_set_text(date_layer, date_text);
	  
	  // Primitive hack to translate the day of week to another language
			// Needs to be exactly 3 characters, e.g. "Mon" or "Mo "
			// Supported characters: A-Z, a-z, 0-9
			strftime(day_text, sizeof(day_text), "%a", tick_time);
	  
			if (day_text[0] == 'M')
			{
				memcpy(&date_text, "Lun", 3); // Monday
			}
			
			if (day_text[0] == 'T' && day_text[1] == 'u')
			{
				memcpy(&day_text, "Mar", 3); // Tuesday
			}
			
			if (day_text[0] == 'W')
			{
				memcpy(&day_text, "Mer", 3); // Wednesday
			}
			
			if (day_text[0] == 'T' && day_text[1] == 'h')
			{
				memcpy(&day_text, "Jeu", 3); // Thursday
			}
			
			if (day_text[0] == 'F')
			{
				memcpy(&day_text, "Ven", 3); // Friday
			}
			
			if (day_text[0] == 'S' && day_text[1] == 'a')
			{
				memcpy(&day_text, "Sam", 3); // Saturday
			}
			
			if (day_text[0] == 'S' && day_text[1] == 'u')
			{
				memcpy(&day_text, "Dim", 3); // Sunday
			}
			
			strftime(month_text, sizeof(month_text), "%b", tick_time);
			 
			//Primitive Hack to translate month - Only a few are translated because in french, most of the beginnings are similar to english
			if (month_text[0] == 'F')
			{
				memmove(&month_text[0], "Fev", sizeof(month_text)); // Fevrier
			}
			
			if (month_text[0] == 'A' && month_text[2] == 'r')
			{
				memmove(&month_text[0], "Avr", sizeof(month_text)); // Avril
			}
			
			if (month_text[0] == 'M' && month_text[2] == 'y')
			{
				memmove(&month_text[0], "Mai", sizeof(month_text)); // Mai
			}
			
			if (month_text[0] == 'A' && month_text[2] == 'g')
			{
				memmove(&month_text[0], "Aou", sizeof(month_text)); // Aout
			}
			
			
			
			//  strcat(date_text, month_text);	//Don't need it anymore ! 
				
			/*if (date_text[4] == '0') {
			    // Hack to get rid of the leading zero of the day of month
	            memmove(&date_text[4], &date_text[5], sizeof(date_text) - 1);
			    }
			*/
			
			
			// Uncomment the line below if your labels consist of 2 characters and 1 space, e.g. "Mo "
			//memmove(&date_text[3], &date_text[4], sizeof(date_text) - 1);
			
			
    		snprintf(date_text, sizeof(date_text), "%s %i %s", day_text, tick_time->tm_mday, month_text);
			  APP_LOG(APP_LOG_LEVEL_DEBUG, "Displayed time. Vibe is at %i", VibeOnHour);
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
    APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "Set Weather icon. I do this every second, even though I don't fetch weather");
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
