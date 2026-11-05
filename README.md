# Meteo-Station-with-ESP32-S3-Agent-DevKit
Got a useless devkit based on ESP32S3 from here:  https://dwmzone.com/en/wireless-tag/1329-esp32-s3-agent-dev-kit-ai-powered-iot-development-kit.html
and finally got it to do something usefull!!!

# BaroClock & Wireless Sensor Ecosystem

**Author & Developer:** Adrian Florescu (YO3HJV)  
**System Architecture:** ESP32-S3 Master Station (`BaroClock`) & Wireless Remote Sensor (`Baro_Clock_Sensor`)  

---

## 1. System Overview & Purpose

The **BaroClock Ecosystem** is an integrated precision instrument designed for radio amateurs, mountain/outdoor enthusiasts, and home weather station observers. It combines atomic-accuracy timekeeping, satellite navigation diagnostics, local atmospheric pressure tracking, physical weather forecasting, and wireless remote temperature telemetry into a cohesive, standalone system that **does not rely on any cloud services, internet forecasts, or third-party weather APIs**.

The system consists of two complementary standalone projects:
1. **`BaroClock` (Master Station):** An intelligent, touch-enabled desktop unit built on the ESP32-S3 platform with a 4.0-inch color LCD, GPS/GNSS receiver, Bosch barometric sensor, I2S digital audio codec, and built-in web server.
2. **`Baro_Clock_Sensor` (Remote Wireless Sensor):** A compact, low-cost external wireless transmitter (built on an ESP8266 / Sonoff Basic module with a Dallas DS18B20 temperature probe) that broadcasts real-time outdoor temperature to the master station via local Wi-Fi UDP packets.

```
       +---------------------------------------------+
       |           LOCAL WI-FI NETWORK               |
       +---------------------------------------------+
               ^                             |
   UDP Broadcast (Port 5005)                 | Web Browser
        Protocol: METEO_V1                   | HTTP (Port 80)
               |                             v
    +-----------------------+     +-----------------------+
    |   Baro_Clock_Sensor   |     |   BaroClock Master    |
    |  (ESP8266 + DS18B20)  |     | (ESP32-S3 + 4.0" LCD) |
    |   Outdoor / Balcony   |     |   Desktop Station     |
    +-----------------------+     +-----------------------+
                                             |
                                  +----------+----------+
                                  |                     |
                                  v                     v
                            GNSS GP-02              Bosch Sensor
                           (GPS/BeiDou)          (BMP280 / BME280)
```

---

## 2. Project 1: BaroClock Master Station

### 2.1 Hardware Platform & Components

The master station is based on the **ESP32-S3 DevKit** architecture running at 240 MHz with dual Xtensa LX7 cores, 16 MB of Flash memory, and Octal SPI PSRAM for fluid graphics:

* **Microcontroller (MCU):** ESP32-S3-WROOM-1 (Dual-Core 240 MHz, 16 MB Flash, 8 MB OPI PSRAM).
* **Color Display:** 4.0-inch TFT LCD (480 x 320 pixels) driven by the ST7796 controller over a high-speed 8-bit parallel 8080 bus.
* **Touchscreen Controller:** FocalTech FT6336 capacitive touch controller (I2C) with gesture support (tap, horizontal swipe).
* **I/O Port Expander:** AW9523 (I2C) managing hardware resets, audio power amplifier enable, and backlight control.
* **Barometric & Temperature Sensor:** Bosch Sensortec BMP280 / BME280 connected to the internal I2C bus (chip auto-detected at startup).
* **Satellite Navigation (GNSS):** Ai-Thinker GP-02 (AT6558 chip) high-sensitivity receiver connected via UART (GPIO 20 Rx) with automatic baud-rate detection (9600 to 115200 bps). Supports GPS, BeiDou, and combined constellations.
* **Audio Engine:** Realtek/Everest ES8311 dedicated I2S digital-to-analog audio codec paired with an onboard Class-D power amplifier and built-in loudspeaker.
* **Proximity Sensor:** MK-TP2016 optical infrared proximity sensor (I2C) for touchless hand-wave detection.

