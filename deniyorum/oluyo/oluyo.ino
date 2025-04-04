#include <BleKeyboard.h>
#include <TFT_eSPI.h>
#include <TouchScreen.h>

// TFT ekran ayarları
TFT_eSPI tft = TFT_eSPI();
#define MINPRESSURE 1
#define MAXPRESSURE 40000

// Dokunmatik ekran pinleri
const int XP = 23, XM = 32, YP = 21, YM = 13;
const int TS_LEFT = -2500, TS_RT = 517, TS_TOP = 780, TS_BOT = -2650;
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
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width());
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
  }
  return pressed;
}

void setup() {
  Serial.begin(921600);
  tft.init();
  tft.setRotation(1);

  // **Ekranı beşe böl ve farklı renklerde göster**
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, tft.width(), tft.height() / 5, TFT_GREEN);    // Ses Aç
  tft.fillRect(0, tft.height() / 5, tft.width(), tft.height() / 5, TFT_RED);  // Ses Kıs
  tft.fillRect(0, 2 * tft.height() / 5, tft.width(), tft.height() / 5, TFT_YELLOW);  // Sonraki Şarkı
  tft.fillRect(0, 3 * tft.height() / 5, tft.width(), tft.height() / 5, TFT_ORANGE);  // Önceki Şarkı
  tft.fillRect(0, 4 * tft.height() / 5, tft.width(), tft.height() / 5, TFT_BLUE);  // Spotify Aç

  // **Bilgilendirici yazılar**
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, tft.height() / 10);
  tft.println("BASLA / DUR");
  tft.setCursor(50, 3 * tft.height() / 10);
  tft.println("ONCEKİ SARKI");
  tft.setCursor(50, 5 * tft.height() / 10);
  tft.println("SONRAKI SARKI");
  tft.setCursor(50, 7 * tft.height() / 10);
  tft.println("SES AC");
  tft.setCursor(50, 9 * tft.height() / 10);
  tft.println("SES KIS");

  Serial.println("BLE Klavye başlatılıyor...");
  bleKeyboard.begin();
}

void loop() {
  if (bleKeyboard.isConnected()) {
    if (Touch_getXY()) {
      if (pixel_y < tft.height() / 5) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        Serial.println("Ses Kısma Tuşu Gönderildi!");
      } else if (pixel_y < 2 * tft.height() / 5) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        Serial.println("Ses Açma Tuşu Gönderildi!");
      } else if (pixel_y < 3 * tft.height() / 5) {
        bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
        Serial.println("Sonraki Şarkı!");
      } else if (pixel_y < 4 * tft.height() / 5) {
       bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
        Serial.println("Önceki Şarkı!");
      } else {
       bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        Serial.println("Dur, Başla!");
      }
      delay(10);
      while (Touch_getXY()) {
        delay(50);
      }
    }
  }
  delay(100);
}
