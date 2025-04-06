#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include "TouchScreen.h"

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

#define BUTTON_ROWS 4
#define BUTTON_COLUMNS 2

bool buttonPressed[8] = {false};
bool lastButtonState[8] = {false};

uint16_t buttonColors[8] = {
  RED, GREEN, BLUE, YELLOW,
  CYAN, MAGENTA, ORANGE, PURPLE
};

const char* buttonCommands[8] = {
  "CMD: VOLUME_DOWN",
  "CMD: VOLUME_UP",
  "CMD: BRIGHTNESS_DOWN",
  "CMD: BRIGHTNESS_UP",
  "CMD: OPEN_BROWSER",
  "CMD: CLOSE_APP",
  "CMD: LOCK_SCREEN",
  "CMD: ALT_TAB"
};

const char* buttonLabels[8] = {
  "SES -", "SES +", "PARLAK -", "PARLAK +",
  "TARAYICI", "KAPAT", "KILITLE", "ALT+TAB"
};

int buttonX[8];
int buttonY[8];
int buttonWidth;
int buttonHeight;

void setup() {
  Serial.begin(9600);      // Bilgisayar üzerinden debug için
  Serial1.begin(9600);     // HC-05 ile haberleşme

  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  buttonWidth = tft.width() / BUTTON_COLUMNS;
  buttonHeight = tft.height() / BUTTON_ROWS;

  for (int i = 0; i < 8; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;

    buttonX[i] = col * buttonWidth;
    buttonY[i] = row * buttonHeight;

    drawButton(i, false);
  }

  Serial.println("Dokunmatik kontrolcü hazır.");
}

void loop() {
  TSPoint p = ts.getPoint();

  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);

  for (int i = 0; i < 8; i++) {
    buttonPressed[i] = false;
  }

  if (p.z > TS_PRESSURE_MIN && p.z < TS_PRESSURE_MAX) {
    int x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    int y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0) + 10;

    for (int i = 0; i < 8; i++) {
      if (x >= buttonX[i] && x < buttonX[i] + buttonWidth &&
          y >= buttonY[i] && y < buttonY[i] + buttonHeight) {
        buttonPressed[i] = true;
      }
    }
  }

  for (int i = 0; i < 8; i++) {
    if (buttonPressed[i] != lastButtonState[i]) {
      drawButton(i, buttonPressed[i]);
      lastButtonState[i] = buttonPressed[i];

      if (buttonPressed[i]) {
        Serial1.println(buttonCommands[i]); // Sadece HC-05'e gönder
        Serial.println(buttonCommands[i]);  // Debug
      }
    }
  }

  delay(50);
}

void drawButton(int i, bool pressed) {
  uint16_t fill_color = pressed ? WHITE : buttonColors[i];
  uint16_t text_color = pressed ? BLACK : WHITE;
  uint16_t outline_color = WHITE;

  tft.fillRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, fill_color);
  tft.drawRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, outline_color);

  tft.setTextColor(text_color);
  tft.setTextSize(2);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(buttonLabels[i], 0, 0, &x1, &y1, &w, &h);

  int x = buttonX[i] + (buttonWidth - w) / 2;
  int y = buttonY[i] + (buttonHeight - h) / 2;

  tft.setCursor(x, y);
  tft.print(buttonLabels[i]);
}