---

### 2.2 Key Features & Functionalities

#### Dual Atomic Clock & Slew Rate Synchronization
* **Local Time & UTC Display:** Features large digital clock digits alongside a dedicated UTC clock display essential for amateur radio logging and aviation operations.
* **Atomic Precision:** Time is primarily synchronized from the atomic clocks on board GNSS satellites. If satellite signals are temporarily unavailable indoors, the system automatically falls back to internet NTP time servers over Wi-Fi, or uses its internal high-stability RTC.
* **Smooth Slew Rate Correction:** Rather than abruptly jumping the clock forwards or backwards when a drift is detected, BaroClock performs a gentle, linear **3-minute Slew adjustment**. Time speeds up or slows down imperceptibly until synchronism is achieved, preventing timeline distortions in ongoing barograph logs.

#### Amateur Radio Maidenhead Grid Locator
* Automatically calculates and displays the user's **6-character QTH Maidenhead Locator** (e.g., `KN34bk`) in real-time based on high-precision GPS coordinates.
* Can also be manually configured in the web settings for indoor environments without GPS coverage.

#### High-Performance LVGL 9 Graphics & Color Themes
The entire visual interface is rendered through the Light and Versatile Graphics Library (LVGL v9.2) with a vector-like, anti-aliased appearance. It includes **6 selectable color profiles**:
1. **Monochrome (High-Contrast):** Pure white digits and light gray labels on an inky black background.
2. **Amber CRT:** Warm vintage amber luminescence reminiscent of classic radar and terminal monitors.
3. **Green Matrix:** Phosphor green styling with uniform brightness across all variables and labels.
4. **Cyan Electric:** Crisp, cyberpunk-inspired high-contrast cyan with uniform variable and label intensity.
5. **Night Red:** Low-light red illumination designed to preserve astronomical dark-adapted night vision.
6. **Original EEZ:** Traditional reflective digital LCD style with solid cyan panels and deep charcoal digits.

#### Acoustic Alarms & Hourly Chime
* **Classic Watch Chime:** Generates an authentic dual-tone hourly pip-pip signal (`Pip-Pip`) at the top of every hour.
* **Wake-Up Alarm Clock:** Multi-burst wake-up alarm with configurable hour, minute, and periodicity (Daily, Monday-to-Friday, or Once).
* **Weather Warning Audio:** Tri-beep alert for sudden weather deterioration and continuous Morse code S.O.S siren for severe storm drops.
* **Anti-Pop Audio Stabilization:** Features a hardware bias settling routine that pre-stabilizes the power amplifier in silent digital mode for 100 ms before tone generation, ensuring that every beep sounds crisp, identical, and without pops.

#### Responsive Multi-Page Web Control Panel
An intuitive, dark-themed responsive dashboard accessible from any phone, tablet, or computer on the local network (`http://<ip-address>/`), divided into 7 organized subpages:
* **Dashboard (`/`):** Instant overview of system status, active alarm, GPS fix, clock engine, and live sensor values.
* **Touch & Feel (`/touch_feel`):** Color theme selection, 180° screen inversion, and visual symbol preview tests.
* **Sound & Alarms (`/sound_alarms`):** Comprehensive configuration of chime, alarm clock, volumes, and proximity detection.
* **Weather (`/weather`):** Forecast indices, barometric history statistics, and manual ground-truth anchoring.
* **Networks (`/networks`):** Wi-Fi credentials management (3 prioritized network slots) and external wireless sensor pairing.
* **Temp Calib (`/temp_calib`):** The interactive 11-band calibration table with live delta calculation.
* **Settings (`/settings`):** Timezone, Daylight Saving Time (DST), measurement units (°C/°F, hPa/mb/mmHg), barograph sensitivity, and GNSS satellite constellation settings.

---

## 3. The Physical Weather Forecasting Engine

