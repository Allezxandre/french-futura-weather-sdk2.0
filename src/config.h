// "c" for temperature in Celsius
// "f" for temperature in Fahrenheit
#define UNIT_SYSTEM "c"

static const char *locale_packages[][2][12] = {
		{{"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"}, { "Jan", "Febr", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"}}, 		// EN = 0
		{{"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"}, { "Janv", "Fevr", "Mars", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"}} 	// FR = 1
	};
