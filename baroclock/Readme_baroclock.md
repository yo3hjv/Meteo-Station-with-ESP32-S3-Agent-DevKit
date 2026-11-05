# BaroClock — ESP32 Agent DevKit
**Dezvoltat de Adrian Florescu (YO3HJV)**  
*Platformă: ESP32-S3 Dev Module (16 MB Flash, OPI PSRAM activ, QIO 80 MHz)*  
*Display: LCD TFT 4.0" 480x320 ST7796 (Interfață paralelă 8080 8-bit, I2C AW9523 IO expander)*  
*Senzor Baro/Temp: Bosch BME280 / BMP280 pe I2C intern (autodetecție Chip ID)*  
*Senzor Poziție/Timp: Modul GPS NMEA pe UART (GPIO 20 Rx, autodetecție baud 9600..115200)*  

---

## 1. Descrierea Proiectului

**BaroClock** este un instrument combinat de precizie pentru radioamatori, meteorologie montană/outdoor și monitorizare barometrică de stație. Integrează:
1. **Ceas Dual (Local Time & UTC Time)** sincronizat prin GPS cu fallback pe NTP (WiFi).
2. **QTH Locator Maidenhead** de 6 caractere calculat din coordonatele GPS sau introdus manual.
3. **Barograf Rolling Strip Chart** cu axă statică de timp și derulare orizontală fără flicker.
4. **Modul de Prognoză Meteo Sintetică** cu istoric persistent în LittleFS, corelat pe poziție geografică.
5. **Portal Web Wi-Fi (AP & STA)** pentru monitorizare live, calibrare QFE (la sol), calibrare temperatură, setare unități de măsură și configurare rețea.
6. **Sistem Audio (Difuzor) & Senzor Proximitate**: Test sonor la apropierea mâinii (bip 1200 Hz / 300 ms) generat prin codec I2S ES8311, amplificator PA și senzor optic MK-TP2016.

---

## 2. Cum Funcționează Prognoza Barică (Cele 3 Elemente)

Pe ecranul LCD, în zona centrală ($Y = 162..248$), prognoza meteo este redată prin trei elemente complementare: **Iconița**, **Săgeata de Tendință** și **Textul de Risc/Stare**.

### A. Săgeata (Tendința Barică — Direcția de evoluție pe termen scurt)
Arată **încotro se mișcă presiunea acum** pe baza ratei de variație pe oră ($hPa/h$) și a diferenței pe 3 ore ($\Delta P_{3h}$):
* **Săgeată Verde în Sus ($\uparrow$)**: Presiune în creștere ($\ge +0.25\text{ hPa/h}$ sau $\Delta P_{3h} \ge +0.5\text{ hPa}$).
* **Săgeată Galbenă Orizontală ($\rightarrow$)**: Presiune stabilă (variație între $-0.25$ și $+0.25\text{ hPa/h}$).
* **Săgeată Roșie în Jos ($\downarrow$)**: Presiune în scădere ($\le -0.25\text{ hPa/h}$ sau $\Delta P_{3h} \le -0.5\text{ hPa}$).

### B. Textul (Nivelul de Risc Baric / Stabilitatea Atmosferei)
Evaluează **amplitudinea și violența scăderii de presiune** pentru a semnala potențialul de furtună sau vânt puternic:
* `[STABIL]` *(Verde)*: Atmosferă calmă. Variațiile sunt lente și nu indică fronturi atmosferice violente (scădere mai mică de $1.0\text{ hPa}$ pe 3 ore).
* `[ATENTIE]` *(Galben)*: Scădere moderată ($\Delta P_{3h} < -1.0\text{ hPa}$ sau $\Delta P_{1h} < -0.6\text{ hPa}$). Posibilă înnorare sau schimbare a vremii.
* `[DETERIORARE METEO]` *(Portocaliu)*: Scădere pronunțată ($\Delta P_{3h} < -2.0\text{ hPa}$ sau $\Delta P_{1h} < -1.2\text{ hPa}$). Probabilitate ridicată de precipitații și vânt.
* `[ALERTA FURTUNA]` *(Roșu intens)*: Cădere abruptă de presiune ($\Delta P_{3h} < -3.5\text{ hPa}$, $\Delta P_{1h} < -2.0\text{ hPa}$ sau cădere rapidă în 30 min cu accelerație negativă). Risc iminent de furtună / vijelie.
* `[LOCATIE NOUA]` *(Bleu cyan)*: Dispozitivul a fost mutat într-un nou QTH Locator și adună primele eșantioane (sub 4 măsurători locale / 12 min).