### 3.1 Principles of Barometric Forecasting (For Beginners)
Atmospheric pressure is literally the **weight of the column of air** pressing down upon the Earth's surface. Air masses moving across continents create natural pressure waves:
* **High-Pressure Air (Anticyclone):** Cold, dense air gently sinks toward the ground. Sinking air warms up slightly, preventing clouds from forming and resulting in clear, calm, and dry weather.
* **Low-Pressure Air (Cyclone/Depression):** Warm, buoyant air rises into the upper atmosphere. As it rises, it cools rapidly, causing moisture to condense into thick clouds, rain, and winds.
* **Violent Pressure Plunges:** When a cold atmospheric front crashes into warm air, the barometer drops sharply, serving as nature's early alarm bell for approaching storms, squalls, and gales.

BaroClock does not simply look at whether the barometer says "1013 hPa". It monitors the **rate and speed of change** over time to predict what is coming hours before it arrives.

### 3.2 The Three Mathematical Indicators
Every 3 minutes, BaroClock samples the pressure and analyzes three key time derivatives:

1. **Short-Term Delta ($\Delta P_{30\text{min}}$):**
   $$\Delta P_{30} = P(t) - P(t - 30\text{ min})$$
   Measures immediate atmospheric instability occurring right now.

2. **Rate of Change / Baric Velocity ($V_p$ in hPa/hour):**
   $$V_p = \frac{P(t) - P(t - 3\text{ hours})}{3}$$
   Standardized hourly rate of change over the internationally recognized 3-hour synoptic window used by professional meteorologists.

3. **Pressure Acceleration ($A_p$):**
   $$A_p = \frac{V_p(t) - V_p(t - \Delta t)}{\Delta t}$$
   The second derivative of pressure. Identifies whether a drop in pressure is stabilizing or dangerously accelerating into an imminent squall.

### 3.3 Visual & Acoustic Prediction Indicators
The forecast is synthesized on the main display through three complementary elements:

| Visual Element | What It Represents | Behavior / Indicators |
| :--- | :--- | :--- |
| **Trend Arrow** | Direction of immediate pressure change | ↗️ **Green Up:** Pressure rising ($\ge +0.25\text{ hPa/h}$)<br>➡️ **Yellow Flat:** Pressure steady (between $-0.25$ and $+0.25\text{ hPa/h}$)<br>↘️ **Red Down:** Pressure falling ($\le -0.25\text{ hPa/h}$) |
| **Forecast Icon** | Overall anticipated outdoor condition | ☀️ **Sun:** Stable high pressure, clear skies<br>🌤️ **Sun with Cloud:** Steady conditions, partly cloudy<br>☁️ **Cloud:** Overcast, pressure sagging<br>🌧️ **Rain:** Sustained pressure decrease, precipitation likely<br>⛈️ **Storm:** Sharp pressure plunge, thunderstorm/gale imminent<br>📍 **Radar Target:** New location, accumulating baseline data |
| **Status Badge** | Atmospheric risk level | **STABLE (Green):** Normal conditions<br>**CAUTION (Yellow):** Moderate drop ($\Delta P_{3h} < -1.0\text{ hPa}$)<br>**DETERIORATING (Orange):** Fast drop ($\Delta P_{3h} < -2.0\text{ hPa}$)<br>**STORM ALERT (Red):** Severe drop ($\Delta P_{3h} < -3.5\text{ hPa}$) with siren alarm<br>**NEW LOCATION (Cyan):** Unit moved to a new geographic grid |

---

### 3.4 Weather Anchor Points & Ground-Truth Calibration (Deep Dive)

A common challenge with pure barometric forecasting is the **"Steady Gray Sky" dilemma**: during autumn or winter, high-pressure fog or low overcast clouds can settle over a region for days while the barometer stays completely flat and motionless. A simple algorithm would see zero pressure change and mistakenly show a bright sunny icon!

To solve this, BaroClock introduces **Ground-Truth Anchor Points**:

