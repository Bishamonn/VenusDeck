#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include "TouchScreen.h"

#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF
#define GRAY    0x8410
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define ORANGE  0xFD20
#define PURPLE  0x8010

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
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// 4x2 düzeninde 8 buton için ayarlar
#define BUTTON_ROWS 4
#define BUTTON_COLUMNS 2

// Buton durumları
bool buttonPressed[8] = {false, false, false, false, false, false, false, false};
bool lastButtonState[8] = {false, false, false, false, false, false, false, false};

// Buton renkleri (normal durum)
uint16_t buttonColors[8] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, ORANGE, PURPLE};

// Buton x ve y pozisyonları, genişlikleri ve yükseklikleri
int buttonX[8];
int buttonY[8];
int buttonWidth;
int buttonHeight;

void setup() {
  Serial.begin(9600);
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0); // Dikey mod
  tft.fillScreen(BLACK);
  
  // Ekran boyutlarına göre buton boyutlarını hesapla
  buttonWidth = tft.width() / BUTTON_COLUMNS;
  buttonHeight = tft.height() / BUTTON_ROWS;
  
  // Buton konumlarını hesapla ve butonları çiz
  for (int i = 0; i < 8; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;
    
    buttonX[i] = col * buttonWidth;
    buttonY[i] = row * buttonHeight;
    
    drawButton(i, false);
  }
  
  Serial.println("8 buton tam ekran olarak hazır. Dokunmatik test başladı.");
}

void loop() {
  TSPoint p = ts.getPoint();
  
  // Dokunmatik pinleri geri ayarla
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);
  
  // Tüm butonları basılmamış duruma getir
  for (int i = 0; i < 8; i++) {
    buttonPressed[i] = false;
  }
  
  // Dokunma kontrolü
  if (p.z > TS_PRESSURE_MIN && p.z < TS_PRESSURE_MAX) {
    int x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    int y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0)+10;
    
    Serial.print("Dokunulan: x="); Serial.print(x);
    Serial.print(" y="); Serial.println(y);
    
    // Tüm butonları kontrol et
    for (int i = 0; i < 8; i++) {
      if (x >= buttonX[i] && x < buttonX[i] + buttonWidth &&
          y >= buttonY[i] && y < buttonY[i] + buttonHeight) {
        buttonPressed[i] = true;
      }
    }
  }
  
  // Buton durumlarını güncelle
  for (int i = 0; i < 8; i++) {
    if (buttonPressed[i] != lastButtonState[i]) {
      drawButton(i, buttonPressed[i]);
      lastButtonState[i] = buttonPressed[i];
      
      if (buttonPressed[i]) {
        Serial.print("Buton ");
        Serial.print(i + 1);
        Serial.println(" basıldı!");
        // Her buton için özel işlevler eklenebilir
      }
    }
  }
  
  delay(50); // Kararlı çalışma için küçük bir gecikme
}

// Belirtilen indeksteki butonu çizen fonksiyon
void drawButton(int buttonIndex, bool pressed) {
  uint16_t fill_color, text_color, outline_color;
  
  if (pressed) {
    fill_color = WHITE;
    text_color = BLACK;
    outline_color = BLACK;
  } else {
    fill_color = buttonColors[buttonIndex];
    text_color = WHITE;
    outline_color = WHITE;
  }
  
  // Butonu çiz
  tft.fillRect(buttonX[buttonIndex], buttonY[buttonIndex], buttonWidth, buttonHeight, fill_color);
  tft.drawRect(buttonX[buttonIndex], buttonY[buttonIndex], buttonWidth, buttonHeight, outline_color);
  
  // Buton metnini ortala
  tft.setTextColor(text_color);
  tft.setTextSize(2);
  
  int textWidth = 12 * 6; // "Buton X" metni için yaklaşık genişlik
  int x = buttonX[buttonIndex] + (buttonWidth - textWidth) / 2;
  int y = buttonY[buttonIndex] + (buttonHeight - 16) / 2;
  
  tft.setCursor(x, y);
  tft.print("Buton ");
  tft.print(buttonIndex + 1);
}