> [!NOTE]  
> **De ce poate apărea Săgeată Roșie în Jos ($\downarrow$) cu textul `[STABIL]`?**  
> Presiunea poate să scadă cu o pantă blândă și constantă (de exemplu $-0.3\text{ hPa}$ în 3 ore).  
> * Săgeata indică pur direcțional: **scade** ($\downarrow$).  
> * Textul evaluează riscul meteorologic: scăderea fiind foarte lentă, riscul de furtună/deteriorare este inexistent, așadar starea rămâne clasificată **`[STABIL]`**.

### C. Iconița Grafică Mare (Condiția Atmosferică Sintezată)
Caseta din stânga ($76 \times 86$ px) desenează starea rezultată prin combinarea tendinței cu nivelul de risc:
1. **Soare cu 8 raze**: Nivel `[STABIL]` cu tendință în creștere ($\uparrow$) — vreme frumoasă și în înseninare.
2. **Soare cu nor**: Nivel `[STABIL]` cu tendință staționară sau scădere ușoară — cer parțial acoperit sau stabil.
3. **Nor dens**: Nivel `[ATENTIE]` — cer înnorat, plafon coborât.
4. **Ploaie**: Nivel `[DETERIORARE METEO]` — precipitații probabile / active.
5. **Furtună cu fulger**: Nivel `[ALERTA FURTUNA]` — front atmosferic rece / descărcări electrice / vânt intens.
6. **Țintă GPS radar**: Nivel `[LOCATIE NOUA]` — calibrare automată pe noul perimetru geografic.

---

## 3. Structura Software și Module

* **`baroclock.ino`**: Modulul principal. Inițializare hardware, buclă de evenimente, desenare UI static și refresh diferențial optimizat.
* **`setup.h`**: Pini GPIO, configurații I2C/UART, paletă de culori RGB565 și timere globale.
* **`lcd_driver.h / .cpp`**: Driver grafic paralel 8080 pe 8 biți pentru controlerul ST7796, cu optimizare GPIO directă și fonturi scalabile.
* **`bme_sensor.h / .cpp`**: Citire și compensare BME280/BMP280 Bosch, calibrare QNH, calibrare temperatură și motorul graficului Barograph (Rolling Strip Chart).
* **`gps_handler.h / .cpp`**: Parser NMEA (TinyGPSPlus), autodetecție viteză UART, calcul Maidenhead Locator și numărare sateliți activi.
* **`weather_logger.h / .cpp`**: Bază de date istorică în LittleFS, corelare a presiunii pe locație geografică, analiză barică multiscara și randare iconițe vectoriale.
* **`sound.h / .cpp`**: Driver audio pentru difuzor: inițializare codec ES8311 (I2C 0x18), interfață I2S pe ESP32-S3, control amplificator PA (AW9523 P1_5) și sinteză bip sinusoidal (1200 Hz / 300 ms) în task separat FreeRTOS.
* **`proximity.h / .cpp`**: Driver independent pentru senzorul optic de proximitate MK-TP2016 (I2C 0x39): achiziție reflexie IR pe 16 biți, filtrare prag și declanșare sunet test.
* **`wifi_mgr.h / .cpp`**: Manager WiFi cu comutare automată STA/AP, stocare NVS (Preferences) și resetare la boot prin GPIO 46.
* **`webpage.h / .cpp`**: Server web HTTP asincron: interfață de configurare parametri, diagnoză senzori și calibrare etalon.

---

## 4. Funcții Disponibile în Portalul Web

1. **Configurare Wi-Fi**: Conectare automată pe 3 sloturi de rețele prioritare sau fallback în mod Access Point (`BaroClock`).
2. **Calibrare QFE (Presiune la Sol)**: Câmp dedicat pentru introducerea presiunii de la un barometru etalon local (la cota terenului).
3. **Calibrare Temperatură**: Câmp dedicat pentru compensarea încălzirii interne din carcasă (°C offset persistent).
4. **Selecție Unitate Presiune**: Opțiune de afișare pe LCD și Web între **hPa**, **mb** sau **mmHg**.
5. **Locator Manual**: Posibilitate de forțare manuală a QTH Locator-ului (util când nu este conectat un modul GPS).
6. **Vreme Curentă Observată (Referință de Fond)**: Selector în portalul Web (*Senin / Frumos*, *Cer înnorat / Închis*, *Ploaie activă / Vreme ploioasă*) ce ancorează realist prognoza barică chiar și în regimuri de presiune staționară.
7. **Test Sunet & Proximitate**: Comutator activare/dezactivare test sunet la apropiere (default ON), monitorizare valoare brută proximitate și buton de test manual difuzor.
8. **Setări Afișaj & Grafic**: Rotire ecran la 180°, selecție unitate temperatură (°C / °F), oră de vară (DST +1h) și pas de eșantionare barograf.

