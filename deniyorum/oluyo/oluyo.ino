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

#define BUTTON_ROWS     3
#define BUTTON_COLUMNS  3
#define BUTTON_SPACING  10

int currentPage = 0; // 0: Ana Sayfa, 1: Sayfa 2

bool buttonPressed[9]    = {false};
bool lastButtonState[9]  = {false};

uint16_t buttonColors[9] = {
  RED, GREEN, BLUE,
  YELLOW, CYAN, MAGENTA,
  ORANGE, PURPLE, WHITE
};

// Sayfa 1 komutları ve etiketleri
const char* buttonCommandsPage1[9] = {
  "CMD: VOLUME_DOWN", "CMD: VOLUME_UP",   "CMD: BRIGHTNESS_DOWN",
  "CMD: BRIGHTNESS_UP","CMD: OPEN_BROWSER","CMD: CLOSE_APP",
  "CMD: LOCK_SCREEN",  "CMD: ALT_TAB",     "PAGE: NEXT"
};
const char* buttonLabelsPage1[9] = {
  "SES-", "SES+", "PRLK-",
  "PRLK+", "GOOGLE", "ALT+F4",
  "LOCK", "ALT+TAB",  ">"
};

// **➤ Sayfa 2 komutları `"CMD: BTN#"` formatında**
const char* buttonCommandsPage2[9] = {
  "CMD: BTN0","CMD: BTN1","CMD: BTN2",
  "CMD: BTN3","CMD: BTN4","CMD: BTN5",
  "CMD: BTN6","CMD: BTN7","PAGE: BACK"
};
const char* buttonLabelsPage2[9] = {
  "1","2","3",
  "4","5","6",
  "7","8","<"
};

// Dinamik olarak hangi diziye bakacağımızı tutan pointerlar
const char** buttonCommands = buttonCommandsPage1;
const char** buttonLabels   = buttonLabelsPage1;

int buttonX[9], buttonY[9];
int buttonWidth, buttonHeight;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  // Buton boyutlarını hesapla
  buttonWidth  = (tft.width()  - (BUTTON_COLUMNS + 1) * BUTTON_SPACING) / BUTTON_COLUMNS;
  buttonHeight = (tft.height() - (BUTTON_ROWS    + 1) * BUTTON_SPACING) / BUTTON_ROWS;

  // Buton pozisyonlarını doldur
  for (int i = 0; i < 9; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;
    buttonX[i] = BUTTON_SPACING + col * (buttonWidth  + BUTTON_SPACING);
    buttonY[i] = BUTTON_SPACING + row * (buttonHeight + BUTTON_SPACING);
  }

  // İlk sayfayı çiz
  drawButtons();
}

void loop() {
  TSPoint p = ts.getPoint();
  // Dokunmatik pinlerini tekrar OUTPUT moda al
  pinMode(YP, OUTPUT); pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH); digitalWrite(XM, HIGH);

  // Her döngüde basılı durumu sıfırla
  for (int i = 0; i < 9; i++) {
    buttonPressed[i] = false;
  }

  // Dokunma varsa hangi butona basıldı?
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

  // Buton durumları değişmiş mi?
  for (int i = 0; i < 9; i++) {
    if (buttonPressed[i] != lastButtonState[i]) {
      drawButton(i, buttonPressed[i]);
      lastButtonState[i] = buttonPressed[i];

      if (buttonPressed[i]) {
        const char* cmd = buttonCommands[i];
        // Sayfa değiştirme komutları
        if      (strcmp(cmd, "PAGE: NEXT") == 0) {
          currentPage = 1;
          switchPage();
        }
        else if (strcmp(cmd, "PAGE: BACK") == 0) {
          currentPage = 0;
          switchPage();
        }
        else {
          // Normal komutları gönder
          Serial1.println(cmd);
          Serial.println(cmd);
        }
      }
    }
  }

  delay(50);
}

// Sayfa değiştiğinde pointerları güncelle, eski buton durumlarını sıfırla ve ekranı yeniden çiz
void switchPage() {
  if (currentPage == 0) {
    buttonCommands = buttonCommandsPage1;
    buttonLabels   = buttonLabelsPage1;
  } else {
    buttonCommands = buttonCommandsPage2;
    buttonLabels   = buttonLabelsPage2;
  }

  // **Önceki sayfanın basılı durumlarını temizliyoruz**
  for (int i = 0; i < 9; i++) {
    lastButtonState[i] = false;
  }

  drawButtons();
}

void drawButtons() {
  tft.fillScreen(BLACK);
  for (int i = 0; i < 9; i++) {
    drawButton(i, false);
    lastButtonState[i] = false;
  }
}

void drawButton(int i, bool pressed) {
  uint16_t fillColor = pressed ? WHITE : buttonColors[i];
  uint16_t textColor = pressed ? BLACK : WHITE;
  const int radius = 10;

  tft.fillRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, radius, fillColor);
  tft.drawRoundRect(buttonX[i], buttonY[i], buttonWidth, buttonHeight, radius, WHITE);

  // Buton etiketini tam ortala
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(buttonLabels[i], 0, 0, &x1, &y1, &w, &h);
  int tx = buttonX[i] + (buttonWidth  - w) / 2;
  int ty = buttonY[i] + (buttonHeight - h) / 2;

  tft.setTextColor(textColor);
  tft.setTextSize(2);
  tft.setCursor(tx, ty);
  tft.print(buttonLabels[i]);

  // Ekranı ters çevirme (invert)
  tft.invertDisplay(true);
}
