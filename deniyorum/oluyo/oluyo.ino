#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include "TouchScreen.h"
#include <EEPROM.h>

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

// EEPROM adres tanımları
#define EEPROM_MAGIC_NUMBER 0xAB // Yapılandırmanın geçerli olduğunu belirtir
#define EEPROM_START_ADDR 10     // EEPROM'da config'in başladığı adres

bool buttonPressed[9];
bool lastButtonState[9];

uint16_t buttonColors[9] = {
  RED, GREEN, BLUE,
  YELLOW, CYAN, MAGENTA,
  ORANGE, PURPLE, WHITE
};

// Buton komutları enum - C# tarafındaki CommandType enum'ına karşılık gelir
enum ButtonCommand {
  NONE = 0,
  VOLUME_UP = 1,
  VOLUME_DOWN = 2, 
  BRIGHTNESS_UP = 3,
  BRIGHTNESS_DOWN = 4,
  OPEN_BROWSER = 5,
  CLOSE_APP = 6,
  LOCK_SCREEN = 7,
  ALT_TAB = 8,
  CUSTOM_ACTION = 9
};

// Her butonun hangi komutu göndereceği
ButtonCommand buttonCommands[9] = {
  VOLUME_DOWN, VOLUME_UP, BRIGHTNESS_DOWN,
  BRIGHTNESS_UP, OPEN_BROWSER, CLOSE_APP,
  LOCK_SCREEN, ALT_TAB, CUSTOM_ACTION
};

// Tüm buton komutları
const char* commandStrings[] = {
  "CMD: NONE", 
  "CMD: VOLUME_UP", 
  "CMD: VOLUME_DOWN", 
  "CMD: BRIGHTNESS_UP", 
  "CMD: BRIGHTNESS_DOWN",
  "CMD: OPEN_BROWSER", 
  "CMD: CLOSE_APP",
  "CMD: LOCK_SCREEN", 
  "CMD: ALT_TAB", 
  "CMD: CUSTOM_ACTION"
};

// Buton etiketleri - bilgisayardan güncellenebilir
const char* defaultButtonLabels[] = {
  "BOS", "SES+", "SES-", "PRLK+", "PRLK-", 
  "GOOGLE", "KAPAT", "KILIT", "ALT+TAB", "OZEL"
};

// Her butonun ekranda gösterilecek etiketi
char buttonLabels[9][10] = {
  "", "", "",
  "", "", "",
  "", "", ""
};

int buttonX[9], buttonY[9];
int buttonWidth, buttonHeight;
bool displayInit = false;

// İleri tanımlamalar (function prototypes)
void loadConfigFromEEPROM();
void saveConfigToEEPROM();
void drawButton(int i, bool pressed);
void drawButtons();
ButtonCommand parseCommandType(String commandStr);

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);  // Bluetooth için

  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  tft.invertDisplay(true); // İnvert özelliğini kapalı tut

  buttonWidth  = (tft.width()  - (BUTTON_COLUMNS + 1) * BUTTON_SPACING) / BUTTON_COLUMNS;
  buttonHeight = (tft.height() - (BUTTON_ROWS    + 1) * BUTTON_SPACING) / BUTTON_ROWS;

  for (int i = 0; i < 9; i++) {
    int row = i / BUTTON_COLUMNS;
    int col = i % BUTTON_COLUMNS;
    buttonX[i] = BUTTON_SPACING + col * (buttonWidth  + BUTTON_SPACING);
    buttonY[i] = BUTTON_SPACING + row * (buttonHeight + BUTTON_SPACING);
  }
  
  // EEPROM'dan yapılandırmayı yükle veya varsayılan değerleri kullan
  loadConfigFromEEPROM();  // Bu fonksiyonu çağırın
  
  // Başlangıç için buton etiketlerini ayarla
  for (int i = 0; i < 9; i++) {
    ButtonCommand cmd = buttonCommands[i];
    if (cmd >= 0 && cmd < 10) {
      strncpy(buttonLabels[i], defaultButtonLabels[cmd], 9);
      buttonLabels[i][9] = '\0'; // NULL terminasyon
    }
  }
  
  drawButtons();
  displayInit = true;
  
  Serial.println(F("Venus Deck başlatıldı!"));
  Serial.println(F("Kullanılabilir komutlar:"));
  Serial.println(F("CFG:ButtonIndex:CommandCode - Buton işlevini değiştirir"));
}

