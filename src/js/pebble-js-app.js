Pebble.addEventListener("ready", function(e) {
  console.log("Starting ...");
  updateWeather();
});

Pebble.addEventListener("appmessage", function(e) {
  console.log("Got a message - Starting weather request...");
  updateWeather();
});

var mConfig = {};

Pebble.addEventListener("ready", function(e) {
        console.log("Starting Configuration fetch");
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
		console.log("Opening URL");
        Pebble.openURL(mConfig.configureUrl);
	
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      console.log("Saved config");
      returnConfigToPebble();
      console.log("Returned config to Pebble");
    }
  }
);

var updateInProgress = false;

function updateWeather() {
  if (!updateInProgress) {
    updateInProgress = true;
    var locationOptions = { "timeout": 15000, "maximumAge": 60000 };
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  }
  else {
    console.log("Not starting a new request. Another one is in progress...");
  }
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  console.log("Got coordinates: " + JSON.stringify(coordinates));
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('Location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({ "error": "Loc unavailable" });
  updateInProgress = false;
}


function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature; //icon, city;
        if (response && response.list && response.list.length > 0) {
          var weatherResult = response.list[0];
          temperature = Math.round(weatherResult.main.temp - 273.15);
          var condition = weatherResult.weather[0].id;

          console.log("Temperature: " + temperature + " Condition: " + condition);
          Pebble.sendAppMessage({
            "condition": condition,
            "temperature": temperature,
			"VibeOnHour":parseInt(mConfig.VibeOnHour)
          });
			console.log("Sent Weather to Pebble");
          updateInProgress = false;
        }
      } else {
        console.log("Error");
        updateInProgress = false;
        Pebble.sendAppMessage({ "error": "HTTP Error" });
      }
    }
  };
  req.send(null);
}



// ————————————————————————————————————————————————————————————————————————         Config Page 


function saveLocalData(config) {

  console.log("saveLocalData() " + JSON.stringify(config));

  localStorage.setItem("VibeOnHour", parseInt(config.VibeOnHour)); 
  
  loadLocalData();

}
function loadLocalData() {
  console.log("Retrieving data, please wait...");
  mConfig.VibeOnHour = parseInt(localStorage.getItem("VibeOnHour"));
  mConfig.configureUrl = "http://pebble.jouand.in/FrenchWeather/index.html";
  console.log("Done !");

  console.log("loadLocalData() " + JSON.stringify(mConfig));
}

function returnConfigToPebble(temperature,condition) {
  console.log("Configuration window returned: " + JSON.stringify(mConfig));
	
	console.log("Temperature: " + temperature + " Condition: " + condition);
          Pebble.sendAppMessage({
            "condition": condition,
            "temperature": temperature,
			"VibeOnHour":parseInt(mConfig.VibeOnHour)
          });
			console.log("Sent Weather to Pebble");
		/*Pebble.sendAppMessage({
    "VibeOnHour":parseInt(mConfig.VibeOnHour)
	});  */
}
