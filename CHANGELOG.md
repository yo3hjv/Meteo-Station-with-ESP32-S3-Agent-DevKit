
## Version History
### [v0.3.6] - 2026-11-09

#### Added / Enhanced
* **Priority Alarm Silencing via Touchscreen**:
  * Touching anywhere on the display while an alarm (wake-up alarm or storm siren) is actively ringing immediately stops all acoustic alerts (`soundStopAllAlarms()`).
  * The touch event is suppressed from passing to LVGL (`LV_INDEV_STATE_RELEASED`), preventing accidental widget taps or theme changes during alarm dismissal.
* **Long Touch Theme Cycling in Clocks Container**:
  * Long pressing inside the clocks and location container (`objects.clock_loc_info_container`, $X \in [3, 274]$, $Y \in [6, 196]$) cycles forward through the 6 LVGL themes in a wrap-around sequence (`(theme + 1) % 6`).
  * Plays tactile feedback click sound on switch and saves the new theme persistently to NVS (`settingsSetUiTheme()`).
* **Configurable Long Touch & Debounce Timers**:
  * Added `DEFAULT_LONG_TOUCH_MS` (1000 ms) and `DEFAULT_TOUCH_DEBOUNCE_MS` (500 ms) in `setup.h`.
  * Added settings and NVS storage for Long Touch duration and Touch Debounce lockout in both `/settings` (Device Settings) and `/touch_feel` (Touch & Feel) web subpages.
* **Hand Gesture Alarm Dismissal**:
  * Front optical sensor (`MK-PB2016`) detects hand wave/approach (`Raw >= 300`) to silence ringing alarms without touching the screen.
  * Added configurable toggle checkbox in the `/sound_alarms` web subpage (`settingsSetAlarmDismissGesture()`).
* **Filtered Ambient Light Sensing for Auto-Dimming**:
  * Implemented moving-average filtered ambient light sampling (`proximityGetFilteredAmbient()`) with configurable threshold and inverted light mode support.

### [v0.3.5] - 2026-11-08

#### Added / Enhanced
* **Hardware-Driven Gradual Backlight Dimming & Auto-Dimming (AW9523)**:
  * Enabled 256-step constant-current LED dimming mode on AW9523 IO expander channels `P1_0..P1_3` connected in parallel to LCD backlight cathodes (`LCD_LEDK`).
  * Implemented flicker-free, non-blocking smooth brightness fading (`lcdUpdateBacklight()`).
  * Added persistent NVS storage for Day Brightness (`bl_day`), Night Brightness (`bl_night`), and Auto-Dimming switch (`bl_auto`).
  * Converted the front optical sensor (`MK-PB2016`) into an Ambient Light Sensor (ALS): automatically detects room lighting conditions (0 counts in total darkness, ~2200+ counts in normal room light) and seamlessly transitions between Night and Day brightness settings.
  * Added interactive Day and Night brightness range sliders and ambient light live telemetry to the **Touch & Feel** web subpage (`/touch_feel`). SEE PREVIOUS VERSION.
---

### [v0.3.4] - 2026-11-08

#### Fixed / Enhanced
* **Strict Independent Band-Based Temperature Calibration**:
  * Corrected single-band calibration logic: Quick Calibrate and sensor calibration now replace **only** the offset of the currently active temperature range (`[minT, maxT)`), completely leaving all other 10 bands untouched.
  * Direct band offset evaluation ensures that the displayed calibrated temperature immediately and accurately matches the entered reference value with **zero residual error** ($0.00^\circ\text{C}$).
  * Synchronized both internal BME280/BMP280 and External Wireless Sensor Slot 1 (DS18B20) to this band-isolated model.
* **Optical Proximity Sensor (MK-PB2016 / SO6103)**:
  * Enabled hardware proximity detection (`ENABLE_PROXIMITY_SENSOR 1`) on the internal I2C bus (`0x39`).
  * Activated real-time diagnostic logging and boot register dump (`DEBUG_PROXIMITY 1`).
  * Verified live telemetry updates in the web portal and acoustic chime test upon hand proximity.
  **FOUND HARDWARE ISSUE WITH THE IR DIODE SWITCHING TRANSISTOR: Wrong Footprint, must be unsoldered from the FPS (Flexible cable) and solder bellyup in order for the proximity ensor to work. If not, the proximity sensor can be used as day/night dimmer with controls on the webpage**.
---

### [v0.3.3] - 2026-11-08

#### Fixed
* **Multi-Point Temperature Calibration Engine**:
  * Fixed browser rejection caused by restrictive HTML5 numeric attributes; replaced `step='0.05'` and `step='0.1'` with `step='any'` across QFE, reference, and 11-point table inputs.
  * Resolved persistent residual post-calibration offset in Quick Calibration by applying a uniform baseline shift across all interpolation nodes ($\Delta = T_{ref} - T_{meas}$), achieving immediate $0.00^\circ\text{C}$ error.
  * Synchronized External Sensor Slot 1 multi-band calibration between UDP telemetry reception, LCD display routines, and the web configuration manager.


---

### [v0.3.2] - 2026-10-15

#### Added
* **Acoustic Warning System**:
  * Integrated ES8311 I2S audio codec with AW9523-controlled power amplifier.
  * Hourly chime (double beep), severe weather drop warning, and Morse code S.O.S. alert for baric plunges.
* **Proximity Sensor Diagnostics**:
  * Added MK-TP2016 optical proximity driver and live telemetry monitoring.

---

### [v0.3.1] - 2026-09-02

#### Added
* **Enhanced Barograph & Historical Trends**:
  * 12h, 24h, 48h, and 72h selectable rolling strip-chart barograph in PSRAM.
  * LittleFS persistent historical logging linked to geographic Maidenhead locator.
* **Multi-Network Wi-Fi**:
  * 3 prioritized Wi-Fi network credential slots with automatic failover and SoftAP configuration portal.

