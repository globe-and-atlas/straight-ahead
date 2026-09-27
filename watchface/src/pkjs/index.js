// Phone side of Straight Ahead. Scaffold stage: send the wearer's location. Later milestones add
// the azimuthal-equidistant coastline projection, landfall labels and magnetic declination
// (see task.md), all computed here so the watch only rotates what it receives.

var REFRESH_MS = 30 * 60 * 1000;

function sendFix(position) {
  Pebble.sendAppMessage({
    LAT_E6: Math.round(position.coords.latitude * 1e6),
    LON_E6: Math.round(position.coords.longitude * 1e6)
  }, null, function (e) {
    console.log('sendAppMessage failed: ' + JSON.stringify(e));
  });
}

function locate() {
  navigator.geolocation.getCurrentPosition(sendFix, function (err) {
    console.log('geolocation failed: ' + err.message);
  }, { timeout: 15000, maximumAge: REFRESH_MS });
}

Pebble.addEventListener('ready', function () {
  locate();
  setInterval(locate, REFRESH_MS);
});
