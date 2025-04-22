#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include "TouchScreen.h"

// Renkler
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define ORANGE  0xFD20
#define PURPLE  0x8010

// Dokunmatik ayarları
#define TS_MINX 150
#define TS_MAXX 920
#define TS_MINY 120
#define TS_MAXY 940

#define XP 9
#define XM A3
#define YP A2
#define YM 8
#define TS_PRESSURE_MIN 10
#define TS_PRESSURE_MAX 1000

MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Buton düzeni
#define BUTTON_ROWS 3
#define BUTTON_COLUMNS 3
#define BUTTON_SPACING 10

bool buttonPressed[9] = {false};
bool lastButtonState[9] = {false};

// Buton renkleri (9 farklı buton)
uint16_t buttonColors[9] = {
  RED, GREEN, BLUE, YELLOW,
  CYAN, MAGENTA, ORANGE, PURPLE,
  WHITE
};



// Gönderilecek komutlar
const char* buttonCommands[9] = {
  "CMD: VOLUME_DOWN",
  "CMD: VOLUME_UP",
  "CMD: BRIGHTNESS_DOWN",
  "CMD: BRIGHTNESS_UP",
  "CMD: OPEN_BROWSER",
  "CMD: CLOSE_APP",
  "CMD: LOCK_SCREEN",
  "CMD: ALT_TAB",
  "CMD: CUSTOM"
};

// Buton yazıları
const char* buttonLabels[9] = {
  "SES-", "SES+", "PRLK-",
  "PRLK+", "GOOGLE", "ALT+F4",
  "LOCK", "ALT+TAB", ">"
};

// Konum ve boyut
int buttonX[9];
int buttonY[9];
int buttonWidth;
int buttonHeight;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  uint16_t ID = tft.readID();

  tft.begin(ID);
  tft.setRotation(0);

  tft.fillScreen(BLACK); // Ekran arka planı siyah
 
  buttonWidth = (tft.width() - (BUTTON_COLUMNS + 1) * BUTTON_SPACING) / BUTTON_COLUMNS;
  buttonHeight = (tft.height() - (BUTTON_ROWS + 1) * BUTTON_SPACING) / BUTTON_ROWS;

  for (int i = 0; i < 9; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;

    buttonX[i] = BUTTON_SPACING + col * (buttonWidth + BUTTON_SPACING);
    buttonY[i] = BUTTON_SPACING + row * (buttonHeight + BUTTON_SPACING);

    drawButton(i, false); // Başlangıçta basılı değil
  }

  Serial.println("Hazır!");
}

void loop() {
  TSPoint p = ts.getPoint();

  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);

  for (int i = 0; i < 9; i++) {
    buttonPressed[i] = false;
  }

  if (p.z > TS_PRESSURE_MIN && p.z < TS_PRESSURE_MAX) {
    int x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    int y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0) + 10;

    for (int i = 0; i < 9; i++) {
      if (x >= buttonX[i] && x < buttonX[i] + buttonWidth &&
          y >= buttonY[i] && y < buttonY[i] + buttonHeight) {
        buttonPressed[i] = true;
      }
    }
  }

  for (int i = 0; i < 9; i++) {
    if (buttonPressed[i] != lastButtonState[i]) {
      drawButton(i, buttonPressed[i]);
      lastButtonState[i] = buttonPressed[i];

      if (buttonPressed[i]) {
        Serial1.println(buttonCommands[i]);
        Serial.println(buttonCommands[i]);
      }
    }
  }

  delay(50);
}

void drawButton(int i, bool pressed) {
  uint16_t fill_color = pressed ? WHITE : buttonColors[i];
  uint16_t text_color = pressed ? WHITE : BLACK;
  int radius = 10;

  tft.fillRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, radius, fill_color);
  tft.drawRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, radius, WHITE);

  tft.setTextColor(text_color);
  tft.setTextSize(2);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(buttonLabels[i], 0, 0, &x1, &y1, &w, &h);

  int x = buttonX[i] + (buttonWidth - w) / 2;
  int y = buttonY[i] + (buttonHeight - h) / 2;

  tft.setCursor(x, y);
  tft.print(buttonLabels[i]);
   tft.invertDisplay(true);
}
