#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#include <TouchScreen_Calibrator.h>  // کتابخانه کالیبراسیون

// ------------------ پیکربندی پین‌ها ------------------
// پین‌های نمایشگر
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

// پین‌های تاچ
#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

SPIClass vspi(VSPI);
SPIClass hspi(HSPI);

Adafruit_ST7789 tft = Adafruit_ST7789(&vspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ایجاد شی برای کالیبراسیون
TouchScreen_Calibrator calibrator(tft, touch);

void setup() {
  Serial.begin(115200);

  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(0);  // حالت عمودی
  tft.fillScreen(0x0000);  // پرکردن صفحه با رنگ مشکی

  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);

  // شروع فرآیند کالیبراسیون
  calibrator.startCalibration();
}

void loop() {
  // کالیبراسیون به طور خودکار انجام می‌شود
  // شما می‌توانید تا زمانی که کالیبراسیون کامل می‌شود، هیچ کاری نکنید.
  // پس از پایان کالیبراسیون، می‌توانید مقادیر به‌دست آمده را استفاده کنید.
}
