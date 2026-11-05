#include "lcd_driver.h"
#include <Wire.h>

// Full ASCII font 5x7 table (32 ' ' to 126 '~')
static const uint8_t font5x7[95][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // 126 '~'
};

static const uint8_t LCD_D_PINS[8] = {
    LCD_DB0, LCD_DB1, LCD_DB2, LCD_DB3, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7
};

static bool currentRotation180 = false;

static void awWrite(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(AW9523_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static void lcdBusWrite(uint8_t value) {
    for (uint8_t bit = 0; bit < 8; bit++) {
        digitalWrite(LCD_D_PINS[bit], (value >> bit) & 0x01);
    }
    digitalWrite(LCD_WR, LOW);
    delayMicroseconds(1);
    digitalWrite(LCD_WR, HIGH);
}

static void lcdCmd(uint8_t cmd) {
    digitalWrite(LCD_RS, LOW);
    lcdBusWrite(cmd);
}

static void lcdData(uint8_t value) {
    digitalWrite(LCD_RS, HIGH);
    lcdBusWrite(value);
}

static void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcdCmd(0x2A);
    lcdData(x0 >> 8); lcdData(x0 & 0xFF);
    lcdData(x1 >> 8); lcdData(x1 & 0xFF);

    lcdCmd(0x2B);
    lcdData(y0 >> 8); lcdData(y0 & 0xFF);
    lcdData(y1 >> 8); lcdData(y1 & 0xFF);

    lcdCmd(0x2C);
}

void lcdSetRotation(bool rotate180) {
    currentRotation180 = rotate180;
    lcdCmd(0x36); // MADCTL
    if (rotate180) {
        // Landscape 180 (MY=1, MX=1, MV=1, BGR=1) -> 0xE8
        lcdData(0xE8);
    } else {
        // Landscape normal (MY=0, MX=0, MV=1, BGR=1) -> 0x28
        lcdData(0x28);
    }
    delay(20);
}

void lcdInitHardware(bool rotate180) {
    // Configure LCD control & data pins
    pinMode(LCD_RS, OUTPUT);
    pinMode(LCD_WR, OUTPUT);
    digitalWrite(LCD_RS, HIGH);
    digitalWrite(LCD_WR, HIGH);
    for (uint8_t i = 0; i < 8; i++) {
        pinMode(LCD_D_PINS[i], OUTPUT);
        digitalWrite(LCD_D_PINS[i], LOW);
    }

    // AW9523 IO expander configuration (identic cu Reveil-CYDGOLD / Internet_radio_clock)
    awWrite(0x11, 0x10);         // GCR: Seteaza Portul 1 in modul Push-Pull 3.3V (Bit 4 = 1)
    awWrite(0x12, 0xFF);         // Switch P0 pe modul GPIO
    awWrite(0x13, 0xFF);         // Switch P1 pe modul GPIO (P1_0..P1_3 BL cathodes LOW = BL ON, P1_5 = PA)
    awWrite(0x04, 0xFF);         // P0 all inputs
    awWrite(0x05, 0x00);         // P1 all outputs
    awWrite(0x03, LCD_RST_BIT);  // BL ON + RST HIGH
    delay(20);

    // Hardware reset LCD
    awWrite(0x03, LCD_RST_BIT);
    delay(20);
    awWrite(0x03, 0x00);         // Reset active (RST LOW)
    delay(100);
    awWrite(0x03, LCD_RST_BIT);  // Reset released (RST HIGH)
    delay(200);

    // Display controller initialization
    lcdCmd(0x01); delay(200);              // SWRESET
    lcdCmd(0x11); delay(200);              // SLEEPOUT
    lcdCmd(0x3A); lcdData(0x55); delay(20); // RGB565 format

    lcdSetRotation(rotate180);

    lcdCmd(0x29); delay(100);              // DISPLAY ON
}

void lcdFillScreen(uint16_t color) {
    setWindow(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    uint8_t hi = ~(color >> 8);
    uint8_t lo = ~(color & 0xFF);
    uint32_t total = (uint32_t)SCREEN_WIDTH * SCREEN_HEIGHT;
    for (uint32_t i = 0; i < total; i++) {
        lcdData(hi);
        lcdData(lo);
    }
}

void lcdDrawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || w <= 0 || h <= 0 || !pixels) return;
    setWindow(x, y, x + w - 1, y + h - 1);
    uint32_t total = (uint32_t)w * h;
    for (uint32_t i = 0; i < total; i++) {
        uint16_t c = pixels[i];
        lcdData(~(c >> 8));
        lcdData(~(c & 0xFF));
    }
}

void lcdFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    setWindow(x, y, x + w - 1, y + h - 1);
    uint8_t hi = ~(color >> 8);
    uint8_t lo = ~(color & 0xFF);
    uint32_t total = (uint32_t)w * h;
    for (uint32_t i = 0; i < total; i++) {
        lcdData(hi);
        lcdData(lo);
    }
}

void lcdDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    lcdFillRect(x, y, w, 1, color);
    lcdFillRect(x, y + h - 1, w, 1, color);
    lcdFillRect(x, y, 1, h, color);
    lcdFillRect(x + w - 1, y, 1, h, color);
}

void lcdDrawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    setWindow(x, y, x, y);
    lcdData(~(color >> 8));
    lcdData(~(color & 0xFF));
}

void lcdDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true) {
        lcdDrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void lcdDrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale) {
    if (x + 6 * scale <= 0 || x >= SCREEN_WIDTH || y + 8 * scale <= 0 || y >= SCREEN_HEIGHT) return;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *chr = font5x7[c - 32];

    uint8_t fgHi = ~(color >> 8);
    uint8_t fgLo = ~(color & 0xFF);
    uint8_t bgHi = ~(bg >> 8);
    uint8_t bgLo = ~(bg & 0xFF);

    // If completely within bounds, stream the rectangle in one window
    if (x >= 0 && (x + 6 * scale) <= SCREEN_WIDTH && y >= 0 && (y + 8 * scale) <= SCREEN_HEIGHT) {
        setWindow(x, y, x + 6 * scale - 1, y + 8 * scale - 1);
        for (uint8_t row = 0; row < 8 * scale; row++) {
            uint8_t char_row = row / scale;
            for (uint8_t col = 0; col < 6 * scale; col++) {
                uint8_t char_col = col / scale;
                bool pixelOn = false;
                if (char_col < 5 && char_row < 7) {
                    pixelOn = (pgm_read_byte(&chr[char_col]) & (1 << char_row)) != 0;
                }
                if (pixelOn) {
                    lcdData(fgHi);
                    lcdData(fgLo);
                } else {
                    lcdData(bgHi);
                    lcdData(bgLo);
                }
            }
        }
    } else {
        // Fallback for clipped characters
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t line = pgm_read_byte(&chr[col]);
            for (uint8_t row = 0; row < 7; row++) {
                uint16_t cColor = (line & (1 << row)) ? color : bg;
                lcdFillRect(x + col * scale, y + row * scale, scale, scale, cColor);
            }
        }
        lcdFillRect(x + 5 * scale, y, scale, 8 * scale, bg);
    }
}

void lcdDrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale) {
    int16_t curX = x;
    while (*str) {
        lcdDrawChar(curX, y, *str++, color, bg, scale);
        curX += 6 * scale;
    }
}

void lcdDrawString(int16_t x, int16_t y, const String &str, uint16_t color, uint16_t bg, uint8_t scale) {
    lcdDrawString(x, y, str.c_str(), color, bg, scale);
}

void lcdDrawSatIcon(int16_t cx, int16_t cy, uint16_t color, uint16_t bg) {
    // Zona de 30x22 pixeli pentru satelit cu panouri si unde radio
    lcdFillRect(cx - 15, cy - 10, 30, 22, bg);
    if (color == bg) return; // Daca GPS e deconectat, ramane fundal negru

    // 1. Corp central satelit (fill cu colturi tesite)
    lcdFillRect(cx - 3, cy - 3, 7, 7, color);
    lcdDrawPixel(cx - 3, cy - 3, bg);
    lcdDrawPixel(cx + 3, cy - 3, bg);
    lcdDrawPixel(cx - 3, cy + 3, bg);
    lcdDrawPixel(cx + 3, cy + 3, bg);

    // Punct central (camera/focar)
    lcdDrawPixel(cx, cy, bg);

    // 2. Conexiuni panouri solare
    lcdDrawLine(cx - 5, cy, cx - 3, cy, color);
    lcdDrawLine(cx + 3, cy, cx + 5, cy, color);

    // 3. Panouri solare laterale (dreptunghiuri 5x6)
    // Panou stanga
    lcdFillRect(cx - 11, cy - 3, 5, 7, color);
    lcdDrawLine(cx - 9, cy - 3, cx - 9, cy + 3, bg); // grilaj celule

    // Panou dreapta
    lcdFillRect(cx + 7, cy - 3, 5, 7, color);
    lcdDrawLine(cx + 9, cy - 3, cx + 9, cy + 3, bg); // grilaj celule

    // 4. Unde radio GPS (arcuri de cerc concentrice sus si jos)
    // Arc interior sus (raza 6)
    lcdDrawPixel(cx - 3, cy - 6, color);
    lcdDrawLine(cx - 2, cy - 7, cx + 2, cy - 7, color);
    lcdDrawPixel(cx + 3, cy - 6, color);

    // Arc exterior sus (raza 9)
    lcdDrawPixel(cx - 5, cy - 9, color);
    lcdDrawLine(cx - 3, cy - 10, cx + 3, cy - 10, color);
    lcdDrawPixel(cx + 5, cy - 9, color);

    // Arc interior jos (raza 6)
    lcdDrawPixel(cx - 3, cy + 6, color);
    lcdDrawLine(cx - 2, cy + 7, cx + 2, cy + 7, color);
    lcdDrawPixel(cx + 3, cy + 6, color);

    // Arc exterior jos (raza 9)
    lcdDrawPixel(cx - 5, cy + 9, color);
    lcdDrawLine(cx - 3, cy + 10, cx + 3, cy + 10, color);
    lcdDrawPixel(cx + 5, cy + 9, color);
}