```
[ Web Dashboard: Weather Tab ]
  ☀️ Clear      🌤️ Partly Cld   ☁️ Overcast    🌫️ Fog/Mist
  🌦️ Drizzle    🌧️ Rain         ⛈️ Storm        🌨️ Snow
```

1. **How Many Anchor States Are Collected?**
   There are **8 discrete Ground-Truth Weather Anchor States** available in the web portal:
   * `State 0: Clear (☀️)`
   * `State 1: Partly Cloudy (🌤️)`
   * `State 2: Overcast (☁️)`
   * `State 3: Fog / Mist (🌫️)`
   * `State 4: Drizzle (🌦️)`
   * `State 5: Rain (🌧️)`
   * `State 6: Storm (⛈️)`
   * `State 7: Snow (🌨️)`

2. **How Ground-Truth Tagging Works in Memory:**
   When the user observes the real weather outside and clicks one of these 8 buttons:
   * BaroClock forces an immediate reading and sets **Bit 2 (`0x04`)** in the binary record's `flags` byte:
     `rec.flags |= 0x04; // Tagged as ground-truth observation`
   * The exact observed weather state (0 to 7) is saved directly into the record's reserved byte.
   * This permanently binds the human observation to the exact pressure, temperature, and baric slope of that moment.

3. **Location Baseline Anchoring (Minimum 4 Local Samples):**
   * If you transport BaroClock in your car or hike up a hill, the elevation change causes a natural drop in pressure that has nothing to do with weather fronts.
   * To prevent false storm alarms, BaroClock tags each record with your **GPS Maidenhead Locator Grid**.
   * If the grid changes, the system enters `[NEW LOCATION]` mode and waits until at least **4 local samples (12 minutes)** are collected before enabling the forecasting engine.

---

## 4. Multi-Point Temperature Calibration (Deep Dive)

### 4.1 The "Why" for Beginners: Why 1-Point Offsets Fail
Most consumer gadgets only have a single "offset" setting (e.g., `-1.5°C`). While this works fine in a room at 21°C, it completely breaks down across seasonal extremes:
* **The "Hot Box" Effect:** When the microcontroller operates inside an enclosure, it generates a small amount of internal heat. At +15°C ambient, the internal temperature rise might be $+2.0°C$, but at +35°C on a hot sunny day, the thermal dissipation characteristics change.
* **Sensor Non-Linearity:** Real-world semiconductor temperature sensors (silicon bandgap or thermistors) exhibit slight non-linear curves over wide spans.

A single flat offset causes the thermometer to read accurately at room temperature, but show errors of $1.5°C$ or more in freezing winter or scorching summer.

---

### 4.2 The 11-Band Piecewise Linear Calibration Engine
BaroClock solves this with a **Piecewise Continuous Linear Calibration System** across 11 temperature bands spanning **-20°C to +45°C**:

```
Band 0: [-20 to -10 °C]  -->  Node: -15.0 °C
Band 1: [-10 to  -5 °C]  -->  Node:  -7.5 °C
Band 2: [ -5 to  +5 °C]  -->  Node:   0.0 °C  (Freezing Point)
Band 3: [  5 to  10 °C]  -->  Node:   7.5 °C
Band 4: [ 10 to  15 °C]  -->  Node:  12.5 °C
Band 5: [ 15 to  20 °C]  -->  Node:  17.5 °C
Band 6: [ 20 to  25 °C]  -->  Node:  22.5 °C  (Room Comfort)
Band 7: [ 25 to  30 °C]  -->  Node:  27.5 °C
Band 8: [ 30 to  35 °C]  -->  Node:  32.5 °C
Band 9: [ 35 to  40 °C]  -->  Node:  37.5 °C
Band 10:[ 40 to  45 °C]  -->  Node:  42.5 °C
```

### 4.3 Continuous Linear Interpolation (No Artificial Jumps!)
Basic systems that use simple lookup tables suffer from "staircase jumps"—as soon as the temperature crosses from 19.9°C to 20.0°C, the reading might suddenly jump by half a degree. 

