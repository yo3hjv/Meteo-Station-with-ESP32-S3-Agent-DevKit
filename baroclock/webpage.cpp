#include "webpage.h"
#include "wifi_mgr.h"
#include "gps_handler.h"
#include "bme_sensor.h"
#include "lcd_driver.h"
#include "weather_logger.h"
#include "sound.h"
#include "proximity.h"
#include "setup.h"
#include "gui.h"
#include "clock.h"
#include "ext_sensors.h"
#include "temp_calib.h"
#include <WebServer.h>

static WebServer server(80);

// ============================================================================
// COMMON HTML / CSS HEADER & NAVIGATION BAR
// ============================================================================
static String getCommonHeader(const String& title, const String& activeTab) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>BaroClock - " + title + "</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#181818;color:#eee;margin:0;padding:15px;}";
    html += ".card{background:#262626;padding:15px;border-radius:8px;margin-bottom:15px;max-width:580px;box-shadow:0 2px 5px rgba(0,0,0,0.5);}";
    html += "h2,h3,h4{color:#00e5ff;margin-top:0;}";
    html += "label{display:block;margin:8px 0 3px;}";
    html += "input[type='text'],input[type='number'],input[type='password'],select{width:100%;padding:8px;border-radius:4px;border:1px solid #555;background:#333;color:#fff;box-sizing:border-box;}";
    html += "table input[type='number']{padding:4px 2px!important;text-align:center!important;height:32px!important;-moz-appearance:textfield!important;}";
    html += "table input[type='number']::-webkit-inner-spin-button,table input[type='number']::-webkit-outer-spin-button{-webkit-appearance:none!important;margin:0!important;}";
    html += ".tbl-wrap{overflow-x:auto;-webkit-overflow-scrolling:touch;margin-bottom:10px;}";
    html += "table{width:100%;border-collapse:collapse;font-size:0.9em;}";
    html += "th,td{padding:8px 6px;text-align:center;border-bottom:1px solid #3a3a3a;}";
    html += "th{background:#1f1f1f;color:#00bcd4;}";
    html += "input[type='submit'],.btn{background:#00bcd4;color:#000;border:none;padding:10px 15px;font-weight:bold;border-radius:4px;cursor:pointer;margin-top:10px;width:100%;text-align:center;text-decoration:none;display:inline-block;box-sizing:border-box;}";
    html += ".btn-reset{background:#f44336;color:#fff;margin-top:5px;}";
    html += ".btn-sec{background:#444;color:#fff;padding:8px;font-size:0.85em;margin:3px 0;width:100%;border-radius:4px;text-align:center;text-decoration:none;display:inline-block;box-sizing:border-box;border:1px solid #666;}";
    html += ".btn-sec:hover{background:#555;}";
    html += ".btn-calib{background:#2e4450;color:#00e5ff;border:1px solid #00acc1;padding:8px 4px;font-size:0.85em;text-align:center;text-decoration:none;border-radius:4px;display:inline-block;}";
    html += ".btn-calib:hover{background:#00acc1;color:#000;}";
    html += ".badge{display:inline-block;padding:3px 8px;border-radius:4px;font-weight:bold;}";
    html += ".bg-green{background:#4caf50;color:#000;} .bg-red{background:#f44336;color:#fff;}";
    html += ".grid-calib{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-top:8px;}";
    html += ".grid-test{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px;}";
    html += ".nav-bar{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:15px;max-width:580px;}";
    html += ".nav-btn{flex:1 1 calc(33.333% - 6px);text-align:center;padding:9px 4px;background:#333;color:#ccc;text-decoration:none;border-radius:6px;font-weight:bold;border:1px solid #444;font-size:0.86em;box-sizing:border-box;transition:0.2s;}";
    html += ".nav-btn:hover{background:#444;color:#fff;}";
    html += ".nav-btn.active{background:#00bcd4;color:#000;border-color:#00e5ff;}";
    html += "</style></head><body>";

    html += "<div class='card'>";
    html += "<h2>BaroClock - Control Panel</h2>";
    html += "<p>Firmware Version: <b>" + String(FIRMWARE_VERSION) + "</b></p>";
    html += "<p>Network Mode: <b>" + (wifiMgrIsAP() ? String("Access Point (AP)") : ("Connected to: " + wifiMgrGetSSID())) + "</b></p>";
    html += "<p>Assigned IP Address: <b>" + wifiMgrGetIP() + "</b></p>";
    html += "</div>";

    html += "<div class='nav-bar'>";
    html += "<a href='/' class='nav-btn" + String(activeTab == "dashboard" ? " active" : "") + "'>Dashboard</a>";
    html += "<a href='/touch_feel' class='nav-btn" + String(activeTab == "touch_feel" ? " active" : "") + "'>Touch &amp; Feel</a>";
    html += "<a href='/sound_alarms' class='nav-btn" + String(activeTab == "sound_alarms" ? " active" : "") + "'>Sound &amp; Alarms</a>";
    html += "<a href='/weather' class='nav-btn" + String(activeTab == "weather" ? " active" : "") + "'>Weather</a>";
    html += "<a href='/networks' class='nav-btn" + String(activeTab == "networks" ? " active" : "") + "'>Networks</a>";
    html += "<a href='/temp_calib' class='nav-btn" + String(activeTab == "temp_calib" ? " active" : "") + "'>Temp Calib</a>";
    html += "<a href='/settings' class='nav-btn" + String(activeTab == "settings" ? " active" : "") + "'>Settings</a>";
    html += "</div>";

    return html;
}

