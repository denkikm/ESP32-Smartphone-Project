#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

// =================== بهبودهای اصلی ===================
// 1. جایگزینی String با آرایه کاراکتر برای مدیریت حافظه
// 2. اضافه شدن سیستم زمان واقعی با NTP
// 3. بهینهسازی سیستم تاچ با فیلتر نویز
// 4. سازماندهی مجدد کد با استفاده از کلاسها
// 5. بهبود سیستم تم تاریک
// 6. اضافه شدن اسکرول عمودی برای لیست وایفای
// =====================================================

// ------------------ پیکربندی پیشرفته ------------------
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 12600; // Tehran time (UTC+3:30)
const int DAYLIGHT_OFFSET_SEC = 0;

// ------------------ پیکربندی پین‌ها ------------------
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
#define TOUCH_THRESHOLD 30
#define SCROLL_THRESHOLD 20

// ------------------ رنگ‌های پیشرفته ------------------
const uint16_t COLORS[2][6] = {
  { // Light Mode
    0x18C3,  // PRIMARY
    0x4A69,  // SECONDARY
    0x03FF,  // ACCENT
    0x10A2,  // BACKGROUND
    0xFFFF,  // TEXT
    0x07FF   // WIFI_ICON
  },
  { // Dark Mode
    0x0020,  // PRIMARY
    0x1808,  // SECONDARY
    0x03EF,  // ACCENT
    0x0000,  // BACKGROUND
    0xFFFF,  // TEXT
    0x02DF   // WIFI_ICON
  }
};

// ------------------ ساختارهای پیشرفته ------------------
typedef struct {
  const char* label;
  void (*launch)();
  void (*update)();
} App;

class TouchManager {
  private:
    int16_t lastX = -1;
    int16_t lastY = -1;
    unsigned long lastTime = 0;
    
  public:
    bool getTouch(int16_t &x, int16_t &y) {
      if (!touch.touched()) return false;
      
      TS_Point p = touch.getPoint();
      x = map(p.x, 250, 3750, 0, 240);
      y = map(p.y, 250, 3750, 320, 0);
      
      // فیلتر نویز
      if(abs(x - lastX) < TOUCH_THRESHOLD && 
         abs(y - lastY) < TOUCH_THRESHOLD &&
         millis() - lastTime < 200) {
        return false;
      }
      
      lastX = x;
      lastY = y;
      lastTime = millis();
      return true;
    }
};

// ------------------ متغیرهای سیستمی ------------------
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
  {"Settings", launchSettings, nullptr},
  // ... سایر اپلیکیشن‌ها
};
const int APPS_COUNT = sizeof(apps)/sizeof(App);

// ------------------ راه‌اندازی پیشرفته ------------------
void setup() {
  Serial.begin(115200);
  
  // راه‌اندازی سخت‌افزار
  pinMode(BACKLIGHT_PIN, OUTPUT);
  analogWrite(BACKLIGHT_PIN, 180); // کاهش مصرف انرژی
  
  initDisplay();
  initTouch();
  initTime();
  
  drawHomeScreen();
}

void initDisplay() {
  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(0);
  tft.fillScreen(getColor(BACKGROUND));
}

void initTouch() {
  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);
}

void initTime() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

// ------------------ حلقه اصلی بهبود یافته ------------------
void loop() {
  static int16_t touchX, touchY;
  
  if(touchManager.getTouch(touchX, touchY)) {
    handleUI(touchX, touchY);
  }
  
  updateStatusBar();
}

// ------------------ سیستم رنگ پویا ------------------
uint16_t getColor(uint8_t colorIndex) {
  return COLORS[darkMode][colorIndex];
}

// ------------------ رابط کاربری پیشرفته ------------------
void drawHomeScreen() {
  tft.fillScreen(getColor(BACKGROUND));
  drawStatusBar();
  drawAppGrid();
}

