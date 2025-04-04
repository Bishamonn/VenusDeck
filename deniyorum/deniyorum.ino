#include <TFT_eSPI.h>
#include <TouchScreen.h>

// TFT ekran pin bağlantıları

// Dokunmatik ekran pin bağlantıları
#define TS_CS 21
#define TS_IRQ 13
#define TS_DOUT 32
#define TS_DIN 33

// Butonun konumu ve boyutu
#define BUTTON_X 50   // Butonun X koordinatı
#define BUTTON_Y 50   // Butonun Y koordinatı
#define BUTTON_W 100  // Butonun genişliği
#define BUTTON_H 40   // Butonun yüksekliği

TFT_eSPI tft = TFT_eSPI();
TouchScreen ts = TouchScreen(TS_CS, TS_IRQ, TS_DOUT, TS_DIN, 300); // Pinleri burada belirtiyoruz

bool buttonState = false;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  drawButton();
  Serial.println("Dokunmatik Başlatıldı!");
}

void loop() {
  TSPoint p = ts.getPoint();
  pinMode(13, OUTPUT);  // IRQ pinini çıkış olarak ayarlıyoruz
  pinMode(12, OUTPUT);  // DOUT pinini çıkış olarak ayarlıyoruz
  
  if (p.z > ts.pressureThreshhold) {
    // Dokunmatik ekranın koordinatlarını kontrol et
    if (p.x > BUTTON_X && p.x < BUTTON_X + BUTTON_W && p.y > BUTTON_Y && p.y < BUTTON_Y + BUTTON_H) {
      buttonState = !buttonState; // Butona basıldığında durumu değiştir
      drawButton(); // Butonun rengini değiştir
      delay(200); // Debounce efekti
    }
  }
}

void drawButton() {
  if (buttonState) {
    tft.fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, 8, TFT_GREEN);
  } else {
    tft.fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, 8, TFT_RED);
  }
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(BUTTON_X + 20, BUTTON_Y + 20);
  tft.print("Ses ");
  tft.print(buttonState ? "Acik" : "Kapali");
}
