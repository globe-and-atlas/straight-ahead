// Geometry for Straight Ahead's phone side. Pure functions (no Pebble APIs) so node can test them.
//
// Distances are nautical miles on a sphere: 1 arcminute of great circle = 1 nmi, so the Earth's
// radius is 10800 / pi nmi. Bearings are degrees clockwise from true north.

var DEG = Math.PI / 180;
var R_NM = 10800 / Math.PI;
var PEN_UP = -128;           // int8 pair (-128, -128) lifts the pen between polylines
var UNIT = 127;              // projected radius units at the view's scale
var JUMP = 40;               // units; a longer step is a wrap near the rim, not a coastline
var DAY_SCALE_NM = 1440;     // day ring: 24 degrees = 23:59 + 1
var WORLD_SCALE_NM = 10800;  // world: the antipode, 180 degrees
var MAX_POINTS = 1500;       // per view, contract D11

// Great-circle distance (nmi) and initial bearing (deg) from a to b. Haversine for distance so
// short hops stay accurate.
function inverse(lat1, lon1, lat2, lon2) {
  var p1 = lat1 * DEG, p2 = lat2 * DEG, dl = (lon2 - lon1) * DEG;
  var h = Math.pow(Math.sin((p2 - p1) / 2), 2) +
          Math.cos(p1) * Math.cos(p2) * Math.pow(Math.sin(dl / 2), 2);
  var d = 2 * Math.asin(Math.min(1, Math.sqrt(h)));
  var b = Math.atan2(Math.sin(dl) * Math.cos(p2),
                     Math.cos(p1) * Math.sin(p2) - Math.sin(p1) * Math.cos(p2) * Math.cos(dl));
  return { nm: d * R_NM, bearing: (b / DEG + 360) % 360 };
}

// Destination from a along bearing (deg) for distance nm.
function forward(lat, lon, bearing, nm) {
  var p1 = lat * DEG, d = nm / R_NM, b = bearing * DEG;
  var p2 = Math.asin(Math.sin(p1) * Math.cos(d) + Math.cos(p1) * Math.sin(d) * Math.cos(b));
  var l2 = lon * DEG + Math.atan2(Math.sin(b) * Math.sin(d) * Math.cos(p1),
                                   Math.cos(d) - Math.sin(p1) * Math.sin(p2));
  var lonOut = ((l2 / DEG + 540) % 360) - 180;
  return { lat: p2 / DEG, lon: lonOut };
}

// Azimuthal-equidistant projection of polylines ([[lon, lat], ...]) around (lat, lon).
// Returns a flat int8 array of x, y pairs (north up, y down), PEN_UP pairs between runs.
function project(lat, lon, polylines, scaleNm) {
  var out = [];
  var points = 0;
  for (var i = 0; i < polylines.length; i++) {
    var line = polylines[i];
    var prev = null;
    for (var j = 0; j < line.length; j++) {
      var g = inverse(lat, lon, line[j][1], line[j][0]);
      if (g.nm > scaleNm) { prev = null; continue; }
      var r = g.nm / scaleNm * UNIT;
      var x = Math.round(r * Math.sin(g.bearing * DEG));
      var y = Math.round(-r * Math.cos(g.bearing * DEG));
      if (prev && (Math.abs(x - prev[0]) > JUMP || Math.abs(y - prev[1]) > JUMP)) prev = null;
      if (prev && x === prev[0] && y === prev[1]) continue;
      if (!prev) out.push(PEN_UP, PEN_UP);
      out.push(x, y);
      points++;
      prev = [x, y];
    }
  }
  return { bytes: out, points: points };
}

// Keep every k-th point so a view never exceeds MAX_POINTS (pen-up markers survive).
function thin(proj) {
  if (proj.points <= MAX_POINTS) return proj.bytes;
  var k = Math.ceil(proj.points / MAX_POINTS), out = [], n = 0;
  for (var i = 0; i < proj.bytes.length; i += 2) {
    var up = proj.bytes[i] === PEN_UP && proj.bytes[i + 1] === PEN_UP;
    if (up || n++ % k === 0) out.push(proj.bytes[i], proj.bytes[i + 1]);
  }
  return out;
}

function gridName(geodata, lat, lon) {
  var y = Math.min(geodata.grid.length - 1, Math.floor((90 - lat) / geodata.cell));
  var x = Math.floor((lon + 180) / geodata.cell) % Math.round(360 / geodata.cell);
  var row = geodata.grid[y], seen = 0;
  for (var i = 0; i < row.length; i += 2) {
    seen += row[i + 1];
    if (x < seen) return geodata.names[row[i]];
  }
  return "";
}

// The `limit` largest cities within the day disc (plus the label window), nearest first, as
// [distance nmi (u16), bearing tenths (u16), name] records. geodata.cities is largest first.
function nearbyCities(geodata, lat, lon, limit) {
  var found = [];
  for (var i = 0; i < geodata.cities.length && found.length < limit; i++) {
    var c = geodata.cities[i];
    var g = inverse(lat, lon, c[1], c[2]);
    if (g.nm <= DAY_SCALE_NM + 25) found.push([Math.round(g.nm), Math.round(g.bearing * 10) % 3600, c[0]]);
  }
  found.sort(function (a, b) { return a[0] - b[0]; });
  return found.slice(0, limit);
}

// 36 bearings x 24 bands: the region at bearing 10*b, distance 60*k + 30 nmi.
function regionCells(geodata, lat, lon) {
  var names = [""], index = { "": 0 }, cells = [];
  for (var b = 0; b < 36; b++) {
    for (var k = 0; k < 24; k++) {
      var p = forward(lat, lon, b * 10, k * 60 + 30);
      var n = gridName(geodata, p.lat, p.lon) || "";
      if (!(n in index)) {
        if (names.length >= 255) n = "";
        else { index[n] = names.length; names.push(n); }
      }
      cells.push(index[n]);
    }
  }
  return { cells: cells, names: names };
}

// ---- Byte packing for AppMessage -----------------------------------------------------------

function packCities(records) {
  var out = [];
  records.forEach(function (r) {
    var name = r[2].slice(0, 22);
    out.push(r[0] & 0xFF, r[0] >> 8, r[1] & 0xFF, r[1] >> 8, name.length);
    for (var i = 0; i < name.length; i++) out.push(name.charCodeAt(i) & 0x7F);
  });
  return out;
}

function packNames(names) {
  var out = [];
  names.forEach(function (n) {
    for (var i = 0; i < n.length; i++) out.push(n.charCodeAt(i) & 0x7F);
    out.push(0);
  });
  return out;
}

// int8 values to unsigned bytes for AppMessage byte arrays.
function toBytes(values) {
  return values.map(function (v) { return v & 0xFF; });
}

module.exports = {
  R_NM: R_NM, UNIT: UNIT, PEN_UP: PEN_UP, DAY_SCALE_NM: DAY_SCALE_NM,
  WORLD_SCALE_NM: WORLD_SCALE_NM, MAX_POINTS: MAX_POINTS,
  inverse: inverse, forward: forward, project: project, thin: thin, gridName: gridName,
  nearbyCities: nearbyCities, regionCells: regionCells,
  packCities: packCities, packNames: packNames, toBytes: toBytes
};
