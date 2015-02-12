// "c" for temperature in Celsius
// "f" for temperature in Fahrenheit
#define UNIT_SYSTEM "c"

static const char *locale_packages[][2][12] = { // Do not forget to add your locale ID in main.c
		{{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}, { "Jan", "Febr", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"}}, 		// EN = 0
		{{"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"}, { "Janv", "Fevr", "Mars", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"}}, 	// FR = 1
		{{"Son", "Mon", "Die","Mit", "Don", "Fre", "Sam"}, { "Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"}} 	// DE = 2		
	};