void loop() {
  // Kullanıcı dokunma işlemlerini kontrol et
  checkTouchInput();
  
  // Bilgisayardan gelen komutları kontrol et
  checkSerialCommands();
  
  delay(50);
}

void checkTouchInput() {
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
        // Butonun mevcut komutunu gönder
        ButtonCommand cmd = buttonCommands[i];
        if (cmd >= 0 && cmd < 10) {
          Serial1.println(commandStrings[cmd]);  // Bluetooth ile gönder
          Serial.println(commandStrings[cmd]);   // Debug için konsola da yaz
        }
      }
    }
  }
}

void checkSerialCommands() {
  // Bluetooth'dan (Serial1) gelen komutları kontrol et
  if (Serial1.available() > 0) {
    String input = Serial1.readStringUntil('\n');
    input.trim();
    
    Serial.print("Bluetooth'dan alındı: ");
    Serial.println(input);
    
    checkJsonInput(input);
  }
  
  // Normal seri port (Serial) üzerinden de komutları kontrol et (debug için)
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    Serial.print("Seri porttan alındı: ");
    Serial.println(input);
    
    checkJsonInput(input);
  }
}


void processConfigCommand(String input) {
  // Komutun CFG: ile başladığını kontrol et
  if (!input.startsWith("CFG:")) {
    Serial.println("Geçersiz komut formatı (CFG: bekleniyor)");
    return;
  }

  // İlk ':' işaretinin pozisyonu
  int firstColon = input.indexOf(':');
  if (firstColon < 0) return;

  // İkinci ':' işaretinin pozisyonu
  int secondColon = input.indexOf(':', firstColon + 1);
  if (secondColon < 0) return;

  // Buton indeksi ve komut kodunu çıkar
  int buttonIndex = input.substring(firstColon + 1, secondColon).toInt();
  String commandStr = input.substring(secondColon + 1);

  Serial.print("Buton indeksi: ");
  Serial.print(buttonIndex);
  Serial.print(", Komut: ");
  Serial.println(commandStr);

  // Komut tipini belirle
  ButtonCommand cmd = parseCommandType(commandStr);

  // Geçerli buton indeksi ve komut ise atama yap
  if (buttonIndex >= 1 && buttonIndex <= 9 && cmd >= 0 && cmd <= 9) {
    buttonIndex--; // 0-bazlı indekse dönüştür

    // Buton komutunu güncelle
    buttonCommands[buttonIndex] = cmd;

    // Buton etiketini güncelle
    if (cmd > 0) {
      // defaultButtonLabels'taki ilgili etiketi kullan
      strncpy(buttonLabels[buttonIndex], defaultButtonLabels[cmd], 9);
    } else {
      // cmd == 0 ise boş etiket
      buttonLabels[buttonIndex][0] = '\0';
    }
    buttonLabels[buttonIndex][9] = '\0'; // NULL terminasyon

    // Ekranı güncelle
    drawButton(buttonIndex, false);

    Serial.print(F("Buton ")); Serial.print(buttonIndex + 1);
    Serial.print(F(" güncellendi: ")); Serial.println(commandStrings[cmd]);

    // Onay gönder
    Serial1.print("OK:"); Serial1.print(buttonIndex + 1); Serial1.print(":"); Serial1.println(cmd);
  } else {
    Serial.println("Geçersiz buton indeksi veya komut");
  }
}

