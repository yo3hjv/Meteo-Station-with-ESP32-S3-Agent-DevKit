# BaroClock — ESP32 Agent DevKit
**Developed by Adrian Florescu (YO3HJV)**  
*Platform: ESP32-S3 Dev Module (16 MB Flash, OPI PSRAM active, QIO 80 MHz)*  
*Display: 4.0" 480x320 ST7796 TFT LCD (8-bit 8080 parallel interface, AW9523 I2C IO expander)*  
*Barometer / Temperature Sensor: Bosch BME280 / BMP280 on internal I2C (Chip ID auto-detection)*  
*Position / Time Sensor: NMEA GPS module on UART (GPIO 20 Rx, auto-baud 9600..115200)*  
### **From an useless device to something usefull!**
---

## 1. Project Overview

**BaroClock** is a precision combined instrument engineered for amateur radio operators, mountain/outdoor weather monitoring, and base station atmospheric tracking. It integrates:

1. **Dual Clock (Local Time & UTC Time)**: Continuously synchronized via GPS with automatic fallback to NTP over Wi-Fi.
2. **Maidenhead QTH Locator (6 Characters)**: Dynamically calculated from high-precision GPS coordinates, with manual override support.
3. **Rolling Strip Chart Barograph**: Smooth horizontal scrolling with a fixed time axis and flicker-free rendering.
4. **Synthetic Weather Forecast Engine**: Powered by barometric pressure change rates, historical trend analysis, and persistent storage in LittleFS correlated with geographical location.
5. **Wi-Fi Web Portal (AP & STA)**: Real-time telemetry, QFE (ground-level) barometer calibration, 11-point multi-range temperature calibration, pressure/temperature unit selection, and network configuration.
6. **Audio System & Optical Proximity Sensor**: Proximity sound test upon hand approach (1200 Hz / 300 ms alert beep) driven by an ES8311 I2S audio codec with integrated PA amplifier and an MK-TP2016 optical proximity sensor.

---

## 2. How the Barometric Forecast Works (The 3 Elements)

In the central area of the LCD ($Y = 162..248$), the atmospheric forecast is conveyed through three complementary indicators: the **Graphic Icon**, the **Trend Arrow**, and the **Atmospheric Risk/State Label**.

### A. The Arrow (Baric Trend — Short-Term Pressure Direction)
Indicates **where the pressure is moving right now**, calculated from the hourly rate of change ($hPa/h$) and the 3-hour moving differential ($\Delta P_{3h}$):
* **Green Up Arrow ($\uparrow$)**: Rising pressure ($\ge +0.25\text{ hPa/h}$ or $\Delta P_{3h} \ge +0.5\text{ hPa}$).
* **Yellow Horizontal Arrow ($\rightarrow$)**: Stable pressure (variation between $-0.25$ and $+0.25\text{ hPa/h}$).
* **Red Down Arrow ($\downarrow$)**: Falling pressure ($\le -0.25\text{ hPa/h}$ or $\Delta P_{3h} \le -0.5\text{ hPa}$).

### B. The Text Label (Baric Risk Level / Atmospheric Stability)
Evaluates the **magnitude and violence of the pressure drop** to assess storm, wind, and severe weather potential:
* `[STABLE]` *(Green)*: Calm atmosphere. Pressure variations are slow and within normal diurnal boundaries (drop under $1.0\text{ hPa}$ in 3 hours).
* `[CAUTION]` *(Yellow)*: Moderate drop ($\Delta P_{3h} < -1.0\text{ hPa}$ or $\Delta P_{1h} < -0.6\text{ hPa}$). Possible cloud buildup or incoming weather transition.
* `[WEATHER DETERIORATION]` *(Orange)*: Pronounced drop ($\Delta P_{3h} < -2.0\text{ hPa}$ or $\Delta P_{1h} < -1.2\text{ hPa}$). High probability of precipitation and gusty winds.
* `[STORM ALERT]` *(Bright Red)*: Steep pressure plunge ($\Delta P_{3h} < -3.5\text{ hPa}$, $\Delta P_{1h} < -2.0\text{ hPa}$, or a sharp drop in 30 minutes with negative acceleration). Imminent storm or gale warning.
* `[NEW LOCATION]` *(Cyan)*: The device was moved to a new Maidenhead QTH Locator and is acquiring initial local samples (< 4 local readings / 12 minutes).

