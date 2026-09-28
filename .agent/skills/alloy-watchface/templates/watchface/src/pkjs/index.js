// Phone-side JS. Clay serves the settings page; pebbleproxy forwards the
// watch's fetch()/Location traffic. They listen to different events, so both
// can coexist — Clay first, proxy second.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