// String komut adını enum değerine çevirir
ButtonCommand parseCommandType(String commandStr) {
  // CommandType enum adını doğrudan kontrol et
  if (commandStr == "None") return NONE;
  else if (commandStr == "VolumeUp") return VOLUME_UP;
  else if (commandStr == "VolumeDown") return VOLUME_DOWN;
  else if (commandStr == "BrightnessUp") return BRIGHTNESS_UP;
  else if (commandStr == "BrightnessDown") return BRIGHTNESS_DOWN;
  else if (commandStr == "OpenBrowser") return OPEN_BROWSER;
  else if (commandStr == "CloseApp") return CLOSE_APP;
  else if (commandStr == "LockScreen") return LOCK_SCREEN;
  else if (commandStr == "AltTab") return ALT_TAB;
  else if (commandStr == "CustomAction") return CUSTOM_ACTION;
  
  // Sayısal değer kontrolü
  int value = commandStr.toInt();
  if (value >= 0 && value <= 9) {
    return (ButtonCommand)value;
  }
  
  // Hiçbiri değilse None döndür
  Serial.print("Bilinmeyen komut tipi: ");
  Serial.println(commandStr);
  return NONE;
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

  if (strlen(buttonLabels[i]) > 0) {
    tft.setTextColor(fg);
    tft.setTextSize(2);

    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(buttonLabels[i], 0, 0, &x1, &y1, &w, &h);

    int tx = buttonX[i] + (buttonWidth - w) / 2;
    int ty = buttonY[i] + (buttonHeight + h) / 2 - 5; // Metni dikey olarak wortalamak için

    tft.setCursor(tx, ty);
    tft.print(buttonLabels[i]);
  }
}

void processJsonConfig(String jsonStr) {
  // ArduinoJson için bellek ayır
  StaticJsonDocument<512> doc;

  // JSON'ı ayrıştır
  DeserializationError error = deserializeJson(doc, jsonStr);

  // Ayrıştırma hatasını kontrol et
  if (error) {
    Serial.print(F("JSON ayrıştırma hatası: "));
    Serial.println(error.c_str());
    return;
  }

  // Her buton için değerleri işle
  for (int i = 1; i <= 9; i++) {
    String buttonKey = "Button" + String(i);
    if (doc.containsKey(buttonKey)) {
      String commandStr = doc[buttonKey].as<String>();
      Serial.print(buttonKey);
      Serial.print(": ");
      Serial.println(commandStr);

      // Komutu işle - CFG:i:commandStr formatında
      String configCmd = "CFG:" + String(i) + ":" + commandStr;
      processConfigCommand(configCmd);
    }
  }

  // İşlem tamamlandı mesajı
  Serial.println("JSON ayarları işlendi");
  Serial1.println("OK:Config:Applied");
}


void loadConfigFromEEPROM() {
  // EEPROM'un ilk byte'ını kontrol et
  byte magicNumber = EEPROM.read(0);
  
  if (magicNumber != EEPROM_MAGIC_NUMBER) {
    // Geçerli bir yapılandırma yok, varsayılanları kullan
    Serial.println(F("Geçerli EEPROM yapılandırması bulunamadı, varsayılanlar kullanılıyor"));
    
    // Varsayılan buton komutları (yukarıda zaten tanımlanmış)
    return;
  }
  
  // EEPROM'dan buton komutlarını oku
  for (int i = 0; i < 9; i++) {
    byte cmd = EEPROM.read(EEPROM_START_ADDR + i);
    if (cmd < 10) { // Geçerli bir komut kodu olduğundan emin ol
      buttonCommands[i] = (ButtonCommand)cmd;
    }
  }
  
  Serial.println(F("EEPROM'dan yapılandırma yüklendi"));
}

void saveConfigToEEPROM() {
  // Magic number'ı ayarla
  EEPROM.write(0, EEPROM_MAGIC_NUMBER);
  
  // Buton komutlarını kaydet
  for (int i = 0; i < 9; i++) {
    EEPROM.write(EEPROM_START_ADDR + i, (byte)buttonCommands[i]);
  }
  
  Serial.println(F("Yapılandırma EEPROM'a kaydedildi"));
}


void checkJsonInput(String input) {
  // JSON formatında mı kontrol et (ilk karakter '{' olmalı)
  if (input.startsWith("{")) {
    Serial.println(F("JSON konfigurasyonu alındı"));
    processJsonConfig(input);
    return;
  }
  
  // Normal komut formatı (CFG:...)
  if (input.startsWith("CFG:")) {
    processConfigCommand(input);
    return;
  }
  
  Serial.print(F("Bilinmeyen format: "));
  Serial.println(input);
}