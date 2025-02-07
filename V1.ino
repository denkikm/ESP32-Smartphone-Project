#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>   // برای HTTPS
#include <HTTPClient.h>
#include <ArduinoJson.h>
#ifdef ESP32
  #include "esp_system.h"       // اطلاعات سیستم
#endif

// ------------------ پیکربندی پین‌ها ------------------
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
#define TFT_SCLK   18
#define TFT_MOSI   23

#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

#define BACKLIGHT_PIN    32

// ------------------ ثابت‌های طراحی ------------------
#define PRIMARY_COLOR    0x7B9F   // نوار وضعیت و نوار پایین
#define SECONDARY_COLOR  0x9CFB   // کادرهای اطلاعات
#define ACCENT_COLOR     0x03FF   // دکمه‌ها
#define BACKGROUND_COLOR 0xEF5D   // پس‌زمینه اصلی
#define TEXT_COLOR       0x0000   // متن (سیاه)
#define WIFI_ICON_COLOR  0x07FF   // آیکون وای‌فای
#define DARK_BACKGROUND  0x0000   // پس‌زمینه حالت تیره
#define DARK_TEXT        0xFFFF   // متن حالت تیره

// ------------------ اشیاء صفحه نمایش و تاچ ------------------
SPIClass vspi(VSPI);
SPIClass hspi(HSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ------------------ اعلان توابع ------------------
void launchWiFiApp();
void launchNotesApp();
void launchCalcApp();
void launchExploreApp();
void launchToolsApp();
void launchSystemApp();
void launchSettingsApp();
void launchChatApp();
void launchApp(String appName);

void drawStatusBar();
void drawBottomNavBar();
void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);
uint16_t darkenColor(uint8_t color, uint8_t percent);  // تابع نمونه (در این نسخه ثابت برگردانده می‌شود)
String getCurrentTime();
void drawAppIcons();
void drawPage(int page);
String inputText(String prompt);
String urlencode(String str);
void deepSearch();

// ------------------ متغیرهای کلیدی ------------------
int currentPage = 0;         // 0: صفحه اصلی (Home)، 1: صفحات دیگر (به صورت نمونه)
bool darkMode = false;
bool inApp = false;          // وقتی در یک اپ هستیم، لمس‌های صفحه اصلی نادیده گرفته می‌شوند

// تنظیمات صفحه اصلی: ۸ اپ با برچسب‌های زیر
const int totalApps = 8;
String appLabels[totalApps] = {
  "WiFi", "Notes", "Calc", "Explore",
  "Tools", "System", "Settings", "Chat"
};

// سایر متغیرهای اپ‌ها
String notes[10];
int noteIndex = 0;
String calcExpr = "";
String selectedSSID = "";

// ساعت (شبیه‌سازی)
unsigned long previousMillis = 0;
unsigned long interval = 1000; // 1 ثانیه

// تنظیم موقعیت اپ‌های صفحه اصلی (بدون اسکرول)
const int numCols = 2;
const int iconWidth = 50;
const int iconHeight = 50;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  String label;
} AppIcon;

AppIcon apps[totalApps];

void initAppIcons() {
  for (int i = 0; i < totalApps; i++) {
    int col = i % numCols;
    int row = i / numCols;
    apps[i].x = 10 + col * (iconWidth + 20);
    apps[i].y = 40 + row * (iconHeight + 20);
    apps[i].width = iconWidth;
    apps[i].height = iconHeight;
    apps[i].label = appLabels[i];
  }
}

// ------------------ توابع UI ------------------

void drawStatusBar() {
  tft.fillRect(0, 0, 240, 30, darkMode ? DARK_BACKGROUND : PRIMARY_COLOR);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.setTextSize(1);
  tft.setCursor(10, 8);
  tft.print("ESP32 Phone v1.0");
  tft.fillCircle(210, 15, 8, WIFI_ICON_COLOR);
  tft.drawCircle(210, 15, 10, darkMode ? DARK_BACKGROUND : 0x0000);
  String timeStr = getCurrentTime();
  tft.setCursor(160, 8);
  tft.print(timeStr);
}

void drawBottomNavBar() {
  int barY = 300;
  tft.fillRect(0, barY, 240, 20, darkMode ? DARK_BACKGROUND : PRIMARY_COLOR);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(10, barY + 2);
  tft.print("Home");
  tft.setCursor(130, barY + 2);
  tft.print("Back");
}