BaroClock uses **continuous linear interpolation** between the two nearest anchor nodes:
$$\text{Offset}(T) = \text{Offset}_i + \left( \frac{T - \text{Node}_i}{\text{Node}_{i+1} - \text{Node}_i} \right) \cdot (\text{Offset}_{i+1} - \text{Offset}_i)$$

* **The Result:** The correction curve is a smooth, continuous line. The temperature display moves with microscopic fluid precision without ever jumping.
* **Independent Dual Channels:** Completely separate 11-point calibration tables are maintained in non-volatile memory (NVS) for:
  1. The **Internal BME280/BMP280** sensor.
  2. The **External Wireless DS18B20** sensor.
* **Quick-Calibrate Tool:** In the web portal, users can simply type the reading of a trusted calibrated reference thermometer at the current ambient temperature. The system automatically detects which band is active, calculates the required delta, and updates the table with one click!

---

## 5. Historical Data Storage & The Rolling Barograph (Deep Dive)

### 5.1 Dual-Layer Storage Architecture: How Long Are Data Kept?
BaroClock employs an advanced **Dual-Layer Storage Architecture** to balance ultra-high resolution with long-term climate tracking:

```
[ LAYER 1: Ultra-High Resolution Ring Buffer (PSRAM) ]
  • Cadence: Every 3 minutes
  • Capacity: 6,720 records
  • Retention Period: Exactly 14 FULL DAYS (2 WEEKS)
  • Resolution: Pressure to 0.01 hPa, Temp to 0.01 °C, Altitude, GPS Grid

[ LAYER 2: Long-Term Climatic Archive (LittleFS Flash) ]
  • Cadence: Consolidated hourly average, min, and max
  • Capacity: 8,760 records
  • Retention Period: Exactly 1 FULL YEAR (365 DAYS / 8,760 HOURS)
  • File Size: Only ~175 KB of compact binary data!
```

1. **Layer 1 (The 14-Day PSRAM Buffer):**
   * Stores **6,720 records** at a high-speed **3-minute cadence**.
   * Gives you a crystal-clear, granular picture of every thunderstorm, cold front passage, and daily atmospheric tide over the past two weeks.
   * Powered by the ESP32-S3 high-speed Octal PSRAM (134.4 KB).

2. **Layer 2 (The 1-Year Flash Climatic Archive):**
   * At the top of every hour, Layer 1 data are consolidated into a specialized hourly archive record containing the **hourly average pressure, absolute minimum, absolute maximum, average temperature, and location tag**.
   * Stores **8,760 records**, representing **365 consecutive days of weather history** directly on the ESP32!
   * You can compare today's barometric patterns against the same month last year.

---

### 5.2 Flash Memory Wear Prevention (The 20-Sample RAM Staging Buffer)
Flash memory chips (like those inside smartphones and microcontrollers) have a physical limit of around 100,000 write cycles per sector before wearing out. If an embedded system wrote to Flash every 3 minutes, the memory could degrade in just a couple of years.

BaroClock implements **RAM Staging Buffer Protection**:
* Measurements are written into an in-memory staging buffer in RAM.
* Data are committed to Flash storage only after **20 samples have accumulated (every 60 minutes)**, or during a graceful reboot.
* This cuts down Flash erase/write cycles by **95%**, extending the physical operational lifespan of the ESP32-S3 Flash memory to **more than 20 years of continuous 24/7 service!**

---

### 5.3 The Rolling Barograph Strip Chart
The barograph on the LCD screen mimics an authentic mechanical drum barograph:
* **Interactive Touch Zooming:** By simply swiping left or right on the chart, or selecting from the web portal, users can instantly switch between **10 time windows**:
  `10m` • `30m` • `1h` • `3h` • `6h` • `12h` • `24h (1 Day)` • `3 Days` • `7 Days (1 Week)` • `14 Days (2 Weeks)`.
