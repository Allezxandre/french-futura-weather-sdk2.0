#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITION 1
#define KEY_ERROR 2
#define KEY_REQUEST_UPDATE 42
#define VIBE_ON_HOUR_KEY 5
/*
 //Sync positions
static AppSync sync;
static uint8_t sync_buffer[64];
enum {
	VIBE_ON_HOUR_KEY = 5
};
*/
static int VibeOnHour = -1;
	

typedef enum {
  WEATHER_E_OK = 0,
  WEATHER_E_DISCONNECTED,
  WEATHER_E_PHONE,
  WEATHER_E_NETWORK
} WeatherError;

typedef struct {
  int temperature;
  int condition;
  time_t updated;
  WeatherError error;
} WeatherData;

void init_network(WeatherData *weather_data);
void close_network();

void request_weather();
