#include "BluetoothSerial.h"
#include <WiFi.h>
#include <WebServer.h>

BluetoothSerial SerialBT;
WebServer server(80);
HardwareSerial GPS(2);

// =====================================================
// POTTATO WORLD
// ESP32 + GPS + HC-SR04 + Bluetooth Authentication
// =====================================================


// =====================================================
// PIN CONFIGURATION
// =====================================================

// GPS
#define GPS_RX 16
#define GPS_TX 17

// HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18


// =====================================================
// WIFI
// =====================================================

const char* ssid = "POTTATO_WORLD";
const char* password = "12345678";


// =====================================================
// POTTATO PERMANENT LOCATION
// =====================================================

const double BASE_LAT = 9.981600;
const double BASE_LON = 76.299900;


// =====================================================
// GPS DATA
// =====================================================

String latitude = "Waiting...";
String longitude = "Waiting...";
String altitude = "Waiting...";
String satellites = "Waiting...";
String speed = "Waiting...";
String utcTime = "Waiting...";
String gpsDate = "Waiting...";
String fixStatus = "NO FIX";


// =====================================================
// SECURITY
// =====================================================

float distanceCM = 999.0;
bool dangerDetected = false;

const float DANGER_DISTANCE = 5.0;


// =====================================================
// SYSTEM STATE
// =====================================================

bool accessGranted = false;
bool wifiStarted = false;


// =====================================================
// NMEA FIELD READER
// =====================================================

String getField(String data, int index) {

  int start = 0;
  int end = -1;

  for (int i = 0; i <= index; i++) {

    start = end + 1;

    end = data.indexOf(
      ',',
      start
    );

    if (end == -1) {
      end = data.length();
    }
  }

  return data.substring(
    start,
    end
  );
}


// =====================================================
// GPS COORDINATE CONVERSION
// =====================================================

double convertCoordinate(
  String value,
  String direction
) {

  if (value.length() < 4) {
    return 0;
  }

  double raw =
    value.toDouble();

  int degrees =
    (int)(raw / 100);

  double minutes =
    raw -
    (degrees * 100);

  double decimal =
    degrees +
    (minutes / 60.0);

  if (
    direction == "S" ||
    direction == "W"
  ) {

    decimal =
      -decimal;

  }

  return decimal;
}


// =====================================================
// GPS PROCESSING
// =====================================================

void processGPS() {

  while (GPS.available()) {

    String line =
      GPS.readStringUntil('\n');

    line.trim();


    // -------------------------------------------------
    // GGA
    // -------------------------------------------------

    if (
      line.startsWith("$GPGGA") ||
      line.startsWith("$GNGGA")
    ) {

      String time =
        getField(line, 1);

      String lat =
        getField(line, 2);

      String latDir =
        getField(line, 3);

      String lon =
        getField(line, 4);

      String lonDir =
        getField(line, 5);

      String fix =
        getField(line, 6);

      String sats =
        getField(line, 7);

      String alt =
        getField(line, 9);


      if (
        fix.toInt() > 0
      ) {

        double latValue =
          convertCoordinate(
            lat,
            latDir
          );

        double lonValue =
          convertCoordinate(
            lon,
            lonDir
          );


        latitude =
          String(
            latValue,
            6
          );

        longitude =
          String(
            lonValue,
            6
          );

        altitude =
          alt + " m";

        satellites =
          sats;

        fixStatus =
          "FIXED";

      }

      else {

        fixStatus =
          "NO FIX";

      }


      utcTime =
        time;
    }


    // -------------------------------------------------
    // RMC
    // -------------------------------------------------

    if (
      line.startsWith("$GPRMC") ||
      line.startsWith("$GNRMC")
    ) {

      String time =
        getField(line, 1);

      String status =
        getField(line, 2);

      String lat =
        getField(line, 3);

      String latDir =
        getField(line, 4);

      String lon =
        getField(line, 5);

      String lonDir =
        getField(line, 6);

      String spd =
        getField(line, 7);

      String date =
        getField(line, 9);


      if (
        status == "A"
      ) {

        double latValue =
          convertCoordinate(
            lat,
            latDir
          );

        double lonValue =
          convertCoordinate(
            lon,
            lonDir
          );


        latitude =
          String(
            latValue,
            6
          );

        longitude =
          String(
            lonValue,
            6
          );


        double speedKmh =
          spd.toDouble() *
          1.852;


        speed =
          String(
            speedKmh,
            2
          ) +
          " km/h";


        fixStatus =
          "FIXED";

      }


      utcTime =
        time;

      gpsDate =
        date;
    }
  }
}


