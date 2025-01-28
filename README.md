# ESP32-Smartphone-Project
An ESP32-Based DIY Smartphone with Modern UI and Advanced Networking Capabilities
## ✨ Key Features  
- **Modern Hybrid UI (Material Design + iOS Inspired)**  
  - Dark Mode with dynamic gradient backgrounds  
  - Smooth touch animations and rounded UI elements  
  - Status bar with WiFi/Battery indicators and digital clock  

- **Hardware Capabilities**  
  - 2.4" Touchscreen (ST7789/ILI9341) with XPT2046 touch controller  
  - Dual SPI bus architecture for simultaneous display/touch operations  
  - WiFi 802.11 b/g/n (Station + AP modes)  

- **Core Framework**  
  - Custom widget system for app development  
  - Touch gesture recognition (Tap/Hold/Swipe)  
  - Memory-optimized rendering engine  

## 🛠 Technical Specifications  
```yaml
Microcontroller: ESP32-WROVER (240MHz Dual-Core)  
Display: 240x320 TFT (ST7789) via VSPI  
Touch: XPT2046 Resistive Touch via HSPI  
RAM Usage: 220KB/520KB (Optimized)  
Power: 3.7V LiPo (1000mAh) with charging circuit  
```

## 📦 Pre-Installed Apps  
- **System Monitor**: Real-time CPU/RAM usage  
- **WiFi Toolkit**:  
  - Network scanner  
  - Signal strength analyzer  
- **Developer Console**: Serial terminal interface  

## 🔧 Installation  
1. **Prerequisites**:  
   - Arduino IDE/PlatformIO  
   - Libraries:  
     ```bash
     Adafruit ST7789 @ ^1.9.3  
     XPT2046_Touchscreen @ ^1.4.0  
     Adafruit GFX Library @ ^1.11.3
     ```

2. **Hardware Setup**:  
   ```cpp
   // Display (VSPI)
   #define TFT_CS   5
   #define TFT_DC   2  
   #define TFT_RST  4

   // Touch (HSPI)
   #define TOUCH_CS   15
   #define TOUCH_IRQ  27
   ```

3. **Calibration**:  
   ```cpp
   const int touch_min_x = 250;  // Requires per-device calibration
   const int touch_max_x = 3750;
   ```