void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, radius, color);
  tft.drawRoundRect(x, y, w, h, radius, darkMode ? DARK_BACKGROUND : 0x0000);
}

uint16_t darkenColor(uint8_t color, uint8_t percent) {
  return color; // در این نسخه ثابت است
}

String getCurrentTime() {
  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - previousMillis;
  if (elapsed >= interval) {
    previousMillis = currentMillis;
  }
  int hrs = (elapsed / 3600000) % 24;
  int mins = (elapsed / 60000) % 60;
  int secs = (elapsed / 1000) % 60;
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", hrs, mins, secs);
  return String(buf);
}

void drawAppIcons() {
  for (int i = 0; i < totalApps; i++) {
    int x = apps[i].x;
    int y = apps[i].y;
    drawRoundedRect(x, y, apps[i].width, apps[i].height, 10,
                    darkMode ? DARK_BACKGROUND : SECONDARY_COLOR);
    tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
    tft.setTextSize(1);
    tft.setCursor(x + 5, y + apps[i].height + 2);
    tft.print(apps[i].label);
  }
}

void drawPage(int page) {
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  if (page == 0) {
    drawAppIcons();
  } else if (page == 1) {
    tft.setTextSize(2);
    tft.setCursor(40, 100);
    tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
    tft.print("Notes App");
  } else if (page == 2) {
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    tft.setTextSize(2);
    tft.setCursor(40, 80);
    tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
    tft.print("WiFi App");
  }
  drawBottomNavBar();
}