// =====================================================
// HC-SR04 DISTANCE
// =====================================================

float readDistance() {

  digitalWrite(
    TRIG_PIN,
    LOW
  );

  delayMicroseconds(3);


  digitalWrite(
    TRIG_PIN,
    HIGH
  );

  delayMicroseconds(10);


  digitalWrite(
    TRIG_PIN,
    LOW
  );


  long duration =
    pulseIn(
      ECHO_PIN,
      HIGH,
      30000
    );


  if (
    duration == 0
  ) {

    return 999.0;

  }


  return
    duration *
    0.0343 /
    2.0;
}


// =====================================================
// SECURITY UPDATE
// =====================================================

void updateSecurity() {

  distanceCM =
    readDistance();


  if (
    distanceCM <=
    DANGER_DISTANCE
  ) {

    dangerDetected =
      true;

  }

  else {

    dangerDetected =
      false;

  }
}


// =====================================================
// WEBSITE
// =====================================================

String webpage() {

  String html = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta
name="viewport"
content="width=device-width, initial-scale=1"
>

<title>POTTATO WORLD</title>


<style>

/* =====================================================
   BASIC
===================================================== */

* {
  box-sizing: border-box;
}

body {

  margin: 0;

  background: #0b0b0b;

  color: white;

  font-family:
  Arial,
  Helvetica,
  sans-serif;

}


/* =====================================================
   HEADER
===================================================== */

header {

  padding:
  28px 20px;

  text-align:
  center;

  background:
  linear-gradient(
    135deg,
    #141414,
    #252525
  );

  border-bottom:
  1px solid #333;

}

header h1 {

  margin:
  0;

  font-size:
  42px;

  letter-spacing:
  2px;

}

.subtitle {

  margin-top:
  8px;

  color:
  #999;

}


/* =====================================================
   MAIN CONTAINER
===================================================== */

.container {

  width:
  100%;

  max-width:
  1700px;

  margin:
  auto;

  padding:
  20px;

}


/* =====================================================
   SECURITY
===================================================== */

.security {

  padding:
  20px;

  margin-bottom:
  20px;

  background:
  #171717;

  border:
  1px solid #333;

  border-radius:
  18px;

  text-align:
  center;

}

.security h2 {

  margin-top:
  0;

}

.security-distance {

  font-size:
  30px;

  font-weight:
  bold;

}

.safe {

  color:
  #00ff88;

}

.danger {

  color:
  #ff3333;

  animation:
  dangerFlash
  0.7s
  infinite
  alternate;

}

@keyframes dangerFlash {

  from {
    opacity: 1;
  }

  to {
    opacity: 0.45;
  }

}


/* =====================================================
   MAIN LAYOUT

   GPS       | BIG GAME
   EXPERIENCE| PROFILE
===================================================== */

.dashboard {

  display:
  grid;

  grid-template-columns:
  1fr 3fr;

  gap:
  20px;

}


/* GPS */

.dashboard .card:nth-child(1) {

  grid-column:
  1;

  grid-row:
  1;

}


/* BIG GAME */

.dashboard .card:nth-child(2) {

  grid-column:
  2;

  grid-row:
  1;

}


/* EXPERIENCE */

.dashboard .card:nth-child(3) {

  grid-column:
  1;

  grid-row:
  2;

}


/* PROFILE */

.dashboard .card:nth-child(4) {

  grid-column:
  2;

  grid-row:
  2;

}


/* =====================================================
   CARDS
===================================================== */

.card {

  background:
  #171717;

  border:
  1px solid #2d2d2d;

  border-radius:
  18px;

  padding:
  22px;

  box-shadow:
  0 8px 30px
  rgba(0,0,0,0.35);

}

.card h2 {

  margin-top:
  0;

  padding-bottom:
  12px;

  border-bottom:
  1px solid #333;

}


/* =====================================================
   GPS
===================================================== */

.gps-box {

  background:
  #0d0d0d;

  border:
  1px solid #292929;

  border-radius:
  14px;

  padding:
  18px;

}

.gps-item {

  margin-bottom:
  16px;

}

.gps-label {

  color:
  #888;

  font-size:
  13px;

}

.gps-value {

  margin-top:
  3px;

  font-size:
  18px;

  font-weight:
  bold;

  word-break:
  break-word;

}


/* =====================================================
   GAME HEADER
===================================================== */

.game-subtitle {

  color:
  #aaa;

  font-size:
  15px;

  margin-bottom:
  8px;

}

.controls {

  display:
  inline-flex;

  align-items:
  center;

  gap:
  8px;

  padding:
  10px 16px;

  margin:
  8px 0 15px;

  background:
  #0d0d0d;

  border:
  1px solid #333;

  border-radius:
  10px;

  font-weight:
  bold;

}

.key {

  display:
  inline-flex;

  align-items:
  center;

  justify-content:
  center;

  width:
  32px;

  height:
  32px;

  background:
  #252525;

  border:
  1px solid #555;

  border-radius:
  7px;

  box-shadow:
  0 2px 0 #111;

}


/* =====================================================
   BIG GAME AREA
===================================================== */

#gameArea {

  width:
  100%;

  height:
  620px;

  background:
  radial-gradient(
    circle at center,
    #333,
    #202020
  );

  border:
  3px solid #555;

  border-radius:
  20px;

  position:
  relative;

  overflow:
  hidden;

  box-shadow:
  inset 0 0 50px
  rgba(0,0,0,0.45);

}


