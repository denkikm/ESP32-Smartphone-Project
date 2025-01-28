#include <SPI.h>
#include "XPT2046_Calibrated.h"

// پین‌های تعریف‌شده برای صفحه لمسی
#define TOUCH_CS   15  // پین انتخاب چیپ
#define TOUCH_IRQ  27  // پین وقفه (اختیاری)

// ایجاد شیء صفحه لمسی
XPT2046_Calibrated touch(TOUCH_CS, TOUCH_IRQ);

// تنظیمات کالیبراسیون
TS_Calibration calib(
  TS_Point(200, 200), TS_Point(0, 0),
  TS_Point(3800, 200), TS_Point(240, 0),
  TS_Point(200, 3800), TS_Point(0, 320),
  240, 320
);

void setup() {
  Serial.begin(115200);
  SPI.begin(); // شروع SPI
  
  // راه‌اندازی صفحه لمسی
  if (touch.begin()) {
    Serial.println("Touchscreen initialized successfully!");
    touch.calibrate(calib); // اعمال کالیبراسیون
    Serial.println("Calibration applied.");
  } else {
    Serial.println("Failed to initialize touchscreen.");
    while (true); // توقف در صورت خطا
  }
}

void loop() {
  // بررسی لمس شدن صفحه
  if (touch.touched()) {
    TS_Point point = touch.getPoint(); // دریافت مختصات لمس‌شده
    Serial.print("Touch detected at: X=");
    Serial.print(point.x);
    Serial.print(", Y=");
    Serial.println(point.y);
    delay(100); // تأخیر برای جلوگیری از پر شدن سریال مانیتور
  }
}
