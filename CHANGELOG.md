# Changelog & Flashing Memory Map — BaroClock

All notable changes to the **BaroClock** project will be documented in this file.

---

## ESP Flash Download Tool & esptool.py Memory Map

When flashing precompiled binary files directly to the **ESP32-S3** board using **Espressif Flash Download Tool** or `esptool.py`, use the following memory addresses:

### Flashing Address Table (ESP32-S3)

| Binary File | Target Flash Offset | Description / Purpose |
| :--- | :---: | :--- |
| **`bootloader.bin`** | **`0x0000`** | First-stage hardware bootloader *(ESP32-S3 uses `0x0000`, NOT `0x1000`)* |
| **`partitions.bin`** | **`0x8000`** | Partition table defining NVS, OTA, APP, and LittleFS layout |
| **`boot_app0.bin`** | **`0xe000`** | OTA data selector initializing the active boot partition |
| **`baroclock.ino.bin`** | **`0x10000`** | Main application compiled firmware |

> [!WARNING]  
> **ESP32-S3 Bootloader Address**:  
> Unlike classic ESP32 chips where the bootloader is placed at `0x1000`, the **ESP32-S3 requires the bootloader at `0x0000`**. Flashing the bootloader at `0x1000` on an ESP32-S3 will result in a completely silent, unbootable board.

### ESP Flash Download Tool Recommended Settings
* **Chip Type**: `ESP32-S3`
* **WorkMode**: `develop`
* **SPI SPEED**: `80MHz`
* **SPI MODE**: `QIO` (or `DIO` depending on module hardware)
* **FLASH SIZE**: `128Mbit (16MB)`

### esptool.py Command Example
```bash
python -m esptool --chip esp32s3 -p COMx -b 921600 --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 baroclock.ino.bin
```

---

## Version History

### [v0.3.3] - 2026-11-08

#### Fixed
* **Multi-Point Temperature Calibration Engine**:
  * Fixed browser rejection caused by restrictive HTML5 numeric attributes; replaced `step='0.05'` and `step='0.1'` with `step='any'` across QFE, reference, and 11-point table inputs.
  * Resolved persistent residual post-calibration offset in Quick Calibration by applying a uniform baseline shift across all interpolation nodes ($\Delta = T_{ref} - T_{meas}$), achieving immediate $0.00^\circ\text{C}$ error.
  * Synchronized External Sensor Slot 1 multi-band calibration between UDP telemetry reception, LCD display routines, and the web configuration manager.

#### Added
* **Hobbyist Documentation**:
  * Comprehensive English `README.md` containing Arduino IDE board options, partition settings, and hardware wiring diagrams.
  * Standalone `CHANGELOG.md` with flash memory mapping for ESP Flash Tool.

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

