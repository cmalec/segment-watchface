var mConfig = {};
var mConfigURL = "https://cmalec.github.io/segment-watchface/server/index.10.html";
//var mConfigURL = "http://localhost:8080/index.10.html";

// Weather: Open-Meteo, free, no API key. Day high/low for the phone's
// location, refreshed hourly. Last values are cached in localStorage with a
// fetchedAt timestamp; caches older than STALE_MS are refetched rather than
// served to the watch.
var WEATHER_REFRESH_MS = 60 * 60 * 1000;
var STALE_MS = 3 * 60 * 60 * 1000;
var FETCH_RETRY_MIN_MS = 5 * 60 * 1000; // don't hammer geolocation/API on watch re-requests
var weatherTimer = null;
var lastWeather = null;
var lastSentHi = null;
var lastSentLo = null;
var lastFetchAttemptAt = 0;
var fetchInFlight = false;
var settings = {}; // watch settings mirrored here; temp conversion needs temp_unit

// Phone battery: sent to the watch for the thin bar under the watch's own
// battery icon. Uses the browser Battery Status API, which is NOT available
// everywhere (notably absent on iOS WebKit) — feature-detected, and the
// watch hides the bar until/unless data arrives.
var phoneBattery = null;

Pebble.addEventListener("ready", function(e) {
  loadLocalData();
  loadWeatherCache();
  returnConfigToPebble();
  initPhoneBattery();
  if (lastWeather && !isStale(lastWeather)) {
    sendWeatherToWatch();
  }
  fetchWeather();          // first fetch as early as possible
  scheduleWeatherRefresh();
});

// The watch asks for weather by sending wtemp_req=1.
Pebble.addEventListener('appmessage', function(e) {
    if (!e.payload) return;
    if (e.payload.temp_unit !== undefined && e.payload.temp_unit !== settings.temp_unit) {
      // Watch shares its unit setting; re-send temps in the new unit.
      settings.temp_unit = e.payload.temp_unit;
      console.log('settings: temp_unit=' + settings.temp_unit);
      if (lastWeather) sendWeatherToWatch(true);
    }
    if (e.payload.wtemp_req === 1) {
      handleWeatherRequest();
    }
});

Pebble.addEventListener("showConfiguration", function(e) {
	var url = mConfigURL + '?';
	if(Pebble.getActiveWatchInfo) {
		url += 'platform=' + Pebble.getActiveWatchInfo().platform + '&';
	}
	// Hydrate the page from the same settings blob the watch runs on, so
	// toggles open in their actual current state (not localStorage/default).
	url += 'settings=' + encodeURIComponent(JSON.stringify(mConfig || {}));
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
    if (!e.response) {
      return;
    }
    var config;
    try {
      config = JSON.parse(decodeURIComponent(e.response));
    }
    catch (err) {
      console.log('config: malformed response, keeping existing settings (' + err + ')');
      return;
    }
    if (config === null || typeof config !== 'object' || Array.isArray(config)) {
      console.log('config: response is not an object, keeping existing settings');
      return;
    }
    // Primary path for the temp unit: the settings page itself knows the
    // choice, so take it now instead of waiting for the watch's echo.
    if (typeof config.temp_unit === 'number') {
      settings.temp_unit = config.temp_unit;
      console.log('settings: temp_unit=' + settings.temp_unit + ' (from config page)');
    }
    saveLocalData(config);
    returnConfigToPebble();
});

function loadLocalData() {
	try {
		mConfig = JSON.parse(localStorage.getItem("config"));
	}
	catch(err) {
		mConfig = {};
	}
	if (mConfig === null || typeof mConfig !== 'object' || Array.isArray(mConfig)) {
		mConfig = {};
	}
}

function saveLocalData(config) {
	localStorage.setItem("config", JSON.stringify(config));
  loadLocalData();
}

function returnConfigToPebble() {
	if(mConfig === null || typeof mConfig !== 'object') {
		mConfig = {};
	}
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage(mConfig, function() {}, function(e) {
    // Common right after 'ready' (APP_MSG_BUSY while the watch boots): retry
    // once after a delay; config is never re-requested by the watch, so a
    // silently dropped send would leave stale settings.
    console.log('config: send failed (' + e.error.message + '), retrying once');
    setTimeout(function() {
      Pebble.sendAppMessage(mConfig, function() {}, function(e2) {
        console.log('config: retry failed (' + e2.error.message + ')');
      });
    }, 2000);
  });
}