// ------------------ تابع دریافت متن (کیبورد مجازی) ------------------
String inputText(String prompt) {
  String inputStr = "";
  // کیبورد در پایین صفحه از ردیف 150
  bool letterMode = true;
  bool shiftOn = false;
  
  const int letterRows = 3, letterCols = 10;
  String letters[letterRows][letterCols] = {
    {"Q","W","E","R","T","Y","U","I","O","P"},
    {"A","S","D","F","G","H","J","K","L",";"},
    {"Z","X","C","V","B","N","M",",",".","/"}
  };
  const int numberRows = 3, numberCols = 10;
  String numbers[numberRows][numberCols] = {
    {"1","2","3","4","5","6","7","8","9","0"},
    {"-","/","(",")","$","&","@","#","*","="},
    {".",",","?","!","'","%","^","~","_","<"}
  };
  
  const int keyW = 24, keyH = 20, gap = 2;
  const int startX = 0, startY = 150;
  const int specialY = startY + ((letterMode ? letterRows : numberRows) * (keyH + gap)) + 5;
  const int specialKeyW = 60, specialKeyH = 30;
  
  while (true) {
    // نمایش ورودی در بالای صفحه (ثابت)
    tft.fillRect(10, 70, 220, 20, SECONDARY_COLOR);
    tft.setTextSize(2);
    tft.setCursor(15, 75);
    tft.setTextColor(TEXT_COLOR);
    tft.print(inputStr);
    
    // رسم کیبورد در پایین صفحه
    if (letterMode) {
      for (int r = 0; r < letterRows; r++) {
        for (int c = 0; c < letterCols; c++) {
          int x = startX + c * (keyW + gap);
          int y = startY + r * (keyH + gap);
          String key = letters[r][c];
          if (!shiftOn) key.toLowerCase();
          drawRoundedRect(x, y, keyW, keyH, 3, SECONDARY_COLOR);
          tft.setTextSize(2);
          tft.setTextColor(TEXT_COLOR);
          int16_t textX = x + (keyW - (key.length() * 12)) / 2;
          int16_t textY = y + (keyH - 16) / 2;
          tft.setCursor(textX, textY);
          tft.print(key);
        }
      }
    } else {
      for (int r = 0; r < numberRows; r++) {
        for (int c = 0; c < numberCols; c++) {
          int x = startX + c * (keyW + gap);
          int y = startY + r * (keyH + gap);
          String key = numbers[r][c];
          drawRoundedRect(x, y, keyW, keyH, 3, SECONDARY_COLOR);
          tft.setTextSize(2);
          tft.setTextColor(TEXT_COLOR);
          int16_t textX = x + (keyW - (key.length() * 12)) / 2;
          int16_t textY = y + (keyH - 16) / 2;
          tft.setCursor(textX, textY);
          tft.print(key);
        }
      }
    }
    
    // رسم دکمه‌های ویژه
    int modeKeyX = 10;
    drawRoundedRect(modeKeyX, specialY, specialKeyW, specialKeyH, 5, ACCENT_COLOR);
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(modeKeyX + 5, specialY + 8);
    tft.print("Mode");
    
    if (letterMode) {
      int shiftKeyX = 80;
      drawRoundedRect(shiftKeyX, specialY, specialKeyW, specialKeyH, 5, ACCENT_COLOR);
      tft.setTextSize(2);
      tft.setTextColor(TEXT_COLOR);
      tft.setCursor(shiftKeyX + 5, specialY + 8);
      tft.print("Shift");
    }
    
    int delKeyX = letterMode ? 150 : 80;
    drawRoundedRect(delKeyX, specialY, specialKeyW, specialKeyH, 5, ACCENT_COLOR);
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(delKeyX + 5, specialY + 8);
    tft.print("Del");
    
    int enterKeyX = letterMode ? 220 : 150;
    drawRoundedRect(enterKeyX, specialY, specialKeyW, specialKeyH, 5, ACCENT_COLOR);
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(enterKeyX + 2, specialY + 8);
    tft.print("Ent");
    
    bool keyPressed = false;
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      
      if (ty >= startY && ty < startY + (letterMode ? letterRows : numberRows) * (keyH + gap)) {
        int col = tx / (keyW + gap);
        int row = (ty - startY) / (keyH + gap);
        if (letterMode && row < letterRows && col < letterCols) {
          String key = letters[row][col];
          if (!shiftOn) key.toLowerCase();
          inputStr += key;
          keyPressed = true;
        } else if (!letterMode && row < numberRows && col < numberCols) {
          inputStr += numbers[row][col];
          keyPressed = true;
        }
      }
      
      if (ty >= specialY && ty < specialY + specialKeyH) {
        if (tx >= modeKeyX && tx < modeKeyX + specialKeyW) {
          letterMode = !letterMode;
          keyPressed = true;
          delay(300);
        } else if (letterMode && tx >= 80 && tx < 80 + specialKeyW) {
          shiftOn = !shiftOn;
          keyPressed = true;
          delay(300);
        } else if (tx >= delKeyX && tx < delKeyX + specialKeyW) {
          if (inputStr.length() > 0)
            inputStr.remove(inputStr.length() - 1);
          keyPressed = true;
          delay(300);
        } else if (tx >= enterKeyX && tx < enterKeyX + specialKeyW) {
          keyPressed = true;
          break;
        }
      }
      
      if (keyPressed) {
        tft.fillRect(10, 70, 220, 20, SECONDARY_COLOR);
        tft.setTextSize(2);
        tft.setCursor(15, 75);
        tft.setTextColor(TEXT_COLOR);
        tft.print(inputStr);
        delay(150);
      }
    }
    delay(100);
  }
  
  return inputStr;
}

// ------------------ تابع URL encode ------------------
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (size_t i = 0; i < str.length(); i++){
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      if (code0 > 9) code0 = 'A' + (code0 - 10);
      else code0 = '0' + code0;
      if (code1 > 9) code1 = 'A' + (code1 - 10);
      else code1 = '0' + code1;
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

// ------------------ تابع DeepSearch با API Deepseek ------------------
void deepSearch() {
  String query = inputText("Search Query:");
  String encodedQuery = urlencode(query);
  String url = "https://api.deepseek.com/v1/search?query=" + encodedQuery;
  
  WiFiClientSecure client;
  client.setInsecure(); // برای تست؛ در پروژه‌های واقعی بهتر است گواهی‌ها را اعتبارسنجی کنید
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  String apiKey = "sk-5b095ce73531413a9e80272a28a285dd";
  http.addHeader("Authorization", "Bearer " + apiKey);
  
  int httpCode = http.GET();
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      const size_t capacity = 2048;
      DynamicJsonDocument doc(capacity);
      DeserializationError error = deserializeJson(doc, payload);
      tft.setTextSize(1);
      int yPos = 40;
      if (!error) {
        JsonArray results = doc["results"].as<JsonArray>();
        for (JsonObject result : results) {
          const char* title = result["title"];
          tft.setCursor(10, yPos);
          tft.print(title);
          yPos += 12;
          if (yPos > 280) break;
        }
      } else {
        tft.setTextSize(2);
        tft.setCursor(10, 80);
        tft.print("JSON Error");
      }
    } else {
      tft.setTextSize(2);
      tft.setCursor(10, 80);
      tft.print("HTTP Error");
    }
  } else {
    tft.setTextSize(2);
    tft.setCursor(10, 80);
    tft.print("Request Failed");
  }
  http.end();
  
  // دکمه Back
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.setTextColor(TEXT_COLOR);
  tft.print("Back");
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 10 && tx <= 110 && ty >= 270 && ty <= 295)
        break;
    }
    delay(100);
  }
  launchWiFiApp();
}