/* =====================================================
   GAME GRID
===================================================== */

#gameArea::before {

  content:
  "";

  position:
  absolute;

  inset:
  0;

  background-image:
  linear-gradient(
    rgba(255,255,255,0.035)
    1px,
    transparent 1px
  ),
  linear-gradient(
    90deg,
    rgba(255,255,255,0.035)
    1px,
    transparent 1px
  );

  background-size:
  40px 40px;

  pointer-events:
  none;

}


/* =====================================================
   POTATO
===================================================== */

#potato {

  position:
  absolute;

  width:
  90px;

  height:
  70px;

  background:
  #c58a45;

  border-radius:
  50%;

  left:
  100px;

  top:
  270px;

  z-index:
  10;

  user-select:
  none;

  box-shadow:
  inset -10px -10px 15px
  rgba(0,0,0,0.25),

  0 8px 18px
  rgba(0,0,0,0.35);

}


/* Potato eyes */

#potato::before {

  content:
  "👀";

  position:
  absolute;

  top:
  12px;

  left:
  20px;

  font-size:
  24px;

}


/* Potato mouth */

#potato::after {

  content:
  "•";

  position:
  absolute;

  bottom:
  2px;

  left:
  40px;

  font-size:
  28px;

}


/* =====================================================
   BASKET
===================================================== */

#basket {

  position:
  absolute;

  font-size:
  68px;

  left:
  70%;

  top:
  45%;

  z-index:
  8;

  user-select:
  none;

  transition:
  left 0.18s ease,
  top 0.18s ease;

  filter:
  drop-shadow(
    0 7px 8px
    rgba(0,0,0,0.4)
  );

}


/* =====================================================
   GAME INFO
===================================================== */

.game-status {

  display:
  flex;

  justify-content:
  space-between;

  flex-wrap:
  wrap;

  gap:
  10px;

  margin-top:
  14px;

}

.stat-box {

  background:
  #0d0d0d;

  border:
  1px solid #333;

  border-radius:
  10px;

  padding:
  10px 15px;

}

.stat-label {

  color:
  #888;

  font-size:
  12px;

}

.stat-value {

  font-size:
  18px;

  font-weight:
  bold;

}


/* =====================================================
   EXPERIENCE
===================================================== */

.experience-box {

  background:
  #0d0d0d;

  border-radius:
  14px;

  padding:
  18px;

  margin-bottom:
  16px;

  line-height:
  1.7;

}

.quote {

  color:
  #aaa;

}

.feeling {

  font-size:
  32px;

  margin:
  15px 0;

}


/* =====================================================
   PROFILE
===================================================== */

.profile-row {

  padding:
  12px 0;

  border-bottom:
  1px solid #292929;

}

.profile-title {

  color:
  #888;

  font-size:
  13px;

}

.profile-value {

  font-size:
  18px;

  margin-top:
  4px;

}

.section-title {

  margin-top:
  22px;

  margin-bottom:
  10px;

  color:
  #d9a15d;

  font-size:
  17px;

  font-weight:
  bold;

}


/* =====================================================
   DANGER OVERLAY
===================================================== */

#dangerOverlay {

  display:
  none;

  position:
  fixed;

  inset:
  0;

  z-index:
  1000;

  background:
  rgba(90,0,0,0.94);

  align-items:
  center;

  justify-content:
  center;

  text-align:
  center;

}

.danger-icon {

  font-size:
  80px;

}

.danger-text {

  font-size:
  48px;

  font-weight:
  bold;

}

