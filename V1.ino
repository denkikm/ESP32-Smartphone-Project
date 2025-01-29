#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>

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

// ------------------ ثابت‌های طراحی ------------------
#define PRIMARY_COLOR    0x18C3
#define SECONDARY_COLOR  0x4A69
#define ACCENT_COLOR     0x03FF
#define BACKGROUND_COLOR 0x10A2
#define TEXT_COLOR       0xFFFF
#define WIFI_ICON_COLOR  0x07FF
#define DARK_BACKGROUND  0x0000
#define DARK_TEXT        0xFFFF

SPIClass vspi(VSPI);
SPIClass hspi(HSPI);

Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ------------------ متغیرهای全局 ------------------
int currentPage = 0; // صفحه فعلی
bool darkMode = false; // حالت Dark Mode
String notes[10]; // ذخیره نوت‌ها
int noteIndex = 0; // شماره نوت فعلی

// ------------------ ساختارهای داده ------------------
typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  String label;
} AppIcon;

AppIcon apps[6] = {
  {20, 60, 80, 80, "WiFi"},
  {120, 60, 80, 80, "Tools"},
  {20, 160, 80, 80, "Notes"},
  {120, 160, 80, 80, "Calc"},
  {20, 260, 80, 80, "System"},
  {120, 260, 80, 80, "Settings"}
};

// ------------------ توابع کمکی ------------------
void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, radius, color);
  tft.drawRoundRect(x, y, w, h, radius, darkenColor(color, 20));
}

uint16_t darkenColor(uint16_t color, uint8_t percent) {
  uint8_t r = (color >> 11) * 0.9;
  uint8_t g = ((color >> 5) & 0x3F) * 0.9;
  uint8_t b = (color & 0x1F) * 0.9;
  return (r << 11) | (g << 5) | b;
}

void drawStatusBar() {
  tft.fillRect(0, 0, 240, 30, darkMode ? DARK_BACKGROUND : PRIMARY_COLOR);
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.setCursor(10, 8);
  tft.print("ESP32 Phone v1.0");
  
  // آیکون وایفای
  tft.fillCircle(210, 15, 8, WIFI_ICON_COLOR);
  tft.drawCircle(210, 15, 10, darkenColor(WIFI_ICON_COLOR, 20));
  
  // ساعت
  tft.setCursor(160, 8);
  tft.print("12:00");
}

void drawAppIcons() {
  for(int i=0; i<6; i++){
    drawRoundedRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, darkMode ? DARK_BACKGROUND : SECONDARY_COLOR);
    tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
    tft.setTextSize(1);
    tft.setCursor(apps[i].x + 15, apps[i].y + apps[i].height + 5);
    tft.print(apps[i].label);
  }
}

// ------------------ Setup & Loop ------------------
void setup() {
  Serial.begin(115200);
  
  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  
  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);
  
  drawStatusBar();
  drawAppIcons();
}

void loop() {
  static int16_t touchStartX = 0;
  
  if(touch.touched()){
    TS_Point p = touch.getPoint();
    int16_t x = map(p.x, 250, 3750, 0, 240);
    int16_t y = map(p.y, 250, 3750, 0, 320);
    
    if(touchStartX == 0) {
      touchStartX = x;
    } else {
      handleSwipe(touchStartX, x);
      touchStartX = 0;
    }
    
    handleTouch(x, y);
  }
  
  updateClock();
}

// ------------------ منطق تاچ ------------------
void handleTouch(int16_t x, int16_t y) {
  for(int i=0; i<6; i++){
    if(x > apps[i].x && x < apps[i].x + apps[i].width &&
       y > apps[i].y && y < apps[i].y + apps[i].height){
       
      // افکت کلیک
      drawRoundedRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, ACCENT_COLOR);
      delay(100);
      drawRoundedRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, darkMode ? DARK_BACKGROUND : SECONDARY_COLOR);
      
      launchApp(apps[i].label);
    }
  }
}

void handleSwipe(int16_t startX, int16_t endX) {
  if (startX - endX > 50) { // سوایپ به چپ
    currentPage = (currentPage + 1) % 3;
    drawPage(currentPage);
  } else if (endX - startX > 50) { // سوایپ به راست
    currentPage = (currentPage - 1 + 3) % 3;
    drawPage(currentPage);
  }
}

void drawPage(int page) {
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  
  switch(page) {
    case 0:
      drawAppIcons();
      break;
    case 1:
      tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
      tft.setTextSize(2);
      tft.setCursor(50, 100);
      tft.print("Notes Page");
      break;
    case 2:
      tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
      tft.setTextSize(2);
      tft.setCursor(50, 100);
      tft.print("Tools Page");
      break;
  }
}

void launchApp(String appName) {
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  
  tft.setTextColor(darkMode ? DARK_TEXT : TEXT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(50, 60);
  tft.print(appName + " App");
  
  delay(2000);
  tft.fillScreen(darkMode ? DARK_BACKGROUND : BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
}

void updateClock() {
  static uint32_t lastUpdate = 0;
  if(millis() - lastUpdate > 60000){
    tft.fillRect(160, 8, 60, 20, darkMode ? DARK_BACKGROUND : PRIMARY_COLOR);
    tft.setCursor(160, 8);
    tft.print("12:00"); // باید با RTC ادغام شود
    lastUpdate = millis();
  }
}
