#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Calibrated.h>
#include <SPI.h>
#include <Wire.h>

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
XPT2046_Calibrated touch(TOUCH_CS, TOUCH_IRQ);

void setup() {
  Serial.begin(115200);

  vspi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  vspi.setFrequency(30000000);
  tft.init(240, 320);
  tft.setRotation(0);  // حالت عمودی
  tft.fillScreen(0x0000);  // پرکردن صفحه با رنگ مشکی

  hspi.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(hspi);

  // رسم صفحه کالیبراسیون
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.print("Touch the Crosshairs!");

  // انجام کالیبراسیون
  touch.calibrate();
  
  // پس از اتمام کالیبراسیون، مقادیر کالیبراسیون در حافظه ذخیره می‌شود.
  // شما می‌توانید از مقادیر ذخیره‌شده استفاده کنید.
  Serial.println("Calibration Complete!");
}

void loop() {
  // در اینجا هیچ کاری نیاز نیست، پس از کالیبراسیون، مقادیر کالیبره شده در حافظه ذخیره می‌شود.
  // در کد اصلی شما می‌توانید از این مقادیر برای استفاده صحیح از صفحه لمسی استفاده کنید.
}