.danger-sub {

  margin-top:
  12px;

  font-size:
  20px;

}


/* =====================================================
   TABLET
===================================================== */

@media(max-width:1100px) {

  .dashboard {

    grid-template-columns:
    1fr;

  }

  .dashboard .card:nth-child(1),
  .dashboard .card:nth-child(2),
  .dashboard .card:nth-child(3),
  .dashboard .card:nth-child(4) {

    grid-column:
    1;

    grid-row:
    auto;

  }

  #gameArea {

    height:
    600px;

  }

}


/* =====================================================
   PHONE
===================================================== */

@media(max-width:650px) {

  header h1 {

    font-size:
    30px;

  }

  .container {

    padding:
    12px;

  }

  .card {

    padding:
    16px;

  }

  #gameArea {

    height:
    430px;

  }

  #potato {

    width:
    75px;

    height:
    58px;

  }

  #basket {

    font-size:
    52px;

  }

  .danger-text {

    font-size:
    34px;

  }

}

</style>

</head>


<body>


<!-- =====================================================
     HEADER
===================================================== -->

<header>

<h1>
🥔 POTTATO WORLD
</h1>

<div class="subtitle">

The most unnecessary technology ever created.

</div>

</header>


<div class="container">


<!-- =====================================================
     SECURITY
===================================================== -->

<div class="security">

<h2>
🛡️ POTTATO SECURITY SYSTEM
</h2>

<div
id="securityStatus"
class="security-distance safe"
>

🟢 AREA SAFE

</div>

<div>

Distance:

<strong id="distance">
Waiting...
</strong>

</div>

</div>


<!-- =====================================================
     DASHBOARD
===================================================== -->

<div class="dashboard">


<!-- =====================================================
     GPS SYSTEM
===================================================== -->

<div class="card">

<h2>
📡 GPS SYSTEM
</h2>


<div class="gps-box">


<div class="gps-item">

<div class="gps-label">
LATITUDE
</div>

<div
id="lat"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
LONGITUDE
</div>

<div
id="lon"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
ALTITUDE
</div>

<div
id="alt"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
SATELLITES
</div>

<div
id="sat"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
SPEED
</div>

<div
id="speed"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
UTC TIME
</div>

<div
id="time"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
DATE
</div>

<div
id="date"
class="gps-value"
>

Waiting...

</div>

</div>


<div class="gps-item">

<div class="gps-label">
GPS FIX
</div>

<div
id="fix"
class="gps-value"
>

NO FIX

</div>

</div>


</div>


<br>


<div class="gps-box">

<div class="gps-label">

POTTATO'S PERMANENT LOCATION

</div>

<div class="gps-value">

Ernakulam,
Kerala,
India

</div>


<br>


<div class="gps-label">

POTTATO COORDINATES

</div>

<div
id="pottatoLat"
class="gps-value"
>

Loading...

</div>

<div
id="pottatoLon"
class="gps-value"
>

Loading...

</div>


<br>

<div
id="movement"
class="quote"
>

🥔 Pottato is staying still.

</div>

</div>

</div>


<!-- =====================================================
     BIG POTTATO GAME
===================================================== -->

<div class="card">

<h2>
🎮 POTTATO GAME
</h2>


<div class="game-subtitle">

🥔 Get the potato close to the basket!

</div>


<div class="controls">

<span>W</span>
<span class="key">W</span>

<span>A</span>
<span class="key">A</span>

<span>S</span>
<span class="key">S</span>

<span>D</span>
<span class="key">D</span>

</div>


<div class="game-subtitle">

The basket escapes when Pottato gets within
<strong>2 cm</strong>!

</div>


<div id="gameArea">

<div id="potato"></div>

<div id="basket">
🧺
</div>

</div>


<div class="game-status">


<div class="stat-box">

<div class="stat-label">
SCORE
</div>

<div
id="score"
class="stat-value"
>

0

</div>

</div>


<div class="stat-box">

<div class="stat-label">
DISTANCE
</div>

<div
id="gameDistance"
class="stat-value"
>

--

</div>

</div>


<div class="stat-box">

<div class="stat-label">
STATUS
</div>

<div
id="gameMessage"
class="stat-value"
>

🎮 READY!

</div>

</div>


</div>

</div>


<!-- =====================================================
     EXPERIENCE
===================================================== -->

<div class="card">

<h2>
🧪 POTTATO EXPERIENCE
</h2>


<div class="experience-box">

<h3>
🥔 Life Experience
</h3>

<p class="quote">

"I have travelled from the farm
to the kitchen."