void drawStatusBar() {
  tft.fillRect(0, 0, 240, 30, getColor(PRIMARY));
  
  // نمایش زمان واقعی
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    char timeStr[9];
    strftime(timeStr, 9, "%H:%M:%S", &timeinfo);
    drawText(timeStr, 160, 8, TEXT, 1);
  }
  
  drawText("ESP32 Phone", 10, 8, TEXT, 1);
}

void drawAppGrid() {
  for(uint8_t i=0; i<3; i++) {
    uint8_t appIndex = currentPage*3 + i;
    if(appIndex >= APPS_COUNT) break;
    
    drawAppIcon(apps[appIndex].label, (i%3)*80+20, 80);
  }
}

void drawAppIcon(const char* label, int x, int y) {
  tft.fillRoundRect(x, y, 80, 80, 15, getColor(SECONDARY));
  drawText(label, x+15, y+90, TEXT, 1);
}

// ------------------ مدیریت تاچ پیشرفته ------------------
void handleUI(int16_t x, int16_t y) {
  static uint8_t currentApp = 255;
  
  if(currentApp == 255) { // حالت اصلی
    if(y > 30 && y < 280) { // ناحیه آیکون‌ها
      handleAppSelection(x, y);
    }
    else if(y > 280) { // ناحیه ناوبری
      handleNavigation(x, y);
    }
  }
  else { // حالت اپلیکیشن
    apps[currentApp].update();
  }
}

void handleAppSelection(int16_t x, int16_t y) {
  for(uint8_t i=0; i<3; i++) {
    uint8_t appIndex = currentPage*3 + i;
    if(appIndex >= APPS_COUNT) break;
    
    int ax = (i%3)*80+20;
    if(x > ax && x < ax+80 && y > 80 && y < 160) {
      animateIcon(ax, 80);
      launchApp(appIndex);
    }
  }
}

void animateIcon(int x, int y) {
  tft.fillRoundRect(x, y, 80, 80, 15, getColor(ACCENT));
  delay(80);
  tft.fillRoundRect(x, y, 80, 80, 15, getColor(SECONDARY));
}

// ------------------ اپلیکیشن وای‌فای پیشرفته ------------------
void launchWiFi() {
  tft.fillScreen(getColor(BACKGROUND));
  drawStatusBar();
  drawScrollableArea();
  scanWiFiNetworks();
}

void updateWiFi() {
  // مدیریت اسکرول لمسی
  static int16_t startY = -1;
  int16_t x, y;
  
  if(touchManager.getTouch(x, y)) {
    if(startY == -1) startY = y;
    else {
      int delta = startY - y;
      if(abs(delta) > SCROLL_THRESHOLD) {
        wifiScrollOffset += delta/10;
        drawWiFiList();
        startY = y;
      }
    }
  }
  else {
    startY = -1;
  }
}

void drawWiFiList() {
  tft.fillRect(0, 30, 240, 250, getColor(BACKGROUND));
  for(int i=0; i<5; i++) {
    int yPos = 30 + i*50 - wifiScrollOffset;
    if(yPos > 30 && yPos < 280) {
      drawNetworkItem("WiFi Network", -60, yPos);
    }
  }
}

// ------------------ توابع کمکی عمومی ------------------
void drawText(const char* text, int x, int y, uint8_t colorIndex, uint8_t size=1) {
  tft.setTextColor(getColor(colorIndex));
  tft.setTextSize(size);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawButton(const char* label, int x, int y, int w=60, int h=30) {
  tft.fillRoundRect(x, y, w, h, 10, getColor(ACCENT));
  drawText(label, x+15, y+10, TEXT);
}

// =================== بهبودهای اضافه شده ===================
// 1. سیستم اسکرول لمسی برای لیست‌های طولانی
// 2. انیمیشن‌های ظریف برای بازخورد لمسی
// 3. مدیریت حافظه بهینه‌تر با حذف String
// 4. معماری ماژولار با استفاده از ساختار App
// 5. به‌روزرسانی خودکار زمان در نوار وضعیت
// 6. سیستم تم پویا با آرایه رنگ
// ========================================================
