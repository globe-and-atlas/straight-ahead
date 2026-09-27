// Phone side of Straight Ahead: locate the wearer, project the map around them and send it to the
// watch. The watch only rotates and draws what arrives here.
//
// Payload kinds (watch: geo_store.h): coastlines for the day and world views, nearby cities, the
// 36 x 24 region cells and their names. Each goes in DATA chunks keyed by KIND/OFFSET/TOTAL.

var geo = require('./geo');
var geodata = require('./geodata');
var dev = require('./dev.json');  // emulator fixture: {"lat": .., "lon": ..}; {} in releases

var REFRESH_MS = 30 * 60 * 1000;
var CHUNK = 1000;
var MAX_CITIES = 160;
var KIND = { DAY_COAST: 0, WORLD_COAST: 1, CITIES: 2, CELLS: 3, NAMES: 4 };

var queue = [];
var sending = false;

function pump() {
  if (sending || !queue.length) return;
  sending = true;
  var msg = queue.shift();
  Pebble.sendAppMessage(msg, function () {
    sending = false;
    pump();
  }, function (e) {
    console.log('sendAppMessage failed, retrying: ' + JSON.stringify(e && e.error));
    queue.unshift(msg);
    sending = false;
    setTimeout(pump, 1000);
  });
}

function enqueue(kind, bytes) {
  for (var off = 0; off < bytes.length || off === 0; off += CHUNK) {
    queue.push({ KIND: kind, OFFSET: off, TOTAL: bytes.length, DATA: bytes.slice(off, off + CHUNK) });
    if (!bytes.length) break;
  }
}

function sendMap(lat, lon) {
  var t0 = Date.now();
  var day = geo.thin(geo.project(lat, lon, geodata.coast, geo.DAY_SCALE_NM));
  // Thin after projecting, not before: near the rim sparse points read as wrap-arounds.
  var world = geo.thin(geo.project(lat, lon, geodata.coast, geo.WORLD_SCALE_NM));
  var cities = geo.nearbyCities(geodata, lat, lon, MAX_CITIES);
  var regions = geo.regionCells(geodata, lat, lon);
  console.log('map for ' + lat.toFixed(3) + ',' + lon.toFixed(3) + ': day ' + day.length / 2 +
              ' pts, world ' + world.length / 2 + ' pts, ' + cities.length + ' cities, ' +
              regions.names.length + ' names, ' + (Date.now() - t0) + ' ms');
  queue = [];
  queue.push({ LAT_E6: Math.round(lat * 1e6), LON_E6: Math.round(lon * 1e6) });
  enqueue(KIND.DAY_COAST, geo.toBytes(day));
  enqueue(KIND.WORLD_COAST, geo.toBytes(world));
  enqueue(KIND.CITIES, geo.packCities(cities));
  enqueue(KIND.CELLS, regions.cells);
  enqueue(KIND.NAMES, geo.packNames(regions.names));
  pump();
}

function locate() {
  if (typeof dev.lat === 'number' && typeof dev.lon === 'number') {
    sendMap(dev.lat, dev.lon);
    return;
  }
  navigator.geolocation.getCurrentPosition(function (pos) {
    sendMap(pos.coords.latitude, pos.coords.longitude);
  }, function (err) {
    console.log('geolocation failed: ' + err.message);
  }, { timeout: 15000, maximumAge: REFRESH_MS });
}

Pebble.addEventListener('ready', function () {
  locate();
  setInterval(locate, REFRESH_MS);
});
