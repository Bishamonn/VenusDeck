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
#define GRAY    0xC618

// Dokunmatik kalibrasyon
#define TS_MINX 150
#define TS_MAXX 920
#define TS_MINY 120
#define TS_MAXY 940

// Dokunmatik pinler
#define XP 9
#define XM A3
#define YP A2
#define YM 8
#define TS_PRESSURE_MIN 10
#define TS_PRESSURE_MAX 1000

MCUFRIEND_kbv tft(A3, A2, A1, A0, A4);
TouchScreen ts(XP, YP, XM, YM, 300);

// Buton düzeni
#define BUTTON_ROWS     3
#define BUTTON_COLUMNS  3
#define BUTTON_SPACING  10

bool buttonPressed[9];
bool lastButtonState[9];

uint16_t buttonColors[9] = {
  RED, GREEN, BLUE,
  YELLOW, CYAN, MAGENTA,
  ORANGE, PURPLE, WHITE
};

// Tüm buton komutları
const char* buttonCommands[9] = {
  "CMD: VOLUME_DOWN", "CMD: VOLUME_UP", "CMD: BRIGHTNESS_DOWN",
  "CMD: BRIGHTNESS_UP", "CMD: OPEN_BROWSER", "CMD: CLOSE_APP",
  "CMD: LOCK_SCREEN", "CMD: ALT_TAB", "CMD: CUSTOM_ACTION" // 9. buton için özel bir komut
};

const char* buttonLabels[9] = {
  "SES-", "SES+", "PRLK-",
  "PRLK+", "GOOGLE", "ALT+F4",
  "LOCK", "ALT+TAB", "ÖZEL" // 9. buton yazısı
};

int buttonX[9], buttonY[9];
int buttonWidth, buttonHeight;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  buttonWidth  = (tft.width()  - (BUTTON_COLUMNS + 1) * BUTTON_SPACING) / BUTTON_COLUMNS;
  buttonHeight = (tft.height() - (BUTTON_ROWS    + 1) * BUTTON_SPACING) / BUTTON_ROWS;

  for (int i = 0; i < 9; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;
    buttonX[i] = BUTTON_SPACING + col * (buttonWidth  + BUTTON_SPACING);
    buttonY[i] = BUTTON_SPACING + row * (buttonHeight + BUTTON_SPACING);
  }

  drawButtons();
}

void loop() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT); pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH); digitalWrite(XM, HIGH);

  for (int i = 0; i < 9; i++) buttonPressed[i] = false;

  if (p.z > TS_PRESSURE_MIN && p.z < TS_PRESSURE_MAX) {
    int x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    int y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
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
        const char* cmd = buttonCommands[i];
        Serial1.println(cmd);
        Serial.println(cmd);
      }
    }
  }

  delay(50);
}

void drawButtons() {
  tft.fillScreen(BLACK);
  for (int i = 0; i < 9; i++) {
    drawButton(i, false);
    lastButtonState[i] = false;
  }
}

void drawButton(int i, bool pressed) {
  uint16_t fg = pressed ? BLACK : WHITE;
  uint16_t bg = pressed ? WHITE : buttonColors[i];
  const int r = 10;
  tft.fillRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, r, bg);
  tft.drawRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, r, WHITE);
  tft.setTextColor(fg);
  tft.setTextSize(2);
  int16_t x1,y1; uint16_t w,h;
  tft.getTextBounds(buttonLabels[i],0,0,&x1,&y1,&w,&h);
  int tx = buttonX[i] + (buttonWidth - w)/2;
  int ty = buttonY[i] + (buttonHeight - h)/2;
  tft.setCursor(tx, ty);
  tft.print(buttonLabels[i]);
  tft.invertDisplay(true);
}