// ------------------ عملکرد اپ‌ها اختصاصی ------------------

void launchWiFiApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(50, 60);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("WiFi App");
  // دو دکمه: Search و DeepSearch
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.print("Search");
  drawRoundedRect(130, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(140, 275);
  tft.print("DeepSearch");
  drawBottomNavBar();
  
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      // اگر دکمه Search لمس شد
      if (tx >= 10 && tx < 110 && ty >= 270 && ty < 295)
        break;
      // اگر دکمه DeepSearch لمس شد
      if (tx >= 130 && tx < 230 && ty >= 270 && ty < 295) {
        deepSearch();
        return;
      }
    }
    delay(100);
  }
  
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.print("Scanning WiFi...");
  delay(100);
  
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) {
    tft.setCursor(10, 80);
    tft.print("No networks found!");
    delay(2000);
  } else {
    int indices[numNetworks];
    for (int i = 0; i < numNetworks; i++)
      indices[i] = i;
    for (int i = 0; i < numNetworks - 1; i++) {
      for (int j = i + 1; j < numNetworks; j++) {
        if (WiFi.RSSI(indices[i]) < WiFi.RSSI(indices[j])) {
          int temp = indices[i];
          indices[i] = indices[j];
          indices[j] = temp;
        }
      }
    }
    
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    int yPos = 40;
    const int boxX = 10, boxW = 220, boxH = 35;
    tft.setTextSize(1);
    for (int i = 0; i < numNetworks && i < 10; i++) {
      String ssid = WiFi.SSID(indices[i]);
      int rssi = WiFi.RSSI(indices[i]);
      drawRoundedRect(boxX, yPos, boxW, boxH, 5, SECONDARY_COLOR);
      String networkInfo = ssid + " (" + String(rssi) + ")";
      int16_t textWidth = networkInfo.length() * 6;
      int16_t xText = boxX + (boxW - textWidth) / 2;
      int16_t yText = yPos + (boxH - 8) / 2;
      tft.setCursor(xText, yText);
      tft.setTextColor(TEXT_COLOR);
      tft.print(networkInfo);
      yPos += boxH + 5;
    }
    
    int selectedBox = -1;
    while (selectedBox == -1) {
      if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int16_t tx = map(p.y, 3750, 250, 0, 240);
        int16_t ty = map(p.x, 250, 3750, 0, 320);
        const int boxX = 10, boxW = 220, boxH = 35;
        for (int i = 0; i < 10 && i < numNetworks; i++) {
          int boxY = 40 + i * (boxH + 5);
          if (tx >= boxX && tx <= boxX + boxW && ty >= boxY && ty <= boxY + boxH) {
            selectedBox = i;
            break;
          }
        }
      }
      delay(100);
    }
    
    selectedSSID = WiFi.SSID(indices[selectedBox]);
    String password = inputText("Enter Password:");
    
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    tft.setTextSize(2);
    tft.setCursor(10, 80);
    tft.print("Connecting to:");
    tft.setCursor(10, 110);
    tft.print(selectedSSID);
    WiFi.begin(selectedSSID.c_str(), password.c_str());
    unsigned long connStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - connStart < 10000)
      delay(500);
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    tft.setTextSize(2);
    tft.setCursor(10, 80);
    if (WiFi.status() == WL_CONNECTED)
      tft.print("Connected!");
    else
      tft.print("Failed to Connect!");
    delay(3000);
  }
  
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.print("Back");
  drawBottomNavBar();
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchNotesApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(40, 40);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("Notes App");
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.print("New Note");
  drawBottomNavBar();
  
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx > 10 && tx < 110 && ty > 270 && ty < 295) {
        if (noteIndex < 10) {
          String noteText = inputText("New Note:");
          notes[noteIndex++] = noteText;
        }
        break;
      }
    }
    delay(100);
  }
  
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.print("Your Notes:");
  tft.setTextSize(1);
  int yPos = 60;
  for (int i = 0; i < noteIndex; i++) {
    tft.setCursor(10, yPos);
    tft.print(notes[i]);
    yPos += 12;
  }
  
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.print("Back");
  drawBottomNavBar();
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchCalcApp() {
  inApp = true;
  calcExpr = "";
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.fillRect(10, 40, 220, 30, SECONDARY_COLOR);
  tft.setTextSize(2);
  tft.setCursor(15, 45);
  tft.setTextColor(TEXT_COLOR);
  tft.print(calcExpr);
  
  const uint16_t keyW = 60, keyH = 50;
  const uint16_t startX = 0, startY = 80;
  String keys[4][4] = {
    {"7", "8", "9", "+"},
    {"4", "5", "6", "-"},
    {"1", "2", "3", "*"},
    {"0", "C", "=", "/"}
  };
  for (uint8_t r = 0; r < 4; r++) {
    for (uint8_t c = 0; c < 4; c++) {
      uint16_t x = startX + c * keyW;
      uint16_t y = startY + r * keyH;
      drawRoundedRect(x, y, keyW, keyH, 5, SECONDARY_COLOR);
      tft.setTextSize(2);
      int16_t textX = x + (keyW - (keys[r][c].length() * 12)) / 2;
      int16_t textY = y + (keyH - 16) / 2;
      tft.setCursor(textX, textY);
      tft.setTextColor(TEXT_COLOR);
      tft.print(keys[r][c]);
    }
  }
  
  drawBottomNavBar();
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (ty >= startY && ty < startY + 4 * keyH) {
        uint8_t col = tx / keyW;
        uint8_t row = (ty - startY) / keyH;
        if (row < 4 && col < 4) {
          String key = keys[row][col];
          if (key == "C") {
            calcExpr = "";
          } else if (key == "=") {
            int opIndex = -1;
            char op;
            for (int i = 0; i < calcExpr.length(); i++) {
              char c = calcExpr.charAt(i);
              if (c == '+' || c == '-' || c == '*' || c == '/') {
                opIndex = i;
                op = c;
                break;
              }
            }
            if (opIndex > 0 && opIndex < calcExpr.length()-1) {
              int a = calcExpr.substring(0, opIndex).toInt();
              int b = calcExpr.substring(opIndex+1).toInt();
              int res = 0;
              if (op == '+') res = a + b;
              else if (op == '-') res = a - b;
              else if (op == '*') res = a * b;
              else if (op == '/') {
                if (b != 0) res = a / b;
                else { calcExpr = "Error"; goto UpdateCalcDisplay; }
              }
              calcExpr = String(res);
            } else {
              calcExpr = "Err";
            }
          } else {
            calcExpr += key;
          }
        }
      }
UpdateCalcDisplay:
      tft.fillRect(10, 40, 220, 30, SECONDARY_COLOR);
      tft.setTextSize(2);
      tft.setCursor(15, 45);
      tft.setTextColor(TEXT_COLOR);
      tft.print(calcExpr);
      if (ty >= 300 && ty <= 320 && tx >= 130 && tx <= 240)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchToolsApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(20, 50);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("Contacts");
  tft.setCursor(20, 100);
  tft.print("Messages");
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.print("Back");
  drawBottomNavBar();
  
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchSystemApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  unsigned long uptime = millis() / 1000;
#ifdef ESP32
  uint32_t freeHeap = ESP.getFreeHeap();
#else
  uint32_t freeHeap = 0;
#endif
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("System Info:");
  tft.setTextSize(1);
  tft.setCursor(10, 70);
  tft.print("Uptime: " + String(uptime) + " s");
  tft.setCursor(10, 90);
  tft.print("Free Heap: " + String(freeHeap));
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.setTextColor(TEXT_COLOR);
  tft.print("Back");
  drawBottomNavBar();
  
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchSettingsApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("Settings App");
  tft.setTextSize(1);
  tft.setCursor(10, 80);
  tft.print("Toggle Dark Mode");
  drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(20, 275);
  tft.setTextColor(TEXT_COLOR);
  tft.print("Back");
  drawBottomNavBar();
  
  while (true) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int16_t tx = map(p.y, 3750, 250, 0, 240);
      int16_t ty = map(p.x, 250, 3750, 0, 320);
      if (tx >= 10 && tx < 110 && ty >= 80 && ty < 110) {
        darkMode = !darkMode;
        break;
      }
      if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
        break;
    }
    delay(100);
  }
  
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchExploreApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("Explore App");
  delay(3000);
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void launchChatApp() {
  inApp = true;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.print("Chat App");
  delay(3000);
  inApp = false;
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

// ------------------ تابع اجرای اپ‌ها کلی ------------------
void launchApp(String appName) {
  if (appName == "WiFi") {
    launchWiFiApp();
  } else if (appName == "Notes") {
    launchNotesApp();
  } else if (appName == "Calc") {
    launchCalcApp();
  } else if (appName == "Explore") {
    launchExploreApp();
  } else if (appName == "Tools") {
    launchToolsApp();
  } else if (appName == "System") {
    launchSystemApp();
  } else if (appName == "Settings") {
    launchSettingsApp();
  } else if (appName == "Chat") {
    launchChatApp();
  } else {
    inApp = true;
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    tft.setTextSize(2);
    tft.setCursor(40, 60);
    tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
    tft.print(appName + " App");
    drawRoundedRect(10, 270, 100, 25, 8, ACCENT_COLOR);
    tft.setTextSize(2);
    tft.setCursor(20, 275);
    tft.setTextColor(TEXT_COLOR);
    tft.print("Back");
    while (true) {
      if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int16_t tx = map(p.y, 3750, 250, 0, 240);
        int16_t ty = map(p.x, 250, 3750, 0, 320);
        if (tx >= 120 && tx < 240 && ty >= 300 && ty <= 320)
          break;
      }
      delay(100);
    }
    inApp = false;
    tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
    drawStatusBar();
    drawAppIcons();
    drawBottomNavBar();
  }
}

