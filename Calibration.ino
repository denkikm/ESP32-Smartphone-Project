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

// ------------------ متغیرهای کالیبره‌کردن ------------------
int touch_min_x = 1000, touch_max_x = 0;
int touch_min_y = 1000, touch_max_y = 0;

void setup() {
  Serial.begin(115200);
  tft.init(240, 320); // Initialize the display
  tft.setRotation(1); // Adjust rotation if needed
  touch.begin();      // Initialize the touchscreen

  // روشن کردن نور پس‌زمینه
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  Serial.println("Touchscreen calibration started. Touch the corners of the screen.");
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    if (p.x < touch_min_x) touch_min_x = p.x;
    if (p.x > touch_max_x) touch_max_x = p.x;
    if (p.y < touch_min_y) touch_min_y = p.y;
    if (p.y > touch_max_y) touch_max_y = p.y;

    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tMinX = "); Serial.print(touch_min_x);
    Serial.print("\tMaxX = "); Serial.print(touch_max_x);
    Serial.print("\tMinY = "); Serial.print(touch_min_y);
    Serial.print("\tMaxY = "); Serial.println(touch_max_y);

    delay(100); // Delay to avoid multiple readings
  }
}
