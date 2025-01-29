#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

// -------- تنظیمات NTP برای زمان دقیق --------
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 12600; // UTC+3:30 تهران
const int DAYLIGHT_OFFSET_SEC = 0;

// -------- پین‌های سخت‌افزار --------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

#define BACKLIGHT_PIN 32
#define TOUCH_THRESHOLD 15
#define SCROLL_THRESHOLD 20

// -------- رنگ‌های رابط کاربری --------
const uint16_t COLORS[2][6] = {
  {0x18C3, 0x4A69, 0x03FF, 0x10A2, 0xFFFF, 0x07FF}, // Light Mode
  {0x0020, 0x1808, 0x03EF, 0x0000, 0xFFFF, 0x02DF}  // Dark Mode
};

// -------- ساختار برنامه‌ها --------
typedef struct {
  const char* label;
  void (*launch)();
  void (*update)();
} App;

class TouchManager {
  private:
    int16_t lastX = -1, lastY = -1;
    unsigned long lastTime = 0;
    
  public:
    bool getTouch(int16_t &x, int16_t &y) {
      if (!touch.touched()) return false;
      TS_Point p = touch.getPoint();

      // تصحیح مقیاس صفحه لمسی (بر اساس تصویر شما)
      x = map(p.x, 300, 3800, 0, 240);
      y = map(p.y, 300, 3800, 320, 0);

      // فیلتر نویز
      if(abs(x - lastX) < TOUCH_THRESHOLD && abs(y - lastY) < TOUCH_THRESHOLD &&
         millis() - lastTime < 150) return false;

      lastX = x; lastY = y; lastTime = millis();
      return true;
    }
};

// -------- متغیرهای سیستمی --------
SPIClass vspi(VSPI);
SPIClass hspi(HSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
TouchManager touchManager;

bool darkMode = false;
uint8_t currentPage = 0;
int wifiScrollOffset = 0;

const App apps[] = {
  {"WiFi", launchWiFi, updateWiFi},
  {"Tools", launchTools, nullptr},
  {"Notes", launchNotes, nullptr}
};
const int APPS_COUNT = sizeof(apps) / sizeof(App);

// -------- راه‌اندازی سیستم --------
void setup() {
  Serial.begin(115200);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  analogWrite(BACKLIGHT_PIN, 180);

  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(3);  // تغییر جهت نمایش برای تصحیح نمایشگر
  tft.fillScreen(getColor(3)); // پس‌زمینه

  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  drawHomeScreen();
}

void loop() {
  int16_t touchX, touchY;
  if(touchManager.getTouch(touchX, touchY)) handleUI(touchX, touchY);
}

// -------- مدیریت رنگ‌ها --------
uint16_t getColor(uint8_t index) {
  return COLORS[darkMode][index];
}

// -------- نمایش صفحه اصلی --------
void drawHomeScreen() {
  tft.fillScreen(getColor(3));
  drawStatusBar();
  drawAppGrid();
}

void drawStatusBar() {
  tft.fillRect(0, 0, 240, 30, getColor(0));
  drawText("ESP32 Phone", 10, 8, 4);
  
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, 6, "%H:%M", &timeinfo);
    drawText(timeStr, 180, 8, 4);
  }
}

void drawAppGrid() {
  for(uint8_t i=0; i<3; i++) {
    if(i >= APPS_COUNT) break;
    drawAppIcon(apps[i].label, (i%3)*80+20, 80);
  }
}

void drawAppIcon(const char* label, int x, int y) {
  tft.fillRoundRect(x, y, 80, 80, 10, getColor(1));
  drawText(label, x+15, y+90, 4);
}

// -------- مدیریت تاچ --------
void handleUI(int16_t x, int16_t y) {
  if(y > 30 && y < 280) handleAppSelection(x, y);
}

void handleAppSelection(int16_t x, int16_t y) {
  for(uint8_t i=0; i<3; i++) {
    if(i >= APPS_COUNT) break;
    int ax = (i%3)*80+20;
    if(x > ax && x < ax+80 && y > 80 && y < 160) {
      animateIcon(ax, 80);
      apps[i].launch();
    }
  }
}

void animateIcon(int x, int y) {
  tft.fillRoundRect(x, y, 80, 80, 10, getColor(2));
  delay(100);
  tft.fillRoundRect(x, y, 80, 80, 10, getColor(1));
}

// -------- اپلیکیشن WiFi --------
void launchWiFi() {
  tft.fillScreen(getColor(3));
  drawStatusBar();
  scanWiFiNetworks();
}

void updateWiFi() {
  int16_t x, y;
  if(touchManager.getTouch(x, y)) {
    wifiScrollOffset += (y - 100) / 10;
    wifiScrollOffset = constrain(wifiScrollOffset, 0, 150);
    drawWiFiList();
  }
}

void drawWiFiList() {
  tft.fillRect(0, 30, 240, 250, getColor(3));
  for(int i=0; i<5; i++) {
    int yPos = 50 + i*50 - wifiScrollOffset;
    if(yPos > 30 && yPos < 280) drawText("WiFi Network", 20, yPos, 5);
  }
}

// -------- توابع کمکی --------
void drawText(const char* text, int x, int y, uint8_t color) {
  tft.setTextColor(getColor(color));
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(text);
}