</p>

<p class="quote">

"I have experienced being boiled,
fried, roasted and turned into curry."

</p>

<p class="quote">

"Humans keep putting masala on me."

</p>

</div>


<div class="experience-box">

<h3>
❤️ Feelings
</h3>

<div class="feeling">
😐
</div>

<p class="quote">

Feeling detection result:

<strong>
UNKNOWN
</strong>

</p>

<p class="quote">

Reason:

Pottato is not a living thing.

</p>

</div>


<div class="experience-box">

<h3>
📍 Experiences
</h3>

<p>
✔ Pottato tracking
</p>

<p>
✔ GPS monitoring
</p>

<p>
✔ Security monitoring
</p>

<p>
✔ Human interaction
</p>

</div>

</div>


<!-- =====================================================
     PROFILE
===================================================== -->

<div class="card">

<h2>
🥔 POTTATO PROFILE
</h2>


<div class="profile-row">

<div class="profile-title">
NAME
</div>

<div class="profile-value">
Pottato
</div>

</div>


<div class="profile-row">

<div class="profile-title">
SPECIES
</div>

<div class="profile-value">
Solanum tuberosum
</div>

</div>


<div class="profile-row">

<div class="profile-title">
PROFESSION
</div>

<div class="profile-value">
Tasty Food
</div>

</div>


<div class="section-title">
🎯 MISSION
</div>

<p>
Become the world's tastiest potato.
</p>

<p>
Survive every kitchen.
</p>

<p>
Absorb maximum masala.
</p>


<div class="section-title">
🎓 EDUCATION
</div>

<p>
Kerala Kitchen Academy
</p>

<p>
Master of Curry Compatibility
</p>

<p>
Advanced Masala Absorption
</p>


<div class="section-title">
⚡ ABILITIES
</div>

<p>
🥔 Can be boiled
</p>

<p>
🔥 Can be roasted
</p>

<p>
🍟 Can become fries
</p>

<p>
🍛 Can become curry
</p>

<p>
🌶️ Absorbs masala
</p>

<p>
🥘 Works with sambar
</p>


<div class="section-title">
👨‍👩‍👧 FAMILY
</div>

<p>
Father Pottato
</p>

<p>
Mother Pottato
</p>

<p>
Uncle Pottato
</p>

<p>
Pottato
</p>

</div>

</div>

</div>


<!-- =====================================================
     DANGER OVERLAY
===================================================== -->

<div
id="dangerOverlay"
>

<div>

<div class="danger-icon">
🚨
</div>

<div class="danger-text">
STAY AWAY!
</div>

<div class="danger-sub">
🥔 POTTATO SECURITY ALERT
</div>

<div class="danger-sub">
Object detected within 5 cm.
</div>

</div>

</div>


<script>


// =====================================================
// POTTATO LOCATION
// =====================================================

const BASE_LAT =
9.9816;

const BASE_LON =
76.2999;


document.getElementById(
  "pottatoLat"
).innerText =
  BASE_LAT.toFixed(6);


document.getElementById(
  "pottatoLon"
).innerText =
  BASE_LON.toFixed(6);


// =====================================================
// FAKE POTTATO LOCATION
// =====================================================

function updatePottatoLocation() {

  const escape =
    Math.random() > 0.5;


  if (escape) {

    document.getElementById(
      "pottatoLat"
    ).innerText =
      (
        BASE_LAT +
        0.0002
      ).toFixed(6);


    document.getElementById(
      "pottatoLon"
    ).innerText =
      (
        BASE_LON +
        0.0002
      ).toFixed(6);


    document.getElementById(
      "movement"
    ).innerText =
      "🥔 Pottato escaped!";

  }

  else {

    document.getElementById(
      "pottatoLat"
    ).innerText =
      BASE_LAT.toFixed(6);


    document.getElementById(
      "pottatoLon"
    ).innerText =
      BASE_LON.toFixed(6);


    document.getElementById(
      "movement"
    ).innerText =
      "🥔 Pottato returned home.";

  }

}


setInterval(
  updatePottatoLocation,
  5000
);


// =====================================================
// REAL GPS
// =====================================================

