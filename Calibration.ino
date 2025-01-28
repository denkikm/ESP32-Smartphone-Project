#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// ------------------ پیکربندی پین‌ها ------------------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

#define TOUCH_CS   15
#define TOUCH_IRQ  27

// ------------------ تنظیمات SPI ------------------
SPIClass vspi(VSPI); // استفاده از SPI سرعت بالا (VSPI)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ------------------ متغیرهای کالیبره‌کردن ------------------
int touch_min_x = 1000, touch_max_x = 0;
int touch_min_y = 1000, touch_max_y = 0;

void setup() {
  Serial.begin(115200);

  // راه‌اندازی صفحه‌نمایش
  tft.init(240, 320); // اندازه صفحه‌نمایش
  tft.setRotation(1); // تنظیم جهت صفحه‌نمایش (در صورت نیاز تغییر دهید)
  tft.fillScreen(ST77XX_BLACK); // پاک‌سازی صفحه با رنگ سیاه

  // راه‌اندازی تاچ‌اسکرین
  touch.begin();
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TOUCH_CS); // تنظیم SPI برای تاچ‌اسکرین

  Serial.println("Touchscreen calibration started. Touch the corners of the screen.");
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint(); // دریافت نقطه لمسی

    // به‌روزرسانی مقادیر کمینه و بیشینه
    if (p.x < touch_min_x) touch_min_x = p.x;
    if (p.x > touch_max_x) touch_max_x = p.x;
    if (p.y < touch_min_y) touch_min_y = p.y;
    if (p.y > touch_max_y) touch_max_y = p.y;

    // نمایش اطلاعات در Serial Monitor
    Serial.print("Raw X = "); Serial.print(p.x);
    Serial.print("\tRaw Y = "); Serial.print(p.y);
    Serial.print("\tMinX = "); Serial.print(touch_min_x);
    Serial.print("\tMaxX = "); Serial.print(touch_max_x);
    Serial.print("\tMinY = "); Serial.print(touch_min_y);
    Serial.print("\tMaxY = "); Serial.println(touch_max_y);

    delay(100); // تاخیر برای جلوگیری از خواندن‌های مکرر
  }
}