// ------------------ Setup & Loop ------------------
void setup() {
  Serial.begin(115200);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  
  WiFi.mode(WIFI_STA);
  
  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(2);
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  tft.invertDisplay(true);
  
  // راه‌اندازی صفحه لمسی
  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);
  
  initAppIcons();
  drawStatusBar();
  drawAppIcons();
  drawBottomNavBar();
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    int16_t tx = map(p.y, 3750, 250, 0, 240);
    int16_t ty = map(p.x, 250, 3750, 0, 320);
    // بررسی لمس نوار پایین
    if (ty >= 300 && ty <= 320) {
      if (tx < 120) {
        inApp = false;
        currentPage = 0;
        drawPage(0);
        delay(200);
        return;
      } else {
        if (inApp) {
          inApp = false;
          currentPage = 0;
          drawPage(0);
          delay(200);
          return;
        }
      }
    }
  }
  
  if (inApp)
    return;
  
  static int16_t touchStartX = 0, touchStartY = 0;
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    int16_t x = map(p.y, 3750, 250, 0, 240);
    int16_t y = map(p.x, 250, 3750, 0, 320);
    
    // در این نسخه صفحه اصلی ثابت است (بدون اسکرول)
    static int16_t lastX = 0;
    if (lastX == 0)
      lastX = x;
    else {
      if (abs(lastX - x) > 30) {
        // به عنوان نمونه تغییر صفحه (در اپ‌های دیگر) انجام می‌شود
        currentPage = (currentPage + 1) % 3;
        drawPage(currentPage);
      }
      lastX = 0;
    }
    
    // لمس روی اپ‌های صفحه اصلی
    for (int i = 0; i < totalApps; i++) {
      int iconX = apps[i].x;
      int iconY = apps[i].y; // بدون اسکرول
      if (x > iconX && x < iconX + apps[i].width &&
          y > iconY && y < iconY + apps[i].height) {
        drawRoundedRect(iconX, iconY, apps[i].width, apps[i].height, 15, ACCENT_COLOR);
        delay(100);
        drawRoundedRect(iconX, iconY, apps[i].width, apps[i].height, 15,
                        darkMode ? DARK_BACKGROUND : SECONDARY_COLOR);
        launchApp(appLabels[i]);
      }
    }
  }
  
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    drawStatusBar();
    lastUpdate = millis();
  }
}