function updateGPS() {

  fetch("/gps")

  .then(
    response =>
      response.json()
  )

  .then(
    data => {

      document.getElementById(
        "lat"
      ).innerText =
        data.latitude;


      document.getElementById(
        "lon"
      ).innerText =
        data.longitude;


      document.getElementById(
        "alt"
      ).innerText =
        data.altitude;


      document.getElementById(
        "sat"
      ).innerText =
        data.satellites;


      document.getElementById(
        "speed"
      ).innerText =
        data.speed;


      document.getElementById(
        "time"
      ).innerText =
        data.time;


      document.getElementById(
        "date"
      ).innerText =
        data.date;


      document.getElementById(
        "fix"
      ).innerText =
        data.fix;

    }
  )

  .catch(
    error =>
      console.log(
        "GPS error:",
        error
      )
  );

}


setInterval(
  updateGPS,
  2000
);

updateGPS();


// =====================================================
// HC-SR04 SECURITY
// =====================================================

function updateSecurity() {

  fetch("/security")

  .then(
    response =>
      response.json()
  )

  .then(
    data => {

      document.getElementById(
        "distance"
      ).innerText =
        data.distance.toFixed(1)
        + " cm";


      const status =
        document.getElementById(
          "securityStatus"
        );


      const overlay =
        document.getElementById(
          "dangerOverlay"
        );


      if (
        data.danger
      ) {

        status.innerText =
          "🚨 STAY AWAY!";

        status.className =
          "security-distance danger";


        overlay.style.display =
          "flex";

      }

      else {

        status.innerText =
          "🟢 AREA SAFE";

        status.className =
          "security-distance safe";


        overlay.style.display =
          "none";

      }

    }
  )

  .catch(
    error =>
      console.log(
        "Security error:",
        error
      )
  );

}


setInterval(
  updateSecurity,
  500
);

updateSecurity();


// =====================================================
// POTTATO GAME
// =====================================================

const potato =
  document.getElementById(
    "potato"
  );

const basket =
  document.getElementById(
    "basket"
  );

const gameArea =
  document.getElementById(
    "gameArea"
  );

const scoreDisplay =
  document.getElementById(
    "score"
  );

const distanceDisplay =
  document.getElementById(
    "gameDistance"
  );

const messageDisplay =
  document.getElementById(
    "gameMessage"
  );


// =====================================================
// GAME SETTINGS
// =====================================================

// Potato movement speed
const SPEED = 7;


// Potato dimensions
const POTATO_WIDTH = 90;
const POTATO_HEIGHT = 70;


// Basket approximate size
const BASKET_SIZE = 70;


// 2 centimetres converted to CSS pixels
// 96 CSS pixels = 1 inch
// 1 inch = 2.54 cm
//
// 2 cm = approximately 75.6 px

const TRIGGER_DISTANCE =
  2 *
  96 /
  2.54;


// =====================================================
// GAME STATE
// =====================================================

let potatoX = 100;
let potatoY = 270;

let basketX = 0;
let basketY = 0;

let score = 0;

let gameRunning = true;


// =====================================================
// INITIAL BASKET POSITION
// =====================================================

function positionBasketInitially() {

  const width =
    gameArea.clientWidth;

  const height =
    gameArea.clientHeight;


  basketX =
    width -
    150;

  basketY =
    height / 2 -
    35;


  basket.style.left =
    basketX + "px";

  basket.style.top =
    basketY + "px";

}


positionBasketInitially();


// =====================================================
// UPDATE POTATO
// =====================================================

function renderPotato() {

  potato.style.left =
    potatoX + "px";

  potato.style.top =
    potatoY + "px";

}


// =====================================================
// KEYBOARD CONTROLS
// =====================================================

const keys = {};


// Listen for keyboard
document.addEventListener(
  "keydown",
  function(event) {

    const key =
      event.key.toLowerCase();


    if (
      [
        "w",
        "a",
        "s",
        "d"
      ].includes(key)
    ) {

      keys[key] =
        true;

      event.preventDefault();

    }

  }
);


document.addEventListener(
  "keyup",
  function(event) {

    const key =
      event.key.toLowerCase();


    if (
      [
        "w",
        "a",
        "s",
        "d"
      ].includes(key)
    ) {

      keys[key] =
        false;

      event.preventDefault();

    }

  }
);


// =====================================================
// GAME MOVEMENT LOOP
// =====================================================

