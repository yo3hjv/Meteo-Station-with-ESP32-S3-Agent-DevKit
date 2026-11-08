**Developed by Adrian Florescu (YO3HJV)**  
*Hardware Platform: ESP32-S3 Dev Module (16 MB Flash, Octal SPI / OPI PSRAM, QIO 80 MHz)*  
*Display: 4.0" 480×320 ST7796 TFT LCD (8-bit 8080 Parallel bus with AW9523 IO Expander)*  
*Atmospheric Sensor: Bosch BME280 / BMP280 (Internal Secondary I2C bus, auto-detection)*  
*Position & Time Source: NMEA GPS Engine (UART1, Hardware Serial Rx: GPIO 41, Tx: GPIO 40)*  
*Audio System: ES8311 I2S Audio Codec with dedicated Power Amplifier & On-board Speaker*  
*Touch Controller: FT6336 Capacitive Multi-Touch Controller*  

### **From an useless device to something usefull!**
# BaroClock — ESP32-S3 Weather Station & Precision Barometer
---

## 1. Project Overview

**BaroClock** is an advanced meteorological monitor and precision instrument engineered for radio amateurs, mountain/outdoor weather enthusiasts, base stations, and electronics hobbyists. Combining high-speed hardware with continuous environmental analysis, BaroClock delivers:

1. **Dual Synchronization Clock (Local & UTC Time)**: Automatically synchronized via hardware GPS (UART NMEA engine) with seamless fallback to NTP via Wi-Fi.
2. **Dynamic Maidenhead QTH Locator (6 Characters)**: Computes the precise 6-character amateur radio grid locator from live GPS satellite coordinates, with manual override support via the web interface.
3. **Rolling Strip-Chart Barograph**: Continuous barometric pressure recording rendered on a dedicated screen layer allocated in external OPI PSRAM, featuring flicker-free updates and selectable time spans (12h, 24h, 48h, 72h).
4. **Synthetic Weather Forecast Engine**: Advanced algorithm analyzing pressure trend rates ($hPa/h$) and 3-hour differentials ($\Delta P_{3h}$) correlated with geographic location and persisted in LittleFS flash storage.
5. **Multi-Sensor Telemetry**: Supports internal BME280/BMP280 alongside remote wireless UDP telemetry sensors (Slot 1 / DS18B20 external probes).
6. **Acoustic Warning System**: Configurable audio alarms through the ES8311 I2S codec, including hourly chimes, severe weather alerts, Morse code storm warnings, and daily alarms.
7. **Integrated Web Portal**: Fully responsive web management interface accessible via local Wi-Fi or on-board SoftAP (`http://192.168.4.1`).

---

## 2. Arduino IDE Setup & Hardware Programming

To compile and upload BaroClock to the ESP32-S3 board, configure the **Arduino IDE** (v2.x recommended) with the official Espressif ESP32 core (`>= 2.0.14` or `3.0.x`).

### A. Arduino IDE Menu Settings