function loadWeatherCache() {
  try {
    lastWeather = JSON.parse(localStorage.getItem('weather'));
  }
  catch (err) {
    lastWeather = null;
  }
  if (lastWeather === null || typeof lastWeather !== 'object' ||
      typeof lastWeather.hi !== 'number' || !isFinite(lastWeather.hi) ||
      typeof lastWeather.lo !== 'number' || !isFinite(lastWeather.lo)) {
    lastWeather = null;
  }
}

function isStale(w) {
  return !w || typeof w.fetchedAt !== 'number' || (Date.now() - w.fetchedAt) > STALE_MS;
}

/*
 * Weather — day high/low via Open-Meteo (https://open-meteo.com), no key.
 * Position comes from the browser geolocation API when permitted; if that
 * fails (no permission, timeout), we fall back to an IP-based lookup
 * (ipapi.co, no key) which is coarse (city-level) but always available for
 * a weather forecast.
 */
function fetchWeather() {
  if (fetchInFlight) {
    return;
  }
  if (!navigator.geolocation) {
    console.log('weather: geolocation unavailable, using IP fallback');
    fetchWeatherIp();
    return;
  }
  fetchInFlight = true;
  lastFetchAttemptAt = Date.now();
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      fetchInFlight = false;
      fetchWeatherFor(pos.coords.latitude.toFixed(4), pos.coords.longitude.toFixed(4));
    },
    function(err) {
      fetchInFlight = false;
      console.log('weather: geolocation error (' + err.message + '), trying IP fallback');
      if (err.code === err.PERMISSION_DENIED) {
        // Stop hammering a denied permission: switch the hourly timer over
        // to the IP fallback (coarse but works without any permission).
        if (weatherTimer) { clearInterval(weatherTimer); }
        weatherTimer = setInterval(fetchWeatherIp, WEATHER_REFRESH_MS);
        // One immediate IP-based attempt now.
        fetchWeatherIp();
      }
      else {
        fetchWeatherIp();
      }
    },
    { timeout: 15000, maximumAge: 15 * 60 * 1000 }
  );
}

function fetchWeatherIp() {
  if (fetchInFlight) {
    return;
  }
  fetchInFlight = true;
  lastFetchAttemptAt = Date.now();
  var req = new XMLHttpRequest();
  req.timeout = 8000;
  req.onload = function() {
    fetchInFlight = false;
    if (req.status !== 200) {
      console.log('weather: IP lookup HTTP ' + req.status);
      return;
    }
    try {
      var loc = JSON.parse(req.responseText);
      if (typeof loc.latitude !== 'number' || typeof loc.longitude !== 'number') {
        console.log('weather: IP lookup returned no coordinates');
        return;
      }
      fetchWeatherFor(loc.latitude.toFixed(4), loc.longitude.toFixed(4));
    } catch (err) {
      console.log('weather: IP lookup parse error ' + err);
    }
  };
  req.ontimeout = function() { fetchInFlight = false; console.log('weather: IP lookup timeout'); };
  req.onerror = function() { fetchInFlight = false; console.log('weather: IP lookup error'); };
  req.open('GET', 'https://ipapi.co/json/', true);
  req.send();
}

function fetchWeatherFor(lat, lon) {
  if (fetchInFlight) {
    return;
  }
  // Open-Meteo has no "return both units" mode, so we cache CELSIUS (its
  // native unit) and convert at send time. Unit flips are instant.
  fetchInFlight = true;
  var url = 'https://api.open-meteo.com/v1/forecast' +
            '?latitude=' + lat + '&longitude=' + lon +
            '&daily=temperature_2m_max,temperature_2m_min' +
            '&forecast_days=1&timezone=auto';
  var req = new XMLHttpRequest();
  req.timeout = 10000;
  req.onload = function() {
    fetchInFlight = false;
    if (req.status !== 200) {
      console.log('weather: HTTP ' + req.status);
      return;
    }
    try {
      var resp = JSON.parse(req.responseText);
      var daily = resp.daily;
      var hiRaw = daily && daily.temperature_2m_max ? daily.temperature_2m_max[0] : undefined;
      var loRaw = daily && daily.temperature_2m_min ? daily.temperature_2m_min[0] : undefined;
      if (typeof hiRaw !== 'number' || !isFinite(hiRaw) ||
          typeof loRaw !== 'number' || !isFinite(loRaw)) {
        console.log('weather: unexpected response shape');
        return;
      }
      lastWeather = {
        hi: Math.round(hiRaw),
        lo: Math.round(loRaw),
        fetchedAt: Date.now()
      };
      localStorage.setItem('weather', JSON.stringify(lastWeather));
      sendWeatherToWatch();
    } catch (err) {
      console.log('weather: parse error ' + err);
    }
  };
  req.ontimeout = function() { fetchInFlight = false; console.log('weather: request timeout'); };
  req.onerror = function() { fetchInFlight = false; console.log('weather: request error'); };
  req.open('GET', url, true);
  req.send();
}