function gameLoop() {

  if (!gameRunning) {
    return;
  }


  const width =
    gameArea.clientWidth;

  const height =
    gameArea.clientHeight;


  // -----------------------------
  // W = UP
  // -----------------------------

  if (keys["w"]) {

    potatoY -= SPEED;

  }


  // -----------------------------
  // S = DOWN
  // -----------------------------

  if (keys["s"]) {

    potatoY += SPEED;

  }


  // -----------------------------
  // A = LEFT
  // -----------------------------

  if (keys["a"]) {

    potatoX -= SPEED;

  }


  // -----------------------------
  // D = RIGHT
  // -----------------------------

  if (keys["d"]) {

    potatoX += SPEED;

  }


  // Keep potato inside game

  potatoX =
    Math.max(
      0,
      Math.min(
        potatoX,
        width -
        POTATO_WIDTH
      )
    );


  potatoY =
    Math.max(
      0,
      Math.min(
        potatoY,
        height -
        POTATO_HEIGHT
      )
    );


  renderPotato();


  checkBasketDistance();


  requestAnimationFrame(
    gameLoop
  );

}


requestAnimationFrame(
  gameLoop
);


// =====================================================
// CALCULATE DISTANCE
// =====================================================

function checkBasketDistance() {

  const potatoCenterX =
    potatoX +
    POTATO_WIDTH / 2;


  const potatoCenterY =
    potatoY +
    POTATO_HEIGHT / 2;


  const basketCenterX =
    basketX +
    BASKET_SIZE / 2;


  const basketCenterY =
    basketY +
    BASKET_SIZE / 2;


  const dx =
    potatoCenterX -
    basketCenterX;


  const dy =
    potatoCenterY -
    basketCenterY;


  const distancePixels =
    Math.sqrt(
      dx * dx +
      dy * dy
    );


  // Convert pixels to cm

  const distanceCM =
    distancePixels *
    2.54 /
    96;


  distanceDisplay.innerText =
    distanceCM.toFixed(1)
    + " cm";


  // =================================================
  // BASKET ESCAPE
  // =================================================

  if (
    distancePixels <=
    TRIGGER_DISTANCE
  ) {

    basketEscape();

  }

}


// =====================================================
// BASKET ESCAPE
// =====================================================

function basketEscape() {

  const width =
    gameArea.clientWidth;

  const height =
    gameArea.clientHeight;


  const oldX =
    basketX;

  const oldY =
    basketY;


  let newX;
  let newY;

  let attempts =
    0;


  // Try to find a position far away
  // from the potato

  do {

    newX =
      Math.random() *
      (
        width -
        BASKET_SIZE
      );


    newY =
      Math.random() *
      (
        height -
        BASKET_SIZE
      );


    const dx =
      newX -
      potatoX;


    const dy =
      newY -
      potatoY;


    const distance =
      Math.sqrt(
        dx * dx +
        dy * dy
      );


    attempts++;


    if (
      distance > 250
    ) {

      break;

    }

  }

  while (
    attempts < 50
  );


  // Make sure it actually changed

  if (
    Math.abs(
      newX - oldX
    ) < 100
    &&
    Math.abs(
      newY - oldY
    ) < 100
  ) {

    newX =
      width -
      120;

    newY =
      Math.random() *
      (
        height -
        BASKET_SIZE
      );

  }


  basketX =
    newX;

  basketY =
    newY;


  basket.style.left =
    basketX + "px";


  basket.style.top =
    basketY + "px";


  // Increase score

  score++;

  scoreDisplay.innerText =
    score;


  messageDisplay.innerText =
    "🏃 Basket escaped!";


  // Temporary message

  setTimeout(
    function() {

      messageDisplay.innerText =
        "🥔 Catch it again!";

    },
    800
  );

}


// =====================================================
// WINDOW RESIZE
// =====================================================

window.addEventListener(
  "resize",
  function() {

    const width =
      gameArea.clientWidth;

    const height =
      gameArea.clientHeight;


    potatoX =
      Math.min(
        potatoX,
        width -
        POTATO_WIDTH
      );


    potatoY =
      Math.min(
        potatoY,
        height -
        POTATO_HEIGHT
      );


    basketX =
      Math.min(
        basketX,
        width -
        BASKET_SIZE
      );


    basketY =
      Math.min(
        basketY,
        height -
        BASKET_SIZE
      );


    renderPotato();


    basket.style.left =
      basketX + "px";


    basket.style.top =
      basketY + "px";

  }
);


</script>

</body>

</html>

)rawliteral";


  return html;
}


// =====================================================
// GPS API
// =====================================================