* **High vs. Low Sensitivity:**
  * **HIGH Sensitivity:** Magnifies subtle changes (0.1 hPa per vertical pixel) to reveal micro-baric waves, building ventilation pulses, or elevator pressure transients.
  * **LOW Sensitivity:** Displays full synoptic swings (1.5 hPa per pixel) to keep deep Atlantic depressions or Siberian high-pressure systems neatly on screen.
* **Auto-Adaptive Timeline Scale:** The tick marks and labels below the graph automatically change from minute markers (`-10m`, `-30m`), to hour markers (`-3h`, `-12h`), to day markers (`-7d`, `-14d`) as you zoom.

---

## 6. Project 2: Baro_Clock_Sensor (Wireless Remote Transmitter)

### 6.1 Purpose & Operating Concept
While the BaroClock master station monitors indoor climate and atmospheric pressure, users frequently need accurate outdoor temperature (or readings from a balcony, greenhouse, or basement). 

`Baro_Clock_Sensor` is a dedicated, self-contained wireless satellite that measures temperature and broadcasts it over the local Wi-Fi network. The master station discovers it automatically without requiring manual IP address pairing.

```
       +---------------------------------------------+
       |           SONOFF BASIC V1.0 MODULE          |
       |                                             |
       |   +-----------+          +---------------+  |
       |   |  ESP8266  | <------> | DS18B20 Probe |  |
       |   |   Wi-Fi   | (1-Wire) | (GPIO 2 / 3V3)|  |
       |   +-----------+          +---------------+  |
       |         |                                   |
       |   [Pushbutton]  GPIO 0 (Config Reset)       |
       |   [Green LED]   GPIO 13 (Status Blink)      |
       +---------------------------------------------+
```

### 6.2 Hardware Specifications
* **Module:** Sonoff Basic V1.0 (ESP8266EX, 1 MB Flash).
* **Temperature Sensor:** Dallas / Maxim DS18B20 digital temperature probe connected to GPIO 2 (with a 4.7 kΩ pull-up resistor).
* **Operating Range:** -45.0°C to +70.0°C with 11-bit resolution (0.125°C precision, ~375 ms conversion time).
* **User Interface:** Single multifunction button (GPIO 0) and status indicator LED (GPIO 13).

### 6.3 Communication Protocol (`METEO_V1`)
The sensor communicates using UDP broadcast on **Port 5005**, transmitting compact JSON payloads that any BaroClock master station on the subnet can listen to:

```json
{"proto":"METEO_V1","type":"DATA","id":"A020A6123456","ip":"192.168.1.50","temp":21.38,"rssi":-62}
```

* **Zero-Configuration Discovery:** At boot and periodically every few cycles, the sensor sends an `ANNOUNCE` packet. The BaroClock master station automatically populates its discovered sensors list in the web portal.
* **Redundant Transmission:** UDP is a fast, connectionless protocol, but packets can occasionally drop over busy home Wi-Fi networks. To guarantee delivery, the sensor transmits each data frame twice with a 50 ms gap.
* **10-Second Transmission Cadence:** Readings are refreshed every 10 seconds, balancing responsive live telemetry with low network traffic.
* **Failsafe Offline Tracking:** If the master station does not receive any broadcast from an assigned sensor for 10 minutes (60 missed intervals), the status badge on the LCD cleanly switches from `ONLINE` to `OFF`.

---

## 7. Quick Start & Setup Guide

### 7.1 Initial Master Station Setup
1. **Powering Up:** Connect the BaroClock master station to a clean 5V (2A minimum) USB-C power source.
2. **First Boot (AP Mode):** If no saved Wi-Fi networks are found, BaroClock starts its own Access Point named **`BaroClock`** (open network, no password).
3. **Connecting to Web Panel:**
   * Connect your phone or laptop to the `BaroClock` Wi-Fi network.
   * Open a web browser and navigate to `http://192.168.4.1/`.
