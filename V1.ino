#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>

// ------------------ پیکربندی پین‌های جدید ------------------
#define BACKLIGHT_PIN 12 // پین کنترل نور پس زمینه

// پیکربندی نمایشگر برای حالت عمودی
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// پین‌های نمایشگر (بهبود یافته برای حالت عمودی)
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

// پین‌های تاچ (تنظیم مجدد برای هماهنگی با جهت عمودی)
#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

// ------------------ ثابت‌های طراحی برای حالت عمودی ------------------
#define PRIMARY_COLOR    0x18C3
#define SECONDARY_COLOR  0x4A69
#define ACCENT_COLOR     0x03FF
#define BACKGROUND_COLOR 0x10A2
#define TEXT_COLOR       0xFFFF
#define WIFI_ICON_COLOR  0x07FF

SPIClass vspi(VSPI);
SPIClass hspi(HSPI);

Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ------------------ ساختارهای داده (بهینه شده برای جهت عمودی) ------------------
class AppIcon {
public:
  AppIcon(uint16_t x, uint16_t y, uint16_t w, uint16_t h, String lbl) 
    : x(x), y(y), width(w), height(h), label(lbl) {}
  
  bool contains(int16_t xPos, int16_t yPos) {
    return (xPos > x && xPos < x + width && yPos > y && yPos < y + height);
  }

  uint16_t x, y, width, height;
  String label;
};

AppIcon apps[6] = {
  // موقعیت‌های جدید برای جهت عمودی
  {30, 60, 80, 80, "WiFi"},
  {130, 60, 80, 80, "Tools"},
  {230, 60, 80, 80, "Settings"},
  {30, 160, 80, 80, "Scan"},
  {130, 160, 80, 80, "System"},
  {230, 160, 80, 80, "Power"}
};

// ------------------ توابع کمکی (بهینه شده) ------------------
void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, radius, color);
  tft.drawRoundRect(x, y, w, h, radius, darkenColor(color, 20));
}

void drawStatusBar() {
  tft.fillRect(0, 0, TFT_WIDTH, 30, PRIMARY_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(10, 8);
  tft.print("ESP32 Phone v1.0");
  
  tft.fillCircle(TFT_WIDTH - 30, 15, 8, WIFI_ICON_COLOR);
  tft.drawCircle(TFT_WIDTH - 30, 15, 10, darkenColor(WIFI_ICON_COLOR, 20));
  
  tft.setCursor(TFT_WIDTH - 100, 8);
  tft.print("12:00");
}

// ------------------ تنظیمات نور پس زمینه ------------------
void initBacklight() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH); // روشن کردن دائمی
}

// ------------------ تنظیمات لمسی برای جهت عمودی ------------------
TS_Point mapTouchToScreen(TS_Point p) {
  // کالیبراسیون برای جهت عمودی
  int16_t x = map(p.y, 200, 3700, 0, TFT_WIDTH);  // محورها معکوس شده
  int16_t y = map(p.x, 300, 3800, 0, TFT_HEIGHT);
  return TS_Point(x, y, p.z);
}

// ------------------ Setup & Loop بهینه شده ------------------
void setup() {
  Serial.begin(115200);
  initBacklight();
  
  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(0); // جهت عمودی
  tft.fillScreen(BACKGROUND_COLOR);
  
  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);
  
  drawStatusBar();
  drawAppIcons();
}

void loop() {
  static uint32_t last_touch = 0;
  
  if(touch.touched()){
    TS_Point p = mapTouchToScreen(touch.getPoint());
    
    if(millis() - last_touch > 200) {
      handleTouch(p.x, p.y);
      last_touch = millis();
    }
  }
  
  updateClock();
}

// ------------------ منطق تاچ (بهبود یافته) ------------------
void handleTouch(int16_t x, int16_t y) {
  for(int i=0; i<6; i++){
    if(apps[i].contains(x, y)) {
      drawRoundedRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, ACCENT_COLOR);
      delay(100);
      drawRoundedRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, SECONDARY_COLOR);
      launchApp(apps[i].label);
    }
  }
}