function handleWeatherRequest() {
  console.log('weather: watch requested temps (cache: ' +
    (lastWeather ? lastWeather.hi + '/' + lastWeather.lo + ' @ ' + new Date(lastWeather.fetchedAt).toISOString() : 'none') + ')');
  if (lastWeather && !isStale(lastWeather)) {
    // Fresh cache (covers watch relaunches and missed sends). An explicit
    // re-request means the watch did NOT receive the earlier send (e.g. it
    // relaunched) — bypass the duplicate check, it's per-phone-context but
    // delivery is per-app-launch.
    sendWeatherToWatch(true);
  }
  else if (Date.now() - lastFetchAttemptAt > FETCH_RETRY_MIN_MS) {
    // Stale or no cache: trigger a fresh fetch instead of serving old data.
    fetchWeather();
  }
}

function sendWeatherToWatch(force) {
  if (!lastWeather || typeof lastWeather.hi !== 'number' || typeof lastWeather.lo !== 'number') {
    console.log('weather: nothing to send (no valid cache)');
    return; // nothing fetched (or partial cache) yet
  }
  // Convert cached Celsius to the configured unit at send time. The
  // conversion is INLINED: standalone top-level helpers can be renamed away
  // by the webpack bundler (the "cToF is not defined" ReferenceError).
  var fahrenheit = settings.temp_unit === 1;
  var hi = fahrenheit ? Math.round(lastWeather.hi * 9 / 5 + 32) : lastWeather.hi;
  var lo = fahrenheit ? Math.round(lastWeather.lo * 9 / 5 + 32) : lastWeather.lo;
  // Skip duplicate sends with identical values — EXCEPT when the watch
  // explicitly re-requested (force), which means it never got the last one.
  if (!force && hi === lastSentHi && lo === lastSentLo) {
    console.log('weather: skipping duplicate send ' + hi + '/' + lo);
    return;
  }
  Pebble.sendAppMessage(
    { wtemp_hi: hi, wtemp_lo: lo },
    function() {
      console.log('weather: sent to watch ' + hi + '/' + lo + ' (' + (fahrenheit ? 'F' : 'C') + ')');
      lastSentHi = hi;
      lastSentLo = lo;
    },
    function(e) {
      console.log('weather: send failed ' + e.error.message);
    }
  );
}

function scheduleWeatherRefresh() {
  if (weatherTimer) { clearInterval(weatherTimer); }
  weatherTimer = setInterval(fetchWeather, WEATHER_REFRESH_MS);
}

/*
 * Phone battery -> watch bar. Battery Status API (navigator.getBattery) is
 * Chromium/Android-only in practice; on iOS it does not exist and the watch
 * bar simply never appears, which is the correct degraded behavior.
 */
function initPhoneBattery() {
  if (!navigator.getBattery) {
    console.log('pbatt: Battery Status API unavailable');
    return;
  }
  navigator.getBattery().then(function(batt) {
    phoneBattery = batt;
    sendPhoneBattery();
    batt.addEventListener('levelchange', sendPhoneBattery);
  }).catch(function(err) {
    console.log('pbatt: getBattery rejected ' + err);
  });
}

function sendPhoneBattery() {
  if (!phoneBattery) {
    return;
  }
  var pct = Math.round(phoneBattery.level * 100);
  if (!isFinite(pct)) {
    return;
  }
  Pebble.sendAppMessage(
    { pbatt_level: pct },
    function() {},
    function(e) {
      console.log('pbatt: send failed ' + e.error.message);
    }
  );
}