4. **Configuring Wi-Fi:** Go to the **Networks** tab and enter your home Wi-Fi SSID and password in Slot 1. Click **Save**. BaroClock will reboot and connect to your home network.
5. **GPS Satellite Reception:** Place the device near a window or outdoors during first initialization. Once a 3D fix is acquired, the satellite icon on screen turns bright and the clock sets itself with atomic precision.

### 7.2 External Wireless Sensor Setup
1. **Power the Sensor:** Power the Sonoff module from mains AC or a 3.3V DC test header.
2. **Configuration Mode:** Hold the onboard push button (GPIO 0) for **5 seconds** until the green LED begins rapid flashing.
3. **Connect to Sensor AP:** Connect to the Wi-Fi network named **`METEO_SETUP`** (Password: `12345678`).
4. **Captive Portal:** Navigate to `http://192.168.4.1/` and save your home Wi-Fi credentials. The sensor will restart and begin broadcasting temperature packets.
5. **Assigning to Screen Slot 1:**
   * Open the BaroClock master web page -> **Networks** tab.
   * In the **Display Slot 1** section, select the newly discovered sensor ID from the dropdown list.
   * Assign a friendly custom alias (e.g., `TERASA`, `BALCON`, `OUTDOOR`) and click **Save Sensor Configuration**.
   * The reading will instantly appear in the dedicated box on the master LCD!

---

## 8. Project Directory Structure

```
ESP_32S3_Agent_DevKit/
├── Baro_Clock/
│   └── baroclock/
│       ├── baroclock.ino         # Main entry point and FreeRTOS task scheduling
│       ├── setup.h               # Hardware pinouts, theme palettes, and global constants
│       ├── gui.h / .cpp          # LVGL 9 runtime theme engine and display controller
│       ├── clock.h / .cpp        # Clock engine, slew rate synchronization, and RTC
│       ├── gps_handler.h / .cpp  # NMEA GPS/BeiDou parser and Maidenhead calculator
│       ├── bme_sensor.h / .cpp   # Bosch sensor driver and barograph rolling buffer
│       ├── weather_logger.h/.cpp # Weather forecasting math, risk index, and LittleFS storage
│       ├── sound.h / .cpp        # ES8311 I2S audio driver, chime, and alarm synthesizers
│       ├── proximity.h / .cpp    # MK-TP2016 optical IR proximity sensor driver
│       ├── temp_calib.h / .cpp   # 11-band continuous piecewise linear calibration engine
│       ├── ext_sensors.h / .cpp  # UDP port 5005 listener and remote sensor cache
│       ├── wifi_mgr.h / .cpp     # Wi-Fi multi-slot connection manager and NVS persistence
│       ├── webpage.h / .cpp      # Multi-page web dashboard server (7 subpages)
│       ├── lcd_driver.h / .cpp   # ST7796 8-bit parallel graphic display driver
│       └── src/                  # EEZ Studio generated GUI layout definitions
├── Baro_Clock_Sensor/
│   └── baro_clock_sensor/
│       ├── baro_clock_sensor.ino # ESP8266 sensor main routine and loop
│       ├── setup.h               # Pinout definitions and broadcast timing parameters
│       ├── sensor_ds18b20.h/.cpp # Dallas 1-Wire DS18B20 temperature measurement driver
│       ├── udp_protocol.h / .cpp # METEO_V1 JSON payload formatting and transmission
│       └── wifi_manager.h / .cpp # Captive portal and Wi-Fi credential management
└── README.md                     # Comprehensive system documentation
```

---

## 9. License & Acknowledgments

* **Author:** Adrian Florescu (YO3HJV)
* **GUI Engine:** [LVGL (Light and Versatile Graphics Library)](https://lvgl.io/) & [EEZ Studio](https://www.envox.eu/eez-studio/)
* **GPS Parsing:** [TinyGPSPlus by Mikal Hart](https://github.com/mikalhart/TinyGPSPlus)
* **Architecture:** Engineered for modularity, low power consumption, and long-term durability.