> [!NOTE]  
> **Why can a Red Down Arrow ($\downarrow$) appear together with `[STABLE]`?**  
> Pressure can decrease along a very gentle, steady slope (for example, $-0.3\text{ hPa}$ over 3 hours).  
> * The arrow reflects pure direction: **falling** ($\downarrow$).  
> * The text evaluates meteorological severity: because the drop rate is gentle, the risk of bad weather is virtually zero, hence the condition remains classified as **`[STABLE]`**.

### C. Large Graphic Icon (Synthesized Weather Condition)
The icon canvas ($76 \times 86$ px) renders a weather symbol by merging the trend with the evaluated risk level:
1. **Sun with 8 Rays**: `[STABLE]` state with rising trend ($\uparrow$) — clear skies, fair weather.
2. **Sun behind Cloud**: `[STABLE]` state with steady or slightly falling trend — partly cloudy or stable skies.
3. **Dense Overcast Cloud**: `[CAUTION]` state — low cloud ceiling, overcast conditions.
4. **Rain Cloud with Droplets**: `[WEATHER DETERIORATION]` state — active or highly probable precipitation.
5. **Thunderstorm with Lightning**: `[STORM ALERT]` state — cold front passage, electrical discharges, strong winds.
6. **Radar / GPS Reticle**: `[NEW LOCATION]` state — baseline calibration underway for the new geographic area.

---

## 3. Web Portal Features

Access the web portal by connecting to the clock's IP address or fallback hotspot (`http://192.168.4.1`):

1. **Wi-Fi Manager (`/wifi`)**: Stores up to 3 prioritized network profiles with automatic reconnect and fallback to Access Point mode (`BaroClock`).
2. **QFE Barometer Calibration (`/sensors`)**: Dedicated field to calibrate ground-level pressure against a local certified barometer.
3. **Multi-Point Temperature Calibration (`/temp_calib`)**:
   - **Quick Calibrate**: Enter the current reading from a reference thermometer to instantly calculate and apply a baseline offset with zero residual error ($0.00^\circ\text{C}$).
   - **11-Range Piecewise Linear Interpolation**: Individually tune offsets across 11 temperature bands from $-20^\circ\text{C}$ to $+45^\circ\text{C}$ for both the internal BME280/BMP280 and External Sensor 1 (DS18B20).
   - Unrestricted decimal input (`step='any'`).
4. **Pressure Units Selection**: Toggle between **hPa**, **mb**, and **mmHg** across the LCD interface and Web portal.
5. **Manual QTH Locator**: Ability to manually assign the Maidenhead grid locator when operating without GPS lock.
6. **Ground Truth Weather Reference**: Quick selectors (*Clear / Sunny*, *Partly Cloudy*, *Overcast*, *Fog*, *Rain*, *Storm*, *Snow*) to anchor the forecast baseline during prolonged stationary pressure conditions.
7. **Sound & Proximity Diagnostics**: Enable or disable the hand-approach proximity chime (default ON), monitor raw proximity counts, and trigger a manual speaker test tone.
8. **Display & Time Settings**: 180° screen rotation, temperature unit (°C / °F), Daylight Saving Time (DST +1h), and barograph rolling chart time-span selection.

---

## 4. Hardware Wiring & Pinout Reference

| Function / Signal | ESP32-S3 GPIO | Connected Peripheral | Notes |
| :--- | :---: | :--- | :--- |
| **LCD Data Bus (D0–D7)** | GPIO 39–46 | ST7796 8-bit Parallel | High-speed DMA/8080 parallel bus |
| **LCD WR / RD / RS / CS** | GPIO 47, 48, 2, 1 | ST7796 Control Lines | Hardware-driven strobe |
| **I2C SDA / SCL** | GPIO 8, 9 | BME280, AW9523, MK-TP2016 | Shared internal sensor bus (400 kHz) |
| **GPS UART Rx** | GPIO 20 | NMEA GPS Module Tx | Hardware Serial (Auto-baud detection) |
| **I2S Audio (BCLK/LRCK/DOUT)** | GPIO 12, 13, 14 | ES8311 Codec + PA | Clean digital audio output |
| **Proximity Sensor** | I2C (0x39) | MK-TP2016 Optical Sensor | IR proximity detection |

---

## 5. Changelog

### v0.3.2
- **Temperature Calibration Overhaul**:
  - Replaced restrictive HTML5 numeric steps with `step='any'` on all calibration fields to allow arbitrary decimal input without browser validation errors.
  - Upgraded Quick Calibration to apply a uniform baseline translation across the sensor curve, guaranteeing exact agreement ($0.00^\circ\text{C}$ error) between reference and displayed values.
  - Synchronized External Sensor Slot 1 multi-point calibration between UDP reception, LCD GUI, and the web configuration interface.