---

## 5. Changelog

### Versiunea V.0.1.0
* **Test Sunet Difuzor & Senzor Proximitate**:
  * Implementate fișiere dedicate [`proximity.h / .cpp`](file:///e:/PROIECTE%20ELECTRONICA/ESP_32S3_Agent_DevKit/Baro_Clock/baroclock/proximity.h) pentru senzorul MK-TP2016 (I2C 0x39).
  * Implementate fișiere dedicate [`sound.h / .cpp`](file:///e:/PROIECTE%20ELECTRONICA/ESP_32S3_Agent_DevKit/Baro_Clock/baroclock/sound.h) pentru codec-ul ES8311, I2S și comanda amplificatorului pe AW9523 P1_5.
  * Bip de test acustic configurabil (1200 Hz / 300 ms, default ON) declanșat la apropierea mâinii, cu debounce/cooldown.
  * Comenzi dedicate în portalul Web pentru activare/dezactivare și emitere bip manual.
* **Mesaje Explicite Centralizate în `setup.h`**:
  * Toate textele de stare meteo au fost migrate în constante `#define` pentru configurare ușoară.
* **Regim Meteo Observat (Referință de Fond în Web Portal)**:
  * Adăugată posibilitatea de a stabili vremea de bază la fața locului (*Senin*, *Cer înnorat*, *Ploaie activă*).
  * Eliminată discrepanța afișării soarelui în perioade de vreme închisă/ploioasă prelungită cu presiune staționară.
  * Modularea dinamică a iconițelor și textelor (`[STABIL - NOROS]`, `[STABIL - PLOAIE]`, `[IN AMELIORARE]`, etc.) corelată cu tendința barică.
* **Redesign UI LCD**:
  * Adăugată casetă mare pe stânga ($76 \times 86$ px) cuprinzând simultan înălțimea liniei de prognoză și a celei de senzori.
  * Desenate iconițe meteo vectoriale mari de înaltă rezoluție (Soare masiv cu 8 raze, Soare+Nor, Nor mare stratificat, Ploaie cu 5 dungi cyan oblice, Furtună cu fulger zig-zag galben, Radar GPS pentru locație nouă).
  * Eliminarea etichetelor redundante `P:` și `T:` de pe ecran.
  * Separator parțial la $Y = 208$ (de la $X = 88$ la $479$), păstrând caseta din stânga neîntreruptă.
* **Unități de Presiune Configurabile**:
  * Adăugat selector în Web Portal pentru `hPa`, `mb` și `mmHg`.
  * Conversie automată în timp real ($P_{\text{mmHg}} = P_{\text{hPa}} \times 0.750062$).
* **Calibrare Temperatură**:
  * Adăugat card dedicat de calibrare etalon în interfața web cu salvare persistentă a offset-ului în NVS.
  * Compensare aplicată transparent atât la afișarea în Celsius cât și în Fahrenheit.
* **Locator Manual Flexibil**:
  * Intrare pentru QTH Locator în Web Portal ce poate fi setat oricând; dacă GPS-ul obține fix valid, locatorul este actualizat automat cu datele din satelit.
* **Monitorizare Sateliți GPS**:
  * Afișare număr sateliți în header în stânga antenei parabolice (spațiu de 6px, culoare albă la fix valid, roșie la căutare).
* **Smart Refresh Independent pentru Presiune și Temperatură (Zero-Flicker)**:
  * Separarea completă a zonelor de randare: Presiunea la $X = 92$, Temperatura la $X = 290$.
  * Eliminat apelul `lcdFillRect` pe toată linia la fiecare variație de senzor.
  * Rescrierea diferențială se face exclusiv pentru valoarea modificată, eliminând complet efectul de clipire pe orizontală.
* **Barograf Rolling Strip Chart & Auto-Centrare pe Axa Y**:
  * Grafic cu derulare orizontală spre stânga și axă statică de timp ancorată în prezent ($0\text{h}$ la marginea din dreapta $X = 478$), eliminând orice vibrație sau rescriere a reperelor temporale.
  * **Auto-centrare permanentă a noului pixel pe mijlocul axei Y (`midY = 295`)**: cel mai recent punct este întotdeauna pe linia de mijloc, eliminând fenomenul de drift baric la marginea inferioară sau superioară a casetei.
  * **Stocare istoric presiuni în RAM (`float baroPressHistory[480]`)**: comutarea sensibilității (High/Low) din pagina Web recalculează și redesenează instantaneu întreaga curbă pe noua rezoluție.
  * **Clipping curat**: punctele din trecut ale căror variații depășesc limitele utile ale casetei nu se desenează (ies natural din cadru), protejând chenarul alb și axa de timp fără artefacte.