// ============================================================================
// 1. MAIN DASHBOARD PAGE (/)
// ============================================================================
static void handleRoot() {
    const GpsData& g = gpsGetData();
    const BmeData& b = bmeGetData();

    String html = getCommonHeader("Main Dashboard", "dashboard");

    // 1. Wakeup Alarm & Chime Summary Card
    bool almEn = settingsGetAlarmEnabled();
    uint8_t almHr = settingsGetAlarmHour();
    uint8_t almMin = settingsGetAlarmMinute();
    uint8_t almPer = settingsGetAlarmPeriodicity();
    bool chimeEn = settingsGetChimeEnabled();

    char almBuf[16];
    snprintf(almBuf, sizeof(almBuf), "%02u:%02u", almHr, almMin);
    const char *perStr = (almPer == 1) ? "Mon-Fri" : (almPer == 2 ? "Once" : "Daily");

    html += "<div class='card'>";
    html += "<h3>Wakeup Alarm & Audio State</h3>";
    html += "<p>Alarm Clock: <span class='badge " + String(almEn ? "bg-green'>ACTIVE (ON)" : "style='background:#555;color:#ccc;'>DISABLED (OFF)") + "</span>";
    html += " | Scheduled Time: <b style='color:#00e5ff;font-size:1.25em;'>" + String(almBuf) + "</b> (Local Time, 24H)</p>";
    html += "<p>Periodicity: <b>" + String(perStr) + "</b> | Hourly Chime: <span class='badge " + String(chimeEn ? "bg-green'>ACTIVE" : "style='background:#555;color:#ccc;'>MUTED") + "</span></p>";
    html += "<a href='/sound_alarms' class='btn-sec' style='margin-top:8px;'>⚙️ Configure Alarm & Audio Parameters &rarr;</a>";
    html += "</div>";

    // 2. GPS & Maidenhead Status Card
    html += "<div class='card'>";
    html += "<h3>GPS &amp; Maidenhead Status</h3>";
    html += "<p>UART Stream: <span class='badge " + String(g.present ? "bg-green'>DETECTED" : "bg-red'>DISCONNECTED") + "</span>";
    html += " | Satellite Fix: <span class='badge " + String(g.fix ? "bg-green'>VALID 3D FIX" : "bg-red'>NO FIX") + "</span></p>";
    html += "<p>Latitude: <b>" + String(g.lat, 6) + "</b> | Longitude: <b>" + String(g.lon, 6) + "</b></p>";
    html += "<p>Maidenhead Locator: <b style='font-size:1.3em;color:#ffeb3b;'>" + String(g.maidenhead) + "</b></p>";
    const GnssSatellitesData &sats = gpsGetSatellitesData();
    html += "<p>Satellites: <b>" + String(sats.satsVisible) + " tracked / " + String(sats.satsInFix) + " in fix</b> | HDOP: <b>" + String(g.hdop, 1) + "</b></p>";
    html += "</div>";

    // 3. Clock Engine Status Card
    html += "<div class='card'>";
    html += "<h3>Clock Engine &amp; Slew Synchronization</h3>";
    html += "<p>Sync State: <span class='badge " + String(clockIsSynchronized() ? "bg-green'>SYNCHRONIZED" : "bg-red'>UNSYNCHRONIZED") + "</span>";
    if (clockIsSlewing()) {
        html += " | Slew: <span class='badge' style='background:#ff9800;color:#000;'>ACTIVE SLEW (" + String(clockGetSlewRemainingSec()) + "s left)</span>";
    } else {
        html += " | Slew: <span class='badge' style='background:#555;color:#ccc;'>STABLE</span>";
    }
    html += "</p>";
    html += "<p>Time Source: <b>" + String(clockGetSyncSourceStr()) + "</b> | Last Drift: <b>" + String(clockGetLastOffsetMs(), 1) + " ms</b></p>";
    html += "<p>Last Sync Info: <b>" + clockGetLastSyncStatus() + "</b></p>";
    html += "<p style='font-size:0.85em;color:#aaa;'>Schedule: Automatic smooth sync every " + String(CLOCK_SYNC_INTERVAL_HOURS) + "h starting at " + String(CLOCK_SYNC_START_HOUR) + ":00 AM (3 min Slew window).</p>";
    html += "<div style='display:flex;gap:10px;margin-top:10px;'>";
    html += "<a href='/sync_clock_slew' class='btn' style='margin-top:0;'>Sync Now (3m Slew)</a>";
    html += "<a href='/sync_clock_step' class='btn btn-sec' style='margin-top:0;'>Immediate Step Sync</a>";
    html += "</div>";
    html += "</div>";

    // 4. Sensors Live Quick Glance Card
    ExtSensorSlot extS1;
    extSensorGetSlot(1, extS1);
    const WeatherForecastInfo& fc = weatherGetForecast();

    String badgeColorClass = "bg-green";
    if (fc.risk == RISK_CAUTION) badgeColorClass = "style='background:#ffeb3b;color:#000;'";
    else if (fc.risk == RISK_DETERIORATION) badgeColorClass = "style='background:#ff9800;color:#000;'";
    else if (fc.risk == RISK_STORM) badgeColorClass = "bg-red";
    else if (fc.risk == RISK_NEW_LOCATION) badgeColorClass = "style='background:#00e5ff;color:#000;'";

    html += "<div class='card'>";
    html += "<h3>Live Sensors &amp; Weather Overview</h3>";
    html += "<p>Internal BME280: <b>" + String(b.tempC, 2) + " &deg;C</b> (" + String(b.tempF, 1) + " &deg;F) | Pressure: <b style='color:#4caf50;'>" + String(b.pressureCalibratedMb, 1) + " mb</b></p>";
    html += "<p>External Sensor 1 (" + String(extS1.alias) + "): <b style='color:#00e5ff;'>" + (extS1.hasData ? (String(extS1.lastTempC, 2) + " &deg;C") : String("--")) + "</b> | State: <span class='badge " + String(extS1.isOnline ? "bg-green'>ONLINE" : (extS1.isAssigned ? "bg-red'>OFFLINE" : "style='background:#555;'>NOT ASSIGNED")) + "</span></p>";
    html += "<p>Forecast Risk / Trend: <span class='badge " + badgeColorClass + "'>" + String(fc.statusText) + "</span> | Location: <b>" + String(fc.locationTag) + "</b></p>";
    html += "<div style='display:flex;gap:8px;margin-top:8px;'>";
    html += "<a href='/temp_calib' class='btn-sec'>🔍 Temperature Calibration</a>";
    html += "<a href='/weather' class='btn-sec'>🌦️ Weather Details</a>";
    html += "</div>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 2. TOUCH & FEEL SUBPAGE (/touch_feel)
// ============================================================================
static void handleTouchFeelPage() {
    bool rot = settingsGetRotate180();
    uint8_t curTheme = settingsGetUiTheme();
    uint8_t currentDisp = settingsGetDisplayPage();

    String html = getCommonHeader("Touch & Feel", "touch_feel");

    // Display & Theme Settings Card
    html += "<div class='card'>";
    html += "<h3>Display &amp; Theme Configuration</h3>";
    html += "<form action='/save_touch_feel' method='POST'>";

    html += "<label>Main Screen Color Theme (LVGL):</label>";
    html += "<select name='ui_theme'>";
    html += "<option value='0' " + String(curTheme == 0 ? "selected" : "") + ">Monochrome (Light on Dark)</option>";
    html += "<option value='1' " + String(curTheme == 1 ? "selected" : "") + ">Amber CRT (Vintage Chihlimbar)</option>";
    html += "<option value='2' " + String(curTheme == 2 ? "selected" : "") + ">Green Matrix (Vintage Fosfor Verde)</option>";
    html += "<option value='3' " + String(curTheme == 3 ? "selected" : "") + ">Cyan Electric (Cyberpunk High-Contrast)</option>";
    html += "<option value='4' " + String(curTheme == 4 ? "selected" : "") + ">Night Red (Rubin / LED Rosu)</option>";
    html += "<option value='5' " + String(curTheme == 5 ? "selected" : "") + ">Original EEZ (Solid Cyan LCD)</option>";
    html += "</select>";

    html += "<label style='margin-top:12px;'><input type='checkbox' name='rot180' value='1' " + (rot ? String("checked") : "") + "> Rotate display 180 degrees</label>";

    html += "<label style='margin-top:12px;'>Active Screen on LCD Hardware:</label>";
    html += "<select name='disp_page'>";
    html += "<option value='0' " + String(currentDisp == 0 ? "selected" : "") + ">0 - Main Clock Screen (BaroClock)</option>";
    html += "<option value='1' " + String(currentDisp == 1 ? "selected" : "") + ">1 - GNSS Diagnostic Screen (Satellites & SNR Chart)</option>";
    html += "</select>";

    html += "<input type='submit' value='Apply Display Settings'>";
    html += "</form>";
    html += "</div>";

    // LCD Graphics & Symbols Preview Card
    html += "<div class='card'>";
    html += "<h3>Display Graphics &amp; Symbols Test (LCD Preview)</h3>";
    html += "<p style='font-size:0.88em;color:#bbb;'>Click any item below to display on the physical screen for 20 seconds:</p>";

    html += "<p><b>Test Weather Icons:</b></p>";
    html += "<div class='grid-test'>";
    html += "<a href='/test_icon?icon=0' class='btn-sec'>☀️ Sun</a>";
    html += "<a href='/test_icon?icon=1' class='btn-sec'>🌤️ Sun+Cloud</a>";
    html += "<a href='/test_icon?icon=2' class='btn-sec'>☁️ Cloud</a>";
    html += "<a href='/test_icon?icon=3' class='btn-sec'>🌧️ Rain</a>";
    html += "<a href='/test_icon?icon=4' class='btn-sec'>⛈️ Storm</a>";
    html += "<a href='/test_icon?icon=5' class='btn-sec'>📍 New Loc</a>";
    html += "</div>";
    html += "<a href='/test_icon?icon=-1' class='btn-sec' style='background:#2a4436;border-color:#4caf50;'>🔄 Normal Live Forecast Icon</a>";

    html += "<p style='margin-top:12px;'><b>Test Pressure Trend Arrows:</b></p>";
    html += "<div class='grid-test'>";
    html += "<a href='/test_arrow?trend=1' class='btn-sec'>↗️ Rising (Up)</a>";
    html += "<a href='/test_arrow?trend=0' class='btn-sec'>➡️ Steady (Flat)</a>";
    html += "<a href='/test_arrow?trend=2' class='btn-sec'>↘️ Falling (Down)</a>";
    html += "</div>";
    html += "<a href='/test_arrow?trend=-1' class='btn-sec' style='background:#2a4436;border-color:#4caf50;'>🔄 Normal Live Trend Arrow</a>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 3. SOUND & ALARMS SUBPAGE (/sound_alarms)
// ============================================================================
static void handleSoundAlarmsPage() {
    bool chimeEn = settingsGetChimeEnabled();
    uint16_t chimeFq = settingsGetChimeFreq();
    uint16_t chimeDur = settingsGetChimeDuration();
    uint16_t chimePause = settingsGetChimePause();
    uint8_t chimeVol = settingsGetChimeVolume();

    bool almEn = settingsGetAlarmEnabled();
    uint8_t almHr = settingsGetAlarmHour();
    uint8_t almMin = settingsGetAlarmMinute();
    uint8_t almPer = settingsGetAlarmPeriodicity();
    uint16_t almFq = settingsGetAlarmFreq();
    uint8_t almVol = settingsGetAlarmVolume();

    bool wtrDetEn = settingsGetWtrDetAlertEnabled();
    uint16_t wtrDetFq = settingsGetWtrDetFreq();
    uint8_t wtrDetVol = settingsGetWtrDetVolume();

    bool stormEn = settingsGetStormAlertEnabled();
    uint16_t stormFq = settingsGetStormFreq();
    uint8_t stormVol = settingsGetStormVolume();

    bool fbEn = settingsGetFeedbackEnabled();
    uint16_t fbFq = settingsGetFeedbackFreq();
    uint16_t fbDur = settingsGetFeedbackDuration();
    uint8_t fbVol = settingsGetFeedbackVolume();

    String html = getCommonHeader("Sound & Alarms", "sound_alarms");

    // Audio Signals & Alarms Card
    html += "<div class='card'>";
    html += "<h3>Acoustic Signals &amp; Audio Alarms</h3>";
    html += "<form action='/save_alarms' method='POST'>";

    // Alarms Table
    html += "<div class='tbl-wrap'><table style='width:100%;min-width:480px;border-collapse:collapse;font-size:0.9em;'>";
    html += "<tr style='border-bottom:1px solid #555;'><th style='text-align:left;padding:6px;'>Alarm Type</th><th>Active</th><th>Parameters / Time</th><th>Freq.</th><th>Volume</th></tr>";

    // 1. Chime
    html += "<tr style='border-bottom:1px solid #444;'>";
    html += "<td style='padding:6px;'><b>Chime (Hourly)</b><br><small>Double Pip-Pip</small></td>";
    html += "<td style='text-align:center;'><input type='checkbox' name='chime_en' value='1' " + (chimeEn ? String("checked") : "") + "></td>";
    html += "<td>Tone: <input type='number' name='chime_dur' value='" + String(chimeDur) + "' min='20' max='500' style='width:60px;'> ms<br>"
            "Pause: <input type='number' name='chime_pause' value='" + String(chimePause) + "' min='20' max='500' style='width:60px;'> ms</td>";
    html += "<td><input type='number' name='chime_fq' value='" + String(chimeFq) + "' min='500' max='5000' style='width:70px;'> Hz</td>";
    html += "<td><input type='number' name='chime_vol' value='" + String(chimeVol) + "' min='10' max='100' style='width:54px;'> %</td>";
    html += "</tr>";

    // 2. Clock Alarm (24H Format)
    char almHrBuf[8], almMinBuf[8];
    snprintf(almHrBuf, sizeof(almHrBuf), "%02u", almHr);
    snprintf(almMinBuf, sizeof(almMinBuf), "%02u", almMin);
    html += "<tr style='border-bottom:1px solid #444;'>";
    html += "<td style='padding:6px;'><b>Alarm Clock (Wake-Up)</b><br><small>5 cyclic bursts (Local Time)</small></td>";
    html += "<td style='text-align:center;'><input type='checkbox' name='alm_en' value='1' " + (almEn ? String("checked") : "") + "></td>";
    html += "<td>";
    html += "<input type='number' name='alm_hr' min='0' max='23' value='" + String(almHrBuf) + "' style='width:54px;display:inline-block;font-weight:bold;'> : ";
    html += "<input type='number' name='alm_min' min='0' max='59' value='" + String(almMinBuf) + "' style='width:54px;display:inline-block;font-weight:bold;'> ";
    html += "<small style='color:#bbb;'>(24H: 00..23 : 00..59)</small><br>";
    html += "<select name='alm_per' style='width:105px;margin-top:4px;'>";
    html += "<option value='0' " + String(almPer == 0 ? "selected" : "") + ">Daily</option>";
    html += "<option value='1' " + String(almPer == 1 ? "selected" : "") + ">Mon-Fri</option>";
    html += "<option value='2' " + String(almPer == 2 ? "selected" : "") + ">Once</option>";
    html += "</select></td>";
    html += "<td><input type='number' name='alm_fq' value='" + String(almFq) + "' min='500' max='5000' style='width:70px;'> Hz</td>";
    html += "<td><input type='number' name='alm_vol' value='" + String(almVol) + "' min='10' max='100' style='width:54px;'> %</td>";
    html += "</tr>";

    // 3. Weather Deterioration
    html += "<tr style='border-bottom:1px solid #444;'>";
    html += "<td style='padding:6px;'><b>Weather Deterioration</b><br><small>3 warning beeps</small></td>";
    html += "<td style='text-align:center;'><input type='checkbox' name='wtr_det_en' value='1' " + (wtrDetEn ? String("checked") : "") + "></td>";
    html += "<td>Emitted once on risk</td>";
    html += "<td><input type='number' name='wtr_det_fq' value='" + String(wtrDetFq) + "' min='500' max='5000' style='width:70px;'> Hz</td>";
    html += "<td><input type='number' name='wtr_det_vol' value='" + String(wtrDetVol) + "' min='10' max='100' style='width:54px;'> %</td>";
    html += "</tr>";

    // 4. Storm Alert
    html += "<tr style='border-bottom:1px solid #444;'>";
    html += "<td style='padding:6px;'><b>Storm Siren Alert</b><br><small>Morse S.O.S (1 min / touch)</small></td>";
    html += "<td style='text-align:center;'><input type='checkbox' name='storm_en' value='1' " + (stormEn ? String("checked") : "") + "></td>";
    html += "<td>S.O.S Morse Siren</td>";
    html += "<td><input type='number' name='storm_fq' value='" + String(stormFq) + "' min='500' max='5000' style='width:70px;'> Hz</td>";
    html += "<td><input type='number' name='storm_vol' value='" + String(stormVol) + "' min='10' max='100' style='width:54px;'> %</td>";
    html += "</tr>";

    // 5. Touch Feedback Click
    html += "<tr>";
    html += "<td style='padding:6px;'><b>Touch Feedback (Click)</b><br><small>Screen tap / slide / GNSS 1s</small></td>";
    html += "<td style='text-align:center;'><input type='checkbox' name='fb_en' value='1' " + (fbEn ? String("checked") : "") + "></td>";
    html += "<td>Duration: <input type='number' name='fb_dur' value='" + String(fbDur) + "' min='20' max='1000' style='width:60px;'> ms</td>";
    html += "<td><input type='number' name='fb_fq' value='" + String(fbFq) + "' min='500' max='5000' style='width:70px;'> Hz</td>";
    html += "<td><input type='number' name='fb_vol' value='" + String(fbVol) + "' min='10' max='100' style='width:54px;'> %</td>";
    html += "</tr>";

    html += "</table></div>";
    html += "<input type='submit' value='Save Alarm Configuration' style='margin-top:12px;'>";
    html += "</form>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<h4>Acoustic Signal Verification (Sound Tests):</h4>";
    html += "<div class='grid-test'>";
    html += "<a href='/test_sound?type=beep' class='btn-sec'>🔔 Test Beep</a>";
    html += "<a href='/test_sound?type=click' class='btn-sec'>👆 Test Click</a>";
    html += "<a href='/test_sound?type=chime' class='btn-sec'>🕰️ Test Chime</a>";
    html += "<a href='/test_sound?type=alarm' class='btn-sec'>⏰ Test Alarm</a>";
    html += "<a href='/test_sound?type=wtr_det' class='btn-sec'>⚠️ Test Deterioration</a>";
    html += "<a href='/test_sound?type=storm' class='btn-sec'>🚨 Test Storm Siren</a>";
    html += "<a href='/test_sound?type=stop' class='btn-sec' style='background:#b71c1c;color:#fff;'>⏹️ Stop All Audio</a>";
    html += "</div>";
    html += "</div>";

    // Proximity Sensor Card
    bool soundTestEn = settingsGetSoundTestEnabled();
    uint16_t proxVal = proximityGetRawValue();
    bool proxDet = proximityIsDetected();
    html += "<div class='card'>";
    html += "<h3>Proximity Sensor (MK-TP2016)</h3>";
    html += "<p>Real-Time Sensor Value: <b style='font-size:1.2em;color:#00e5ff;'>" + String(proxVal) + "</b></p>";
    html += "<p>Detection Threshold: <b>" + String(PROXIMITY_THRESHOLD) + "</b></p>";
    html += "<p>Status: <span class='badge " + String(proxDet ? "bg-green'>OBJECT DETECTED" : "style='background:#555;color:#ccc;'>IDLE / NO OBJECT") + "</span></p>";
    html += "<form action='/save_sound_test' method='POST' style='margin-top:12px;'>";
    html += "<label><input type='checkbox' name='snd_test' value='1' " + (soundTestEn ? String("checked") : "") + "> Trigger automatic Beep on hand proximity</label>";
    html += "<input type='submit' value='Save Sensor Settings'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 4. WEATHER SUBPAGE (/weather)
// ============================================================================
static void handleWeatherPage() {
    const BmeData& b = bmeGetData();
    const WeatherForecastInfo& fc = weatherGetForecast();
    float qnhOff = settingsGetQnhOffset();

    String badgeColorClass = "bg-green";
    if (fc.risk == RISK_CAUTION) badgeColorClass = "style='background:#ffeb3b;color:#000;'";
    else if (fc.risk == RISK_DETERIORATION) badgeColorClass = "style='background:#ff9800;color:#000;'";
    else if (fc.risk == RISK_STORM) badgeColorClass = "bg-red";
    else if (fc.risk == RISK_NEW_LOCATION) badgeColorClass = "style='background:#00e5ff;color:#000;'";

    String html = getCommonHeader("Weather & Forecast", "weather");

    // Weather Forecast & Pressure History Card
    html += "<div class='card'>";
    html += "<h3>Weather Forecast &amp; Pressure History (LittleFS)</h3>";
    html += "<p>Current Risk / Trend: <span class='badge " + badgeColorClass + "'>" + String(fc.statusText) + "</span>";
    html += " | Analyzed Location: <b style='color:#00e5ff;'>" + String(fc.locationTag) + "</b></p>";
    html += "<p>Local Location Matching Samples: <b>" + String(fc.localSamplesCount) + "</b></p>";
    html += "<p>&Delta;P (30 min): <b>" + String(fc.deltaP_30m, 2) + " hPa</b> | &Delta;P (1 hour): <b>" + String(fc.deltaP_1h, 2) + " hPa</b> | &Delta;P (3 hours): <b>" + String(fc.deltaP_3h, 2) + " hPa</b></p>";
    html += "<p>Pressure Rate of Change (V_p): <b>" + String(fc.rateOfChange, 2) + " hPa/h</b> | Acceleration: <b>" + String(fc.acceleration, 2) + " hPa/h&sup2;</b></p>";
    html += "<p>Total Flash Records: <b>" + String(weatherGetTotalHistoryRecords()) + "</b> (~" + String((weatherGetTotalHistoryRecords() * sizeof(WeatherRecord)) / 1024) + " KB) | Pending RAM Buffer: <b>" + String(weatherGetRamBufferPending()) + " / 20</b></p>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<h4>Ground Truth Weather Calibration:</h4>";
    html += "<p style='font-size:0.88em;color:#bbb;'>Click current real outdoor weather to anchor prediction and train historic model:</p>";
    html += "<div class='grid-calib'>";
    html += "<a href='/calibrate_wtr?state=0' class='btn-calib'>☀️ Clear</a>";
    html += "<a href='/calibrate_wtr?state=1' class='btn-calib'>🌤️ Partly Cld</a>";
    html += "<a href='/calibrate_wtr?state=2' class='btn-calib'>☁️ Overcast</a>";
    html += "<a href='/calibrate_wtr?state=3' class='btn-calib'>🌫️ Fog/Mist</a>";
    html += "<a href='/calibrate_wtr?state=4' class='btn-calib'>🌦️ Drizzle</a>";
    html += "<a href='/calibrate_wtr?state=5' class='btn-calib'>🌧️ Rain</a>";
    html += "<a href='/calibrate_wtr?state=6' class='btn-calib'>⛈️ Storm</a>";
    html += "<a href='/calibrate_wtr?state=7' class='btn-calib'>🌨️ Snow</a>";
    html += "</div>";
    html += "</div>";

    // Sensors Card & Barometer Standard Calibration
    html += "<div class='card'>";
    html += "<h3>BMP280 Barometer Standard Calibration</h3>";
    html += "<p>Raw Pressure: <b>" + String(b.pressureRawMb, 1) + " mb</b> | Calibrated: <b style='font-size:1.1em;color:#4caf50;'>" + String(b.pressureCalibratedMb, 1) + " mb</b></p>";
    html += "<p>Barometer Calibration Offset (QFE): <b>" + String(qnhOff, 2) + " hPa</b></p>";

    html += "<form action='/save_qfe' method='POST'>";
    html += "<label>Standard Reference Barometer (QFE hPa):</label>";
    html += "<input type='number' step='0.1' name='qfe_val' placeholder='e.g. 1013.2' required>";
    html += "<input type='submit' value='Calibrate &amp; Save QFE'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 5. NETWORKS SUBPAGE (/networks)
// ============================================================================
static void handleNetworksPage() {
    String html = getCommonHeader("Networks & Sensors", "networks");

    // WiFi Configuration Card
    html += "<div class='card'>";
    html += "<h3>WiFi Network Configuration (Slot 1, 2, 3)</h3>";

    for (int slot = 1; slot <= 3; slot++) {
        String s, p;
        wifiMgrGetSlot(slot, s, p);
        html += "<div style='border-top:1px solid #444;margin-top:10px;padding-top:10px;'>";
        html += "<form action='/save_wifi' method='POST'>";
        html += "<input type='hidden' name='slot' value='" + String(slot) + "'>";
        html += "<label><b>Network " + String(slot) + " - SSID:</b></label>";
        html += "<input type='text' name='ssid' value='" + s + "' placeholder='Network SSID'>";
        html += "<label>Password:</label>";
        html += "<input type='password' name='pass' value='" + p + "' placeholder='Password'>";
        html += "<input type='submit' value='Save Network " + String(slot) + "'>";
        html += "</form>";
        html += "</div>";
    }

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<form action='/reset_wifi' method='POST' onsubmit='return confirm(\"Reset all WiFi settings and enter AP mode?\");'>";
    html += "<input type='submit' class='btn-reset' value='Clear All WiFi Settings & Reboot into AP'>";
    html += "</form>";
    html += "</div>";

    // External Temperature Sensors Card (UDP Broadcast Port 5005)
    html += "<div class='card'>";
    html += "<h3>External Temperature Sensors (UDP Broadcast Port " + String(EXT_SENSORS_UDP_PORT) + ")</h3>";

    uint8_t discCount = extSensorGetDiscoveredCount();
    html += "<p><b>Discovered Network Sensors:</b> " + String(discCount) + "</p>";
    if (discCount > 0) {
        html += "<div class='tbl-wrap'><table style='width:100%;min-width:440px;border-collapse:collapse;font-size:0.85em;margin-bottom:6px;'>";
        html += "<tr style='border-bottom:1px solid #555;'><th style='text-align:left;padding:4px;'>Device ID</th><th>IP</th><th>Temp</th><th>Last Seen</th><th>Status</th></tr>";
        for (int i = 0; i < discCount; i++) {
            ExtSensorDiscovered dev;
            extSensorGetDiscovered(i, dev);
            unsigned long secAgo = (millis() - dev.lastSeenMillis) / 1000UL;
            html += "<tr style='border-bottom:1px solid #444;text-align:center;'>";
            html += "<td style='text-align:left;padding:4px;'><b style='color:#ffeb3b;'>" + String(dev.id) + "</b></td>";
            html += "<td>" + String(dev.ip) + "</td>";
            html += "<td><b>" + (dev.hasData ? (String(dev.lastTempC, 1) + " &deg;C") : String("--")) + "</b></td>";
            html += "<td>" + String(secAgo) + "s ago</td>";
            html += "<td><span class='badge " + String(dev.isOnline ? "bg-green'>ONLINE" : "bg-red'>OFFLINE") + "</span></td>";
            html += "</tr>";
        }
        html += "</table></div>";
    } else {
        html += "<p style='color:#aaa;font-size:0.85em;'><i>No external sensors detected yet. Waiting for UDP broadcast...</i></p>";
    }

    ExtSensorSlot extS1;
    extSensorGetSlot(1, extS1);

    html += "<form action='/save_ext_sensor' method='POST' style='margin-top:10px;'>";
    html += "<p><b>Display Slot 1 (Assigned to Main LCD):</b></p>";
    html += "<label>Assigned Sensor ID (MAC):</label>";
    html += "<select name='slot1_id'>";
    html += "<option value=''" + String(!extS1.isAssigned ? " selected" : "") + ">-- None (Disabled) --</option>";
    for (int i = 0; i < discCount; i++) {
        ExtSensorDiscovered dev;
        extSensorGetDiscovered(i, dev);
        bool sel = (extS1.isAssigned && strcasecmp(extS1.id, dev.id) == 0);
        html += "<option value='" + String(dev.id) + "'" + (sel ? " selected" : "") + ">" + String(dev.id) + " (" + String(dev.ip) + ")</option>";
    }
    if (extS1.isAssigned) {
        bool foundInDisc = false;
        for (int i = 0; i < discCount; i++) {
            ExtSensorDiscovered dev;
            extSensorGetDiscovered(i, dev);
            if (strcasecmp(extS1.id, dev.id) == 0) { foundInDisc = true; break; }
        }
        if (!foundInDisc) {
            html += "<option value='" + String(extS1.id) + "' selected>" + String(extS1.id) + " (Saved: " + String(extS1.lastIp) + ")</option>";
        }
    }
    html += "</select>";

    html += "<label>Sensor Custom Alias (displayed on screen):</label>";
    html += "<input type='text' name='slot1_alias' maxlength='16' value='" + String(extS1.alias) + "' placeholder='e.g. TERASA or BALCON'>";
    html += "<p style='font-size:0.85em;color:#ccc;'>Raw Network Temp: <b>" + (extS1.hasData ? (String(extS1.rawTempC, 2) + " &deg;C") : String("--")) + "</b> | Calibrated: <b style='color:#00e5ff;'>" + (extS1.hasData ? (String(extS1.lastTempC, 2) + " &deg;C") : String("--")) + "</b> | State: <span class='badge " + String(extS1.isOnline ? "bg-green'>ONLINE" : (extS1.isAssigned ? "bg-red'>OFF" : "style='background:#555;'>DISABLED")) + "</span></p>";
    html += "<p style='margin-top:6px;font-size:0.9em;'>Multi-Point Calibration: <a href='/temp_calib?sensor=1' style='color:#00e5ff;font-weight:bold;'>Open Slot 1 11-Band Table &rarr;</a></p>";

    html += "<input type='submit' value='Save Sensor Configuration'>";
    html += "</form>";

    if (extS1.isAssigned && extS1.hasData) {
        html += "<form action='/calibrate_ext_sensor' method='POST' style='margin-top:10px;border-top:1px solid #444;padding-top:10px;'>";
        html += "<input type='hidden' name='slot' value='1'>";
        html += "<label>Standard Reference Thermometer (&deg;C) for Slot 1:</label>";
        html += "<input type='number' step='0.1' name='ref_temp' placeholder='e.g. 21.5' value='" + String(extS1.lastTempC, 1) + "' required>";
        html += "<input type='submit' class='btn-sec' style='margin-top:8px;' value='Calibrate Slot 1 (& Save Offset)'>";
        html += "</form>";
    }
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 6. TEMPERATURE CALIBRATION SUBPAGE (/temp_calib)
// ============================================================================
static void handleTempCalibPage() {
    uint8_t sensorIdx = 0;
    if (server.hasArg("sensor")) {
        sensorIdx = server.arg("sensor").toInt();
        if (sensorIdx > 1) sensorIdx = 0;
    }

    String html = getCommonHeader("Temperature Calibration", "temp_calib");

    // Internal Sub-tabs for Sensors
    html += "<div class='nav-bar' style='margin-bottom:12px;'>";
    html += "<a href='/temp_calib?sensor=0' class='nav-btn" + String(sensorIdx == 0 ? " active" : "") + "'>Internal BME280</a>";
    html += "<a href='/temp_calib?sensor=1' class='nav-btn" + String(sensorIdx == 1 ? " active" : "") + "'>External 1 (DS18B20)</a>";
    html += "</div>";

    float offsets[TEMP_CALIB_BANDS_COUNT];
    if (sensorIdx == 0) {
        tempCalibGetBmeOffsets(offsets);
    } else {
        tempCalibGetExt1Offsets(offsets);
    }
    const char *sensorName = (sensorIdx == 0) ? "Internal Sensor (BME280)" : "External Sensor 1 (DS18B20)";

    float liveRaw = 0.0f;
    float liveCal = 0.0f;
    bool hasData = false;

    if (sensorIdx == 0) {
        const BmeData &b = bmeGetData();
        liveRaw = b.tempRawC;
        liveCal = b.tempC;
        hasData = true;
    } else {
        ExtSensorSlot extS1;
        extSensorGetSlot(1, extS1);
        liveRaw = extS1.rawTempC;
        liveCal = extS1.lastTempC;
        hasData = extS1.hasData;
    }
    float liveDelta = liveCal - liveRaw;

    html += "<div class='card'>";
    html += "<h3>" + String(sensorName) + " - Live Status</h3>";
    if (hasData) {
        html += "<p>Raw Temperature: <b>" + String(liveRaw, 2) + " &deg;C</b></p>";
        html += "<p>Calibrated Temperature: <b style='color:#4caf50;font-size:1.2em;'>" + String(liveCal, 2) + " &deg;C</b></p>";
        html += "<p>Current Interpolated Offset: <b style='color:#00e5ff;font-size:1.15em;'>" + String(liveDelta > 0 ? "+" : "") + String(liveDelta, 2) + " &deg;C</b></p>";
    } else {
        html += "<p style='color:#aaa;'><i>No live sensor reading available.</i></p>";
    }
    html += "</div>";

    // Quick Calibration Box
    html += "<div class='card'>";
    html += "<h3>Quick Calibrate from Standard Reference</h3>";
    html += "<p style='font-size:0.88em;color:#bbb;'>Enter reference thermometer reading at current temperature to calculate and insert offset into the active band:</p>";
    html += "<form action='/save_temp_cal_quick' method='POST'>";
    html += "<input type='hidden' name='sensor' value='" + String(sensorIdx) + "'>";
    html += "<label>Standard Reference Thermometer (&deg;C):</label>";
    html += "<input type='number' step='0.05' name='ref_val' placeholder='e.g. 22.30' value='" + (hasData ? String(liveCal, 2) : "20.00") + "' required>";
    html += "<input type='submit' class='btn' value='Calculate &amp; Apply Offset'>";
    html += "</form>";
    html += "</div>";

    // 11-Band Multi-Point Table
    html += "<div class='card'>";
    html += "<h3>11-Point Calibration Table</h3>";
    html += "<p style='font-size:0.85em;color:#aaa;'>Piecewise continuous linear interpolation across ranges. Set 0.00 to remove offset.</p>";

    html += "<form action='/save_temp_calib' method='POST'>";
    html += "<input type='hidden' name='sensor' value='" + String(sensorIdx) + "'>";
    html += "<div class='tbl-wrap'>";
    html += "<table>";
    html += "<tr><th>Band Range</th><th>Standard (&deg;C)</th><th>Offset (&Delta;T &deg;C)</th></tr>";

    for (int i = 0; i < TEMP_CALIB_BANDS_COUNT; i++) {
        const TempBandInfo &band = TEMP_BANDS[i];
        float off = offsets[i];
        float refT = band.nodeT + off;

        bool isCurrentBand = (hasData && liveRaw >= band.minT && liveRaw < band.maxT);
        String rowStyle = isCurrentBand ? " style='background:#1b393b;font-weight:bold;'" : "";

        html += "<tr" + rowStyle + ">";
        html += "<td>" + String(band.label) + (isCurrentBand ? " <span class='badge bg-green'>CURRENT</span>" : "") + "</td>";
        html += "<td>" + String(refT, 2) + "</td>";
        html += "<td>";
        html += "<input type='number' step='0.05' name='d_" + String(i) + "' value='" + String(off, 2) + "' style='width:90px;'>";
        html += "</td>";
        html += "</tr>";
    }

    html += "</table>";
    html += "</div>";
    html += "<input type='submit' value='Save All 11 Calibration Offsets'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// 7. DEVICE SETTINGS SUBPAGE (/settings)
// ============================================================================
static void handleSettingsPage() {
    int32_t tzMin = settingsGetTimezoneMin();
    bool dst = settingsGetDst();
    bool tempF = settingsGetTempUnitF();
    uint8_t pressUnit = settingsGetPressUnit();
    bool highSens = settingsGetHighSens();
    uint8_t curSpan = baroGraphGetTimeSpan();
    uint32_t fcMin = settingsGetForecastIntervalMin();
    String savedManLoc = settingsGetManualLocator();

    uint8_t gnssMode = settingsGetGnssMode();
    bool nmeaEss = settingsGetNmeaEssential();
    bool nmeaDbg = settingsGetNmeaSerialDebug();

    String html = getCommonHeader("Device Settings", "settings");

    // Device Settings Form Card
    html += "<div class='card'>";
    html += "<h3>Clock, Units &amp; Sampling Settings</h3>";
    html += "<form action='/save_device_settings' method='POST'>";

    html += "<h4>Clock &amp; Timezone:</h4>";
    html += "<label>UTC to Local Time Difference (minutes):</label>";
    html += "<input type='number' name='tz_min' value='" + String(tzMin) + "' required>";
    html += "<label style='margin-top:10px;'><input type='checkbox' name='dst' value='1' " + (dst ? String("checked") : "") + "> Daylight Saving Time (DST +1h)</label>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<h4>Measurement Units:</h4>";
    html += "<label>Temperature Display Unit:</label>";
    html += "<select name='temp_f'>";
    html += "<option value='0' " + String(!tempF ? "selected" : "") + ">Celsius (&deg;C)</option>";
    html += "<option value='1' " + String(tempF ? "selected" : "") + ">Fahrenheit (&deg;F)</option>";
    html += "</select>";

    html += "<label style='margin-top:10px;'>Pressure Display Unit:</label>";
    html += "<select name='p_unit'>";
    html += "<option value='0' " + String(pressUnit == 0 ? "selected" : "") + ">Millibars (mb)</option>";
    html += "<option value='1' " + String(pressUnit == 1 ? "selected" : "") + ">Hectopascals (hPa)</option>";
    html += "<option value='2' " + String(pressUnit == 2 ? "selected" : "") + ">Millimeters of Mercury (mmHg)</option>";
    html += "</select>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<h4>Barograph &amp; Sampling:</h4>";
    html += "<label>Barograph Graph Sensitivity:</label>";
    html += "<select name='high_sens'>";
    html += "<option value='1' " + String(highSens ? "selected" : "") + ">HIGH (units & tenths)</option>";
    html += "<option value='0' " + String(!highSens ? "selected" : "") + ">LOW (tens & units)</option>";
    html += "</select>";

    html += "<label style='margin-top:10px;'>Barograph Time Window (Swipe on screen or select):</label>";
    html += "<select name='baro_win'>";
    html += "<option value='0' " + String(curSpan == 0 ? "selected" : "") + ">10 Minutes</option>";
    html += "<option value='1' " + String(curSpan == 1 ? "selected" : "") + ">30 Minutes</option>";
    html += "<option value='2' " + String(curSpan == 2 ? "selected" : "") + ">1 Hour</option>";
    html += "<option value='3' " + String(curSpan == 3 ? "selected" : "") + ">3 Hours</option>";
    html += "<option value='4' " + String(curSpan == 4 ? "selected" : "") + ">6 Hours</option>";
    html += "<option value='5' " + String(curSpan == 5 ? "selected" : "") + ">12 Hours</option>";
    html += "<option value='6' " + String(curSpan == 6 ? "selected" : "") + ">24 Hours (1 Day)</option>";
    html += "<option value='7' " + String(curSpan == 7 ? "selected" : "") + ">3 Days</option>";
    html += "<option value='8' " + String(curSpan == 8 ? "selected" : "") + ">7 Days (1 Week)</option>";
    html += "<option value='9' " + String(curSpan == 9 ? "selected" : "") + ">14 Days (2 Weeks)</option>";
    html += "</select>";

    html += "<label style='margin-top:10px;'>Forecast Sampling Interval (minutes: 1 .. 30min):</label>";
    html += "<input type='number' name='fc_min' value='" + String(fcMin) + "' min='1' max='30'>";

    html += "<label style='margin-top:10px;'>Maidenhead Locator (Manual / Fallback):</label>";
    html += "<input type='text' name='man_loc' maxlength='6' value='" + savedManLoc + "' placeholder='e.g. KN34bk (overwritten by GPS fix)'>";

    html += "<input type='submit' value='Save Device Settings'>";
    html += "</form>";
    html += "</div>";

    // GNSS Control Card
    html += "<div class='card'>";
    html += "<h3>GNSS Receiver Configuration (Ai-Thinker GP-02 / AT6558)</h3>";
    html += "<form action='/save_gnss_cfg' method='POST' style='margin-bottom:15px;'>";
    html += "<label>Active Satellite Constellation:</label>";
    html += "<select name='gnss_mode'>";
    html += "<option value='1' " + String(gnssMode == 1 ? "selected" : "") + ">GPS Only ($PCAS04,1)</option>";
    html += "<option value='2' " + String(gnssMode == 2 ? "selected" : "") + ">BeiDou / BDS Only ($PCAS04,2)</option>";
    html += "<option value='3' " + String(gnssMode == 3 ? "selected" : "") + ">GPS + BeiDou (ALL) ($PCAS04,3)</option>";
    html += "</select>";

    html += "<label style='margin-top:10px;'>NMEA Sentence Filter:</label>";
    html += "<select name='nmea_ess'>";
    html += "<option value='1' " + String(nmeaEss ? "selected" : "") + ">Essential Only: GGA, GSA, GSV, RMC ($PCAS03)</option>";
    html += "<option value='0' " + String(!nmeaEss ? "selected" : "") + ">All Sentences: +GLL, VTG, ZDA, TXT ($PCAS03)</option>";
    html += "</select>";

    html += "<label style='margin-top:12px;'><input type='checkbox' name='nmea_dbg' value='1' " + String(nmeaDbg ? "checked" : "") + "> Stream NMEA to Serial Monitor (Live Debug)</label>";

    html += "<input type='submit' value='Apply GNSS Configuration'>";
    html += "</form>";

    html += "<hr style='border:0;border-top:1px solid #444;margin:15px 0;'>";
    html += "<form action='/save_gnss_flash' method='POST' onsubmit='return confirm(\"Confirm permanent save to GP-02 non-volatile Flash?\");'>";
    html += "<input type='submit' class='btn-reset' style='background:#ff9800;' value='Save Settings to GP-02 Flash ($PCAS00)'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ============================================================================
// POST ACTION HANDLERS & REDIRECTS
// ============================================================================
static void handleSaveTouchFeel() {
    if (server.hasArg("ui_theme")) {
        uint8_t thm = server.arg("ui_theme").toInt();
        settingsSetUiTheme(thm);
        guiApplyTheme(thm);
    }
    bool rot = server.hasArg("rot180");
    settingsSetRotate180(rot);
    lcdSetRotation(rot);

    if (server.hasArg("disp_page")) {
        uint8_t page = server.arg("disp_page").toInt();
        settingsSetDisplayPage(page);
    }

    guiInvalidate();
    server.sendHeader("Location", "/touch_feel");
    server.send(303);
}

static void handleSaveDisplayPage() {
    if (server.hasArg("disp_page")) {
        uint8_t page = server.arg("disp_page").toInt();
        settingsSetDisplayPage(page);
    }
    server.sendHeader("Location", "/touch_feel");
    server.send(303);
}

static void handleTestWeatherIcon() {
    if (server.hasArg("icon")) {
        int8_t icon = server.arg("icon").toInt();
        guiSetTestWeatherIcon(icon);
    }
    server.sendHeader("Location", "/touch_feel");
    server.send(303);
}

static void handleTestTrendArrow() {
    if (server.hasArg("trend")) {
        int8_t trend = server.arg("trend").toInt();
        guiSetTestTrendArrow(trend);
    }
    server.sendHeader("Location", "/touch_feel");
    server.send(303);
}

static void handleSaveAlarms() {
    bool chimeEn = server.hasArg("chime_en");
    settingsSetChimeEnabled(chimeEn);
    if (server.hasArg("chime_dur")) settingsSetChimeDuration(server.arg("chime_dur").toInt());
    if (server.hasArg("chime_pause")) settingsSetChimePause(server.arg("chime_pause").toInt());
    if (server.hasArg("chime_fq")) settingsSetChimeFreq(server.arg("chime_fq").toInt());
    if (server.hasArg("chime_vol")) settingsSetChimeVolume(server.arg("chime_vol").toInt());

    bool almEn = server.hasArg("alm_en");
    settingsSetAlarmEnabled(almEn);

    if (server.hasArg("alm_hr") && server.hasArg("alm_min")) {
        uint8_t h = server.arg("alm_hr").toInt();
        uint8_t m = server.arg("alm_min").toInt();
        if (h > 23) h = 23;
        if (m > 59) m = 59;
        settingsSetAlarmHour(h);
        settingsSetAlarmMinute(m);
    }
    if (server.hasArg("alm_per")) settingsSetAlarmPeriodicity(server.arg("alm_per").toInt());
    if (server.hasArg("alm_fq")) settingsSetAlarmFreq(server.arg("alm_fq").toInt());
    if (server.hasArg("alm_vol")) settingsSetAlarmVolume(server.arg("alm_vol").toInt());

    bool wtrDetEn = server.hasArg("wtr_det_en");
    settingsSetWtrDetAlertEnabled(wtrDetEn);
    if (server.hasArg("wtr_det_fq")) settingsSetWtrDetFreq(server.arg("wtr_det_fq").toInt());
    if (server.hasArg("wtr_det_vol")) settingsSetWtrDetVolume(server.arg("wtr_det_vol").toInt());

    bool stormEn = server.hasArg("storm_en");
    settingsSetStormAlertEnabled(stormEn);
    if (server.hasArg("storm_fq")) settingsSetStormFreq(server.arg("storm_fq").toInt());
    if (server.hasArg("storm_vol")) settingsSetStormVolume(server.arg("storm_vol").toInt());

    bool fbEn = server.hasArg("fb_en");
    settingsSetFeedbackEnabled(fbEn);
    if (server.hasArg("fb_dur")) settingsSetFeedbackDuration(server.arg("fb_dur").toInt());
    if (server.hasArg("fb_fq")) settingsSetFeedbackFreq(server.arg("fb_fq").toInt());
    if (server.hasArg("fb_vol")) settingsSetFeedbackVolume(server.arg("fb_vol").toInt());

    server.sendHeader("Location", "/sound_alarms");
    server.send(303);
}

static void handleTestSound() {
    String type = server.arg("type");
    if (type == "beep") {
        soundPlayBeep(1200, 150);
    } else if (type == "click") {
        soundPlayFeedbackClick();
    } else if (type == "chime") {
        soundPlayChime();
    } else if (type == "alarm") {
        soundStartAlarmClock();
    } else if (type == "wtr_det") {
        soundPlayWeatherDeterioration();
    } else if (type == "storm") {
        soundStartStormAlert();
    } else if (type == "stop") {
        soundStopAllAlarms();
    }
    server.sendHeader("Location", "/sound_alarms");
    server.send(303);
}

static void handleSaveSoundTest() {
    bool en = server.hasArg("snd_test");
    settingsSetSoundTestEnabled(en);
    server.sendHeader("Location", "/sound_alarms");
    server.send(303);
}

static void handleCalibrateWeather() {
    if (server.hasArg("state")) {
        uint8_t state = server.arg("state").toInt();
        weatherLoggerCalibrate(state);
    }
    server.sendHeader("Location", "/weather");
    server.send(303);
}

static void handleSaveQfe() {
    if (server.hasArg("qfe_val")) {
        float standardMb = server.arg("qfe_val").toFloat();
        const BmeData &b = bmeGetData();
        float newOffset = standardMb - b.pressureRawMb;
        settingsSetQnhOffset(newOffset);
        bmeSensorUpdate();
        baroGraphResetBaseline();
    }
    server.sendHeader("Location", "/weather");
    server.send(303);
}

static void handleSaveWifi() {
    if (server.hasArg("slot") && server.hasArg("ssid") && server.hasArg("pass")) {
        int slot = server.arg("slot").toInt();
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        wifiMgrSaveSlot(slot, ssid, pass);
    }
    server.sendHeader("Location", "/networks");
    server.send(303);
}

static void handleResetWifi() {
    wifiMgrResetAll();
    server.send(200, "text/html", "<html><body style='background:#181818;color:#eee;font-family:sans-serif;padding:20px;'><h2>WiFi Reset Completed</h2><p>Rebooting in AP mode...</p></body></html>");
    delay(1000);
    ESP.restart();
}

static void handleSaveExtSensor() {
    if (server.hasArg("slot1_id")) {
        String id = server.arg("slot1_id");
        String alias = server.arg("slot1_alias");
        id.trim();
        alias.trim();
        if (id.length() > 0) {
            extSensorSetSlot(1, id.c_str(), alias.c_str());
        } else {
            extSensorClearSlot(1);
        }
    }
    guiInvalidate();
    server.sendHeader("Location", "/networks");
    server.send(303);
}

static void handleCalibrateExtSensor() {
    if (server.hasArg("slot") && server.hasArg("ref_temp")) {
        int slot = server.arg("slot").toInt();
        float refT = server.arg("ref_temp").toFloat();
        extSensorCalibrate(slot, refT);
        guiInvalidate();
    }
    server.sendHeader("Location", "/networks");
    server.send(303);
}

static void handleSaveTempCalQuick() {
    uint8_t sensorIdx = 0;
    if (server.hasArg("sensor")) sensorIdx = server.arg("sensor").toInt();

    if (server.hasArg("ref_val")) {
        float refT = server.arg("ref_val").toFloat();
        float rawT = 0.0f;
        if (sensorIdx == 0) {
            const BmeData &b = bmeGetData();
            rawT = b.tempRawC;
        } else {
            ExtSensorSlot extS1;
            extSensorGetSlot(1, extS1);
            rawT = extS1.rawTempC;
        }
        float neededOffset = refT - rawT;

        float curOffsets[TEMP_CALIB_BANDS_COUNT];
        if (sensorIdx == 0) {
            tempCalibGetBmeOffsets(curOffsets);
        } else {
            tempCalibGetExt1Offsets(curOffsets);
        }

        for (int i = 0; i < TEMP_CALIB_BANDS_COUNT; i++) {
            if (rawT >= TEMP_BANDS[i].minT && rawT < TEMP_BANDS[i].maxT) {
                curOffsets[i] = neededOffset;
                break;
            }
        }

        if (sensorIdx == 0) {
            tempCalibSetBmeOffsets(curOffsets);
        } else {
            tempCalibSetExt1Offsets(curOffsets);
        }
    }
    server.sendHeader("Location", "/temp_calib?sensor=" + String(sensorIdx));
    server.send(303);
}

static void handleSaveTempCalib() {
    uint8_t sensorIdx = 0;
    if (server.hasArg("sensor")) sensorIdx = server.arg("sensor").toInt();

    float newOffsets[TEMP_CALIB_BANDS_COUNT];
    for (int i = 0; i < TEMP_CALIB_BANDS_COUNT; i++) {
        String argName = "d_" + String(i);
        if (server.hasArg(argName)) {
            newOffsets[i] = server.arg(argName).toFloat();
        } else {
            newOffsets[i] = 0.0f;
        }
    }

    if (sensorIdx == 0) {
        tempCalibSetBmeOffsets(newOffsets);
    } else {
        tempCalibSetExt1Offsets(newOffsets);
    }

    server.sendHeader("Location", "/temp_calib?sensor=" + String(sensorIdx));
    server.send(303);
}

static void handleSaveDeviceSettings() {
    if (server.hasArg("tz_min")) settingsSetTimezoneMin(server.arg("tz_min").toInt());
    settingsSetDst(server.hasArg("dst"));
    if (server.hasArg("temp_f")) settingsSetTempUnitF(server.arg("temp_f") == "1");
    if (server.hasArg("p_unit")) settingsSetPressUnit(server.arg("p_unit").toInt());
    if (server.hasArg("high_sens")) settingsSetHighSens(server.arg("high_sens") == "1");
    if (server.hasArg("baro_win")) {
        baroGraphSetTimeSpan(server.arg("baro_win").toInt());
        guiInvalidate();
    }
    if (server.hasArg("fc_min")) settingsSetForecastIntervalMin(server.arg("fc_min").toInt());
    if (server.hasArg("man_loc")) {
        String loc = server.arg("man_loc");
        loc.trim();
        if (loc.length() >= 4 && loc.length() <= 6) {
            settingsSetManualLocator(loc);
            gpsSetManualLocator(loc.c_str());
        }
    }
    baroGraphRedrawBox();
    server.sendHeader("Location", "/settings");
    server.send(303);
}

static void handleSaveGnssCfg() {
    if (server.hasArg("gnss_mode")) {
        uint8_t m = server.arg("gnss_mode").toInt();
        settingsSetGnssMode(m);
        gpsSetConstellationMode(m);
    }
    if (server.hasArg("nmea_ess")) {
        bool ess = (server.arg("nmea_ess") == "1");
        settingsSetNmeaEssential(ess);
        gpsSetNmeaFilter(ess);
    }
    bool dbg = server.hasArg("nmea_dbg");
    settingsSetNmeaSerialDebug(dbg);

    server.sendHeader("Location", "/settings");
    server.send(303);
}

static void handleSaveGnssFlash() {
    gpsSaveToFlash();
    server.sendHeader("Location", "/settings");
    server.send(303);
}

static void handleSyncClockSlew() {
    clockTriggerSync(false);
    server.sendHeader("Location", "/");
    server.send(303);
}

static void handleSyncClockStep() {
    clockTriggerSync(true);
    server.sendHeader("Location", "/");
    server.send(303);
}

// ============================================================================
// SERVER INITIALIZATION & ROUTING
// ============================================================================
void webpageInit() {
    // 1. Pages
    server.on("/", HTTP_GET, handleRoot);
    server.on("/touch_feel", HTTP_GET, handleTouchFeelPage);
    server.on("/sound_alarms", HTTP_GET, handleSoundAlarmsPage);
    server.on("/weather", HTTP_GET, handleWeatherPage);
    server.on("/networks", HTTP_GET, handleNetworksPage);
    server.on("/temp_calib", HTTP_GET, handleTempCalibPage);
    server.on("/settings", HTTP_GET, handleSettingsPage);

    // 2. Action endpoints
    server.on("/save_touch_feel", HTTP_POST, handleSaveTouchFeel);
    server.on("/save_display_page", HTTP_POST, handleSaveDisplayPage);
    server.on("/test_icon", HTTP_GET, handleTestWeatherIcon);
    server.on("/test_arrow", HTTP_GET, handleTestTrendArrow);

    server.on("/save_alarms", HTTP_POST, handleSaveAlarms);
    server.on("/test_sound", HTTP_GET, handleTestSound);
    server.on("/save_sound_test", HTTP_POST, handleSaveSoundTest);

    server.on("/calibrate_wtr", HTTP_GET, handleCalibrateWeather);
    server.on("/save_qfe", HTTP_POST, handleSaveQfe);
    server.on("/save_qnh", HTTP_POST, handleSaveQfe);

    server.on("/save_wifi", HTTP_POST, handleSaveWifi);
    server.on("/reset_wifi", HTTP_POST, handleResetWifi);
    server.on("/save_ext_sensor", HTTP_POST, handleSaveExtSensor);
    server.on("/calibrate_ext_sensor", HTTP_POST, handleCalibrateExtSensor);

    server.on("/save_temp_calib", HTTP_POST, handleSaveTempCalib);
    server.on("/save_temp_cal_quick", HTTP_POST, handleSaveTempCalQuick);

    server.on("/save_device_settings", HTTP_POST, handleSaveDeviceSettings);
    server.on("/save_settings", HTTP_POST, handleSaveDeviceSettings); // Backward compatibility
    server.on("/save_gnss_cfg", HTTP_POST, handleSaveGnssCfg);
    server.on("/save_gnss_flash", HTTP_POST, handleSaveGnssFlash);

    server.on("/sync_clock_slew", HTTP_GET, handleSyncClockSlew);
    server.on("/sync_clock_step", HTTP_GET, handleSyncClockStep);

    server.begin();
#if DEBUG_PONSTEST
    Serial.println("[WEB] Web server started on port 80 (7-Tab Subpage Architecture).");
#endif
}

void webpageHandleClient() {
    server.handleClient();
}
