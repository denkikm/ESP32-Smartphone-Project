#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>

// ==================== پیکربندی پین‌ها ====================
#define TFT_CS        5
#define TFT_DC        2
#define TFT_RST       4
#define BACKLIGHT_PIN 25

#define TOUCH_CS   15
#define TOUCH_IRQ  27

// ==================== تنظیمات کالیبراسیون ====================
const int touch_min_x = 350;
const int touch_max_x = 3850;
const int touch_min_y = 150;
const int touch_max_y = 3750;

// ==================== ثابت‌های طراحی ====================
#define PRIMARY_COLOR    0x18C3
#define DARK_BACKGROUND  0x0020
#define ACCENT_COLOR     0x07FF
#define TEXT_COLOR       0xFFFF

// ==================== پیش‌اعلان توابع ====================
void scanNetworks(); // <--- این خط را اضافه کنید

SPIClass vspi(VSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ==================== ساختارهای سیستم ====================
enum AppState {HOME, WIFI_TOOLS, SETTINGS};
AppState currentApp = HOME;

typedef struct {
  uint16_t x;
  uint16_t y;
  String label;
  void (*action)();
} AppIcon;

// ==================== آیکون‌های اصلی ====================
AppIcon homeScreen[] = {
  {30, 80, "WiFi", [](){ currentApp = WIFI_TOOLS; }},
  {130, 80, "Scan", [](){ scanNetworks(); }}, // <--- حالا کامپایلر تابع را می‌شناسد
  {30, 180, "Settings", [](){ currentApp = SETTINGS; }},
  {130, 180, "Power", [](){ shutdown(); }}
};

// ==================== راه‌اندازی اولیه ====================
void setup() {
  Serial.begin(115200);
  
  // فعال‌سازی نور پس‌زمینه
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  // تنظیمات نمایشگر
  vspi.begin();
  tft.init(240, 320);
  tft.setRotation(0);
  tft.fillScreen(DARK_BACKGROUND);

  // تنظیمات تاچ
  touch.begin();
  
  drawStatusBar();
  drawHomeScreen();
}

// ==================== حلقه اصلی ====================
void loop() {
  switch(currentApp){
    case HOME:
      handleHomeScreen();
      break;
    case WIFI_TOOLS:
      handleWiFiTools();
      break;
    case SETTINGS:
      handleSettings();
      break;
  }
  updateStatusBar();
}

// ==================== رابط خانگی ====================
void drawHomeScreen(){
  tft.fillScreen(DARK_BACKGROUND);
  
  // رسم آیکون‌ها
  for(int i=0; i<4; i++){
    drawRoundedRect(homeScreen[i].x, homeScreen[i].y, 80, 80, 15, PRIMARY_COLOR);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(homeScreen[i].x+15, homeScreen[i].y+90);
    tft.print(homeScreen[i].label);
  }
}

void handleHomeScreen(){
  if(touch.touched()){
    TS_Point p = touch.getPoint();
    int16_t x = map(p.x, touch_min_x, touch_max_x, 0, 240);
    int16_t y = map(p.y, touch_min_y, touch_max_y, 320, 0);
    
    for(int i=0; i<4; i++){
      if(x > homeScreen[i].x && x < homeScreen[i].x+80 &&
         y > homeScreen[i].y && y < homeScreen[i].y+80){
         
        // افکت کلیک
        drawRoundedRect(homeScreen[i].x, homeScreen[i].y, 80, 80, 15, ACCENT_COLOR);
        delay(100);
        drawRoundedRect(homeScreen[i].x, homeScreen[i].y, 80, 80, 15, PRIMARY_COLOR);
        
        homeScreen[i].action();
      }
    }
  }
}

// ==================== نوار وضعیت ====================
void drawStatusBar(){
  tft.fillRect(0, 0, 240, 30, PRIMARY_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(10, 8);
  tft.print("ESP32 Phone");
}

void updateStatusBar(){
  static uint32_t lastUpdate = 0;
  if(millis() - lastUpdate > 1000){
    // آپدیت ساعت
    tft.fillRect(180, 8, 50, 20, PRIMARY_COLOR);
    tft.setCursor(180, 8);
    tft.print("12:00");
    lastUpdate = millis();
  }
}

// ==================== ابزارهای وایفای ====================
void handleWiFiTools(){
  tft.fillScreen(DARK_BACKGROUND);
  tft.setCursor(50, 50);
  tft.print("WiFi Tools");
  // افزودن منطق اسکن شبکه
}

void scanNetworks(){
  tft.fillScreen(DARK_BACKGROUND);
  tft.setCursor(10, 40);
  tft.print("Scanning Networks...");
  
  // اسکن واقعی شبکه
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  int n = WiFi.scanNetworks();
  tft.setCursor(10, 60);
  tft.print(n);
  tft.print(" networks found");

  // نمایش شبکه‌های یافت شده
  for (int i = 0; i < n; ++i) {
    tft.setCursor(10, 80 + (i * 20));
    tft.print(WiFi.SSID(i));
    tft.print(" (");
    tft.print(WiFi.RSSI(i));
    tft.print(")");
    tft.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
  }
}

// ==================== تنظیمات ====================
void handleSettings(){
  tft.fillScreen(DARK_BACKGROUND);
  tft.setCursor(50, 50);
  tft.print("Settings");
}

// ==================== توابع کمکی ====================
void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color){
  tft.fillRoundRect(x, y, w, h, r, color);
  tft.drawRoundRect(x, y, w, h, r, color-0x1000);
}

void shutdown(){
  digitalWrite(BACKLIGHT_PIN, LOW);
  esp_deep_sleep_start();
}
