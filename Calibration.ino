#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// ------------------ پیکربندی پین‌ها ------------------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TOUCH_CS 15
#define TOUCH_IRQ 27

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

uint16_t rawX[4], rawY[4]; // ذخیره مختصات خام لمس‌شده
int calibrationStep = 0;   // مرحله کالیبراسیون

// ------------------ تنظیمات اولیه ------------------
void setup() {
  Serial.begin(115200);

  tft.init(240, 320);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  if (!touch.begin()) {
    Serial.println("Error: Touch screen not detected!");
    while (1);
  }

  showCalibrationPoint(calibrationStep);
}

// ------------------ حلقه اصلی ------------------
void loop() {
  if (calibrationStep < 4) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      rawX[calibrationStep] = p.x;
      rawY[calibrationStep] = p.y;
      Serial.print("Point ");
      Serial.print(calibrationStep + 1);
      Serial.print(": X = ");
      Serial.print(rawX[calibrationStep]);
      Serial.print(", Y = ");
      Serial.println(rawY[calibrationStep]);
      calibrationStep++;
      delay(500); // تاخیر برای جلوگیری از لمس دوباره
      if (calibrationStep < 4) {
        showCalibrationPoint(calibrationStep);
      } else {
        finishCalibration();
      }
    }
  }
}

// ------------------ توابع کالیبراسیون ------------------
void showCalibrationPoint(int step) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(20, 140);
  tft.print("Touch the point:");

  switch (step) {
    case 0: // بالا چپ
      drawCalibrationDot(20, 20);
      break;
    case 1: // بالا راست
      drawCalibrationDot(220, 20);
      break;
    case 2: // پایین چپ
      drawCalibrationDot(20, 300);
      break;
    case 3: // پایین راست
      drawCalibrationDot(220, 300);
      break;
  }
}

void drawCalibrationDot(int x, int y) {
  tft.fillCircle(x, y, 5, ST77XX_WHITE);
}

void finishCalibration() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(20, 140);
  tft.print("Calibration Done!");
  Serial.println("Calibration completed!");
  Serial.println("Raw Touch Points:");
  for (int i = 0; i < 4; i++) {
    Serial.print("Point ");
    Serial.print(i + 1);
    Serial.print(": X = ");
    Serial.print(rawX[i]);
    Serial.print(", Y = ");
    Serial.println(rawY[i]);
  }
  while (1); // توقف برنامه
}