void handleGPS() {

  String json =
    "{";


  json +=
    "\"latitude\":\"" +
    latitude +
    "\",";


  json +=
    "\"longitude\":\"" +
    longitude +
    "\",";


  json +=
    "\"altitude\":\"" +
    altitude +
    "\",";


  json +=
    "\"satellites\":\"" +
    satellites +
    "\",";


  json +=
    "\"speed\":\"" +
    speed +
    "\",";


  json +=
    "\"time\":\"" +
    utcTime +
    "\",";


  json +=
    "\"date\":\"" +
    gpsDate +
    "\",";


  json +=
    "\"fix\":\"" +
    fixStatus +
    "\"";


  json +=
    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
// SECURITY API
// =====================================================

void handleSecurity() {

  String json =
    "{";


  json +=
    "\"distance\":" +
    String(
      distanceCM,
      1
    ) +
    ",";


  json +=
    "\"danger\":" +
    String(
      dangerDetected
      ? "true"
      : "false"
    );


  json +=
    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
// START WEBSITE
// =====================================================

void startWebsite() {

  if (
    wifiStarted
  ) {

    return;

  }


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "ACCESS GRANTED"
  );

  Serial.println(
    "Starting POTTATO WORLD..."
  );

  Serial.println(
    "================================"
  );


  WiFi.softAP(
    ssid,
    password
  );


  Serial.println(
    "WiFi AP started!"
  );


  Serial.print(
    "IP address: "
  );


  Serial.println(
    WiFi.softAPIP()
  );


  server.on(
    "/",
    []() {

      server.send(
        200,
        "text/html",
        webpage()
      );

    }
  );


  server.on(
    "/gps",
    handleGPS
  );


  server.on(
    "/security",
    handleSecurity
  );


  server.begin();


  wifiStarted =
    true;


  Serial.println(
    "Website server started."
  );

}


// =====================================================
// BLUETOOTH AUTHENTICATION
// =====================================================

void checkBluetooth() {

  Serial.println();

  Serial.println(
    "Scanning for Bluetooth devices..."
  );


  BTScanResults *results =
    SerialBT.discover(
      5000
    );


  if (
    results == nullptr
  ) {

    Serial.println(
      "No devices found."
    );

    return;

  }


  int count =
    results->getCount();


  Serial.print(
    "Devices found: "
  );


  Serial.println(
    count
  );


  bool devanFound =
    false;


  for (
    int i = 0;
    i < count;
    i++
  ) {

    BTAdvertisedDevice *device =
      results->getDevice(i);


    String name =
      device->
      getName().
      c_str();


    Serial.print(
      "Device: "
    );


    Serial.println(
      name
    );


    if (
      name == "Devan"
    ) {

      devanFound =
        true;

    }

  }


  Serial.println(
    "----------------------------"
  );


  if (
    devanFound
  ) {

    Serial.println(
      "ACCESS GRANTED"
    );


    accessGranted =
      true;


    SerialBT.end();


    delay(
      1000
    );


    startWebsite();

  }

  else {

    Serial.println(
      "ACCESS DENIED"
    );

  }

}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  delay(
    1000
  );


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "       POTTATO WORLD"
  );

  Serial.println(
    "================================"
  );


  // -------------------------------------------------
  // GPS
  // -------------------------------------------------

  GPS.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );


  Serial.println(
    "GPS started."
  );


  // -------------------------------------------------
  // HC-SR04
  // -------------------------------------------------

  pinMode(
    TRIG_PIN,
    OUTPUT
  );


  pinMode(
    ECHO_PIN,
    INPUT
  );


  digitalWrite(
    TRIG_PIN,
    LOW
  );


  Serial.println(
    "HC-SR04 started."
  );


  // -------------------------------------------------
  // BLUETOOTH
  // -------------------------------------------------

  SerialBT.begin(
    "ESP32_AUTH"
  );


  Serial.println(
    "Bluetooth name: ESP32_AUTH"
  );


  Serial.println(
    "Waiting for Devan..."
  );

}


// =====================================================
// LOOP
// =====================================================

unsigned long
lastBluetoothScan = 0;

unsigned long
lastSecurityRead = 0;


void loop() {

  // -------------------------------------------------
  // GPS
  // -------------------------------------------------

  processGPS();


  // -------------------------------------------------
  // HC-SR04
  // -------------------------------------------------

  if (
    millis() -
    lastSecurityRead >=
    300
  ) {

    lastSecurityRead =
      millis();


    updateSecurity();

  }


  // -------------------------------------------------
  // BLUETOOTH
  // -------------------------------------------------

  if (
    !accessGranted
  ) {

    if (
      millis() -
      lastBluetoothScan >=
      10000
    ) {

      lastBluetoothScan =
        millis();


      checkBluetooth();

    }

  }


  // -------------------------------------------------
  // WEB SERVER
  // -------------------------------------------------

  if (
    wifiStarted
  ) {

    server.handleClient();

  }

}