Open the sketch from [`baroclock/baroclock.ino`](file:///E:/PROIECTE%20ELECTRONICA/ESP_32S3_Agent_DevKit/Baro_Clock/baroclock/baroclock.ino) and select the following parameters under the **Tools** menu:

| Arduino IDE Menu Option | Required Value | Notes / Explanation |
| :--- | :--- | :--- |
| **Board** | `ESP32S3 Dev Module` | Standard target definition for ESP32-S3 chips |
| **USB CDC On Boot** | `Enabled` | Enables the native USB port for Serial Monitor logging |
| **CPU Frequency** | `240MHz (WiFi)` | Maximum clock speed for smooth LVGL rendering |
| **Flash Mode** | `QIO 80MHz` | Quad I/O High Speed Flash access |
| **Flash Size** | `16MB (128Mb)` | **Mandatory:** Matches the 16 MB physical NOR flash |
| **Partition Scheme** | `16M Flash (3MB APP/9.9MB LittleFS)` | Provides ample room for application code and historical records |
| **PSRAM** | `OPI PSRAM` | **Mandatory:** Octal SPI PSRAM is required for LCD framebuffers |
| **Upload Mode** | `UART0 / Hardware CDC` | Direct flashing via on-board USB port |
| **USB Mode** | `Hardware CDC and JTAG` | Default standard hardware USB support |
| **Core Debug Level** | `None` (or `Info` for debugging) | Set to `None` for optimal performance in production |
| **Erase All Flash Before Upload** | `Disabled` | Set `Enabled` only when performing a clean initial wipe |

> [!IMPORTANT]  
> If **PSRAM** is not set to `OPI PSRAM`, the graphical engine and barograph buffer initialization will fail, causing boot loop restarts. Ensure your board is the 16MB Flash + 8MB OPI PSRAM variant.

### B. Required Libraries

Install the following libraries via the Arduino Library Manager or project dependencies:
* `Wire`, `WiFi`, `WebServer`, `Preferences`, `LittleFS`, `SPI` (Included in ESP32 Core)
* `TinyGPSPlus` (by Mikal Hart) — for GPS NMEA sentence parsing
* `Adafruit BME280 Library` & `Adafruit Unified Sensor`
* `LVGL` (v9.x supported)

---

## 3. Web Portal Administration Guide

Connect your computer, tablet, or smartphone to the same Wi-Fi network as the BaroClock, or connect directly to the fallback access point:
* **Access Point SSID**: `BaroClock`
* **Default IP Address**: `http://192.168.4.1`

### A. Main Live Dashboard (`/`)
* **Real-Time Telemetry**: Current station barometric pressure (QFE / QNH), temperature, relative humidity, and dew point.
* **External Sensors**: Live status and readings from wireless UDP environmental probes (Slot 1..3).
* **Clock & Satellite Status**: GPS fix indicator, active satellite count, UTC time, Local time, and computed Maidenhead QTH locator.
* **Baric Trend & Forecast**: Current atmospheric stability badge (`STABLE`, `CAUTION`, `WEATHER DETERIORATION`, `STORM ALERT`) and directional pressure trend arrow.

### B. Wi-Fi Manager (`/wifi`)
* Supports storing up to **3 prioritized Wi-Fi networks** (SSID + Password).
* The ESP32-S3 attempts connection in order of priority; if none are available, it automatically activates the `BaroClock` SoftAP fallback.
* Physical Wi-Fi reset: Pulling **GPIO 46** LOW at startup triggers an emergency Wi-Fi configuration reset.

### C. Barometer Calibration (`/sensors`)
* **QFE Ground-Level Pressure Calibration**: Enter the calibrated pressure reading from an official local certified weather station.
* Allows fine-tuning the electronic pressure sensor to compensate for packaging offset and component aging.

### D. Multi-Point Temperature Calibration (`/temp_calib`)
BaroClock provides high-precision dual-mode temperature calibration for both internal (BME280) and external sensors:
1. **Quick Calibrate**:
   * Enter the exact temperature reported by a calibrated reference thermometer.
   * The firmware automatically computes the required shift and performs a **uniform baseline translation** across all calibration points.
   * Guarantees immediate zero-error ($0.00^\circ\text{C}$) alignment between actual and indicated readings.
2. **11-Point Piecewise Linear Interpolation**:
   * For advanced users, individual correction offsets can be defined across 11 temperature points (ranging from $-20^\circ\text{C}$ to $+45^\circ\text{C}$).
   * All form fields support decimal values (`step='any'`).

### E. Units & Regional Preferences
* **Barometric Units**: Toggle between **hPa**, **mb**, and **mmHg** across the LCD interface and Web portal.
* **Temperature Units**: Switch between Celsius (**°C**) and Fahrenheit (**°F**).
* **Time & Daylight Saving**: Configurable UTC time offset and automatic Daylight Saving Time (+1h) toggle.
* **Manual QTH Locator**: Manually assign the 6-character Maidenhead locator for stationary indoor operations where GPS signals cannot penetrate.

### F. Audio, Alarms & Proximity Settings
* **Hourly Chime**: Enable or disable double-beep chime at the top of each hour with adjustable volume.
* **Weather Alarms**: Configure audio warning tones for rapid pressure drops (`WEATHER DETERIORATION`) and severe storm alerts (`Morse S.O.S`).
* **Display Orientation**: 180° software screen rotation toggle to accommodate custom enclosures.
* **Proximity Sensor**: Enable hand-approach proximity detection (MK-TP2016) to automatically awaken the screen or trigger an approach tone.

---

## 4. Hardware Wiring & Pinout Reference

| Function / Signal | ESP32-S3 GPIO | Connected Peripheral | Description / Notes |
| :--- | :---: | :--- | :--- |
| **LCD Data Bus (DB0–DB7)** | GPIO 9, 3, 8, 18, 17, 16, 15, 7 | ST7796 8-bit Parallel | High-speed 8080 parallel display bus |
| **LCD Control (RS / WR)** | GPIO 45, 10 | ST7796 Command / Write | Hardware strobe lines |
| **LCD Reset** | *P1_6 (AW9523)* | ST7796 Reset pin | Controlled via AW9523 IO expander |
| **Primary I2C (SDA / SCL)** | GPIO 5, 4 | AW9523, FT6336, MK-TP2016 | Address `0x59` (IO Expander), `0x38` (Touch), `0x39` (Proximity) |
| **Secondary I2C (SDA / SCL)**| GPIO 48, 38 | Bosch BME280 / BMP280 | Dedicated sensor bus (Address `0x76`) |
| **GPS UART (Rx / Tx)** | GPIO 41, 40 | NMEA GPS Module (GP-02) | Hardware Serial (Auto-baud 9600..115200) |
| **Audio I2S (MCLK)** | GPIO 47 | ES8311 Audio Codec | Master Clock for digital audio codec |
| **Audio I2S (BCLK / WS / DOUT)**| GPIO 14, 13, 12 | ES8311 Audio Codec | Bit Clock, Word Select, and Serial Data |
| **Audio PA Enable** | *P1_5 (AW9523)* | Power Amplifier Enable | Mute/unmute speaker power amplifier |
| **Touch Interrupt (INT)** | GPIO 6 | FT6336 Touch Controller | Hardware touch interrupt |
| **Emergency Wi-Fi Reset** | GPIO 46 | Pushbutton / Jumper | Pull LOW during boot to reset credentials |

---

## 5. Understanding the Weather Forecast Display

The central weather forecast panel synthesizes three complementary indicators:

```
+-----------------------------------------------------------------+
|   [Graphic Icon]            [Arrow]            [State Label]    |
|   Sun / Cloud / Storm   Rising / Falling       STABLE / STORM   |
+-----------------------------------------------------------------+
```

1. **Directional Arrow ($\uparrow$, $\rightarrow$, $\downarrow$)**:
   * Reflects instantaneous pressure tendency over the last 1-hour and 3-hour moving window.
   * **Green Up ($\uparrow$)**: Pressure rising ($\ge +0.25\text{ hPa/h}$).
   * **Yellow Right ($\rightarrow$)**: Steady atmospheric pressure ($\pm 0.25\text{ hPa/h}$).
   * **Red Down ($\downarrow$)**: Pressure falling ($\le -0.25\text{ hPa/h}$).

2. **Atmospheric Severity Badge**:
   * Evaluates the physical hazard of the pressure drop:
   * **`[STABLE]`**: Calm conditions (drop $< 1.0\text{ hPa}$ in 3h).
   * **`[CAUTION]`**: Moderate drop ($-1.0$ to $-2.0\text{ hPa}$ in 3h). Potential wind or cloud cover.
   * **`[WEATHER DETERIORATION]`**: Significant drop ($-2.0$ to $-3.5\text{ hPa}$ in 3h). Precipitation expected.
   * **`[STORM ALERT]`**: Severe plunge ($> -3.5\text{ hPa}$ in 3h or violent short-term drop). Storm / gale warning.

3. **Graphic Weather Icon**:
   * Merges trend dynamics with barometric pressure levels into an intuitive graphic symbol: Clear Sun, Sun behind Clouds, Overcast, Rain, Storm Lightning, or Radar Reticle (new location baseline calibration).

---

## 6. Project Documentation & History

* For the detailed list of version changes, bug fixes, and memory flashing addresses for ESP Flash Tool, refer to **CHANGELOG.md)**.
