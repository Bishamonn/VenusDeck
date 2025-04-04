#include <BleKeyboard.h>
#include <TFT_eSPI.h>
#include <TouchScreen.h>

// TFT ekran ayarları
TFT_eSPI tft = TFT_eSPI();
#define MINPRESSURE 1
#define MAXPRESSURE 40000

// Dokunmatik ekran pinleri
const int XP = 23, XM = 32, YP = 21, YM = 13;
const int TS_LEFT = 20, TS_RT = 466, TS_TOP = -53, TS_BOT = 373;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

int pixel_x, pixel_y;

// BLE Klavye
BleKeyboard bleKeyboard("VenusDeck", "My_Company", 100);

// Dokunmatik ekran okuma fonksiyonu
bool Touch_getXY() {
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);
  p.z = abs(p.z);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (pressed) {
    // **X ve Y eksenlerini değiştirdik!**
    int temp_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
    int temp_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());

    // **Negatif Y değerlerini düzeltme**
    if (temp_y < 0) {
      temp_y = abs(temp_y);
    }

    pixel_x = temp_x;
    pixel_y = temp_y;

    Serial.print("Düzgün Dokunma X: ");
    Serial.print(pixel_x);
    Serial.print(" Y: ");
    Serial.println(pixel_y);
  }

  return pressed;
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);

  // **Ekranı 2x2 bölümlere ayır ve renklendir**
  tft.fillScreen(TFT_BLACK);
  int w = tft.width() / 2;
  int h = tft.height() / 2;

  tft.fillRect(0, 0, w, h, TFT_GREEN);    // Ses Aç
  tft.fillRect(w, 0, w, h, TFT_RED);      // Ses Kıs
  tft.fillRect(0, h, w, h, TFT_YELLOW);   // Önceki Şarkı
  tft.fillRect(w, h, w, h, TFT_BLUE);     // Sonraki Şarkı

  // **Bilgilendirici yazılar (Ortalanmış)**
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);

  tft.setCursor(w / 4, h / 3);
  tft.println("SES AC");

  tft.setCursor(3 * w / 4 - 30, h / 3);
  tft.println("SES KIS");

  tft.setCursor(w / 4, h + h / 3);
  tft.println("ONCEKI");

  tft.setCursor(3 * w / 4 - 30, h + h / 3);
  tft.println("SONRAKI");

  Serial.println("BLE Klavye başlatılıyor...");
  bleKeyboard.begin();
}

void loop() {
  if (bleKeyboard.isConnected()) {
    if (Touch_getXY()) {
      int w = tft.width() / 2;
      int h = tft.height() / 2;

      if (pixel_x < w && pixel_y < h) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        Serial.println("Ses Aç!");
      } else if (pixel_x >= w && pixel_y < h) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        Serial.println("Ses Kıs!");
      } else if (pixel_x < w && pixel_y >= h) {
        bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        Serial.println("Önceki Şarkı!");
      } else {
        bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        Serial.println("Sonraki Şarkı!");
      }

      delay(100);
      while (Touch_getXY()) {
        delay(50);
      }
    }
  }
  delay(100);
}
