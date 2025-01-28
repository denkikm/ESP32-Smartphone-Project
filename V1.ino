#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
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

#define BACKLIGHT_PIN 21

// ------------------ ثابت‌ها و تنظیمات ------------------
#define PRIMARY_COLOR    0x18C3
#define SECONDARY_COLOR  0x4A69
#define ACCENT_COLOR     0x03FF
#define BACKGROUND_COLOR 0x10A2
#define TEXT_COLOR       0xFFFF

SPIClass vspi(VSPI);
SPIClass hspi(HSPI);

Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

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
  {20, 160, 80, 80, "Settings"},
  {120, 160, 80, 80, "Scan"},
  {20, 260, 80, 80, "System"},
  {120, 260, 80, 80, "Power"}
};

// متغیرهای کالیبراسیون تاچ
int16_t x_min = 250, x_max = 3750, y_min = 250, y_max = 3750;
bool isAppRunning = false;

// ------------------ توابع کالیبراسیون ------------------
void calibrateTouch() {
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.println("Touch the corners as instructed.");

  // کالیبراسیون گوشه بالا چپ
  tft.fillCircle(20, 20, 10, ACCENT_COLOR);
  while (!touch.touched());
  TS_Point p = touch.getPoint();
  x_min = p.x;
  y_min = p.y;
  delay(500);

  // کالیبراسیون گوشه پایین راست
  tft.fillCircle(220, 300, 10, ACCENT_COLOR);
  while (!touch.touched());
  p = touch.getPoint();
  x_max = p.x;
  y_max = p.y;
  delay(500);

  tft.fillScreen(BACKGROUND_COLOR);
  tft.setCursor(10, 10);
  tft.println("Calibration Complete.");
  delay(1000);
}

// ------------------ توابع رسم ------------------
void drawStatusBar() {
  tft.fillRect(0, 0, 240, 30, PRIMARY_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(10, 8);
  tft.print("ESP32 Phone v1.0");
}

void drawAppIcons() {
  for (int i = 0; i < 6; i++) {
    tft.fillRoundRect(apps[i].x, apps[i].y, apps[i].width, apps[i].height, 15, SECONDARY_COLOR);
    tft.setCursor(apps[i].x + 20, apps[i].y + apps[i].height / 2);
    tft.setTextColor(TEXT_COLOR);
    tft.print(apps[i].label);
  }
}

void drawBackButton() {
  tft.fillRoundRect(10, 290, 60, 20, 5, ACCENT_COLOR);
  tft.setCursor(20, 295);
  tft.setTextColor(TEXT_COLOR);
  tft.print("Back");
}

// ------------------ توابع برنامه ------------------
void launchApp(String appName) {
  tft.fillScreen(BACKGROUND_COLOR);
  drawStatusBar();
  drawBackButton();

  tft.setCursor(10, 50);
  tft.setTextColor(TEXT_COLOR);
  tft.print(appName + " App");
  
  isAppRunning = true;

  if (appName == "WiFi") {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
      tft.setCursor(10, 70 + (i * 15));
      tft.print(WiFi.SSID(i));
    }
  }
}

void handleTouch(int16_t x, int16_t y) {
  if (isAppRunning) {
    if (x > 10 && x < 70 && y > 290 && y < 310) {
      // بازگشت به صفحه اصلی
      tft.fillScreen(BACKGROUND_COLOR);
      drawStatusBar();
      drawAppIcons();
      isAppRunning = false;
    }
    return;
  }

  for (int i = 0; i < 6; i++) {
    if (x > apps[i].x && x < apps[i].x + apps[i].width &&
        y > apps[i].y && y < apps[i].y + apps[i].height) {
      launchApp(apps[i].label);
      break;
    }
  }
}

// ------------------ تنظیمات اولیه ------------------
void setup() {
  Serial.begin(115200);
  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(0);

  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  calibrateTouch();
  tft.fillScreen(BACKGROUND_COLOR);
  drawStatusBar();
  drawAppIcons();
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    int16_t x = map(p.x, x_min, x_max, 0, 240);
    int16_t y = map(p.y, y_min, y_max, 0, 320);
    handleTouch(x, y);
  }
}
