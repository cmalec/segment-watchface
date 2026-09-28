// Phone-side JS (PebbleKit JS): serves the Clay settings page and fetches
// Open-Meteo weather for the watch. Runs only while the watchface is open.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);   // handles showConfiguration / webviewClosed

function xhrRequest(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
}

// Open-Meteo WMO codes → short labels (the watch displays these verbatim)
function weatherCodeToCondition(code) {
  if (code === 0) return 'Clear';
  if (code <= 3) return 'Cloudy';
  if (code <= 48) return 'Fog';
  if (code <= 55) return 'Drizzle';
  if (code <= 57) return 'Fz. Drizzle';
  if (code <= 65) return 'Rain';
  if (code <= 67) return 'Fz. Rain';
  if (code <= 75) return 'Snow';
  if (code <= 77) return 'Snow Grains';
  if (code <= 82) return 'Showers';
  if (code <= 86) return 'Snow Shwrs';
  if (code === 95) return 'T-Storm';
  if (code <= 99) return 'T-Storm';
  return 'Unknown';
}

function locationSuccess(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast?' +
      'latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&current=temperature_2m,weather_code';

  xhrRequest(url, 'GET', function (responseText) {
    var json = JSON.parse(responseText);

    // Always Celsius on the wire; the watch converts for Fahrenheit
    var temperature = Math.round(json.current.temperature_2m);
    var conditions = weatherCodeToCondition(json.current.weather_code);

    Pebble.sendAppMessage(
      { 'TEMPERATURE': temperature, 'CONDITIONS': conditions },
      function () { console.log('Weather sent: ' + temperature + '°C ' + conditions); },
      function () { console.log('Error sending weather'); }
    );
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready!');
  getWeather();               // initial fetch when the watchface opens
});

Pebble.addEventListener('appmessage', function (e) {
  // The watch asks for a refresh (every 30 min, and after a settings change)
  if (e.payload['REQUEST_WEATHER']) {
    getWeather();
  }
});
