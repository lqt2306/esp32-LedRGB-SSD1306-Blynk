// BẮT BUỘC đặt 3 dòng này trên cùng trước mọi #include
#define BLYNK_TEMPLATE_ID   "TMPL6EKn4Citc"
#define BLYNK_TEMPLATE_NAME "New Template"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Thông tin WiFi 2.4GHz nhà em
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASS";

// Cấu hình màn hình OLED SSD1306 (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledReady = false;

// Chân LED RGB giữ nguyên như mạch đang chạy
#define PIN_RED   25
#define PIN_GREEN 26
#define PIN_BLUE  27

// Đổi thành true nếu LED của em là loại Anode chung (chân dài nối 3V3)
#define COMMON_ANODE false

// Biến lưu trạng thái 4 công tắc trên Blynk
bool isPowerOn = false;
bool redOn     = false;
bool greenOn   = false;
bool blueOn    = false;

// Hàm cập nhật nội dung hiển thị lên màn hình OLED
// Hàm tự tính độ rộng chữ để căn giữa chuẩn xác 100% (không phải đoán tọa độ X bằng tay)
void printCentered(const char* text, int16_t y, uint8_t textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(textSize);
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - (int16_t)w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

// Hàm cập nhật nội dung hiển thị lên màn hình OLED
void updateOLED() {
  if (!oledReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Dòng tiêu đề căn giữa
  printCentered("TRANG THAI DEN", 0, 1);
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  // 1. Nguồn tổng đang TẮT
  if (!isPowerOn) {
    printCentered("LED OFF", 30, 2);
    display.display();
    return;
  }

  // Đếm tổng số công tắc màu đang BẬT (0, 1, 2 hoặc 3)
  uint8_t soDenBat = (uint8_t)redOn + (uint8_t)greenOn + (uint8_t)blueOn;

  if (soDenBat == 0) {
    printCentered("LED ON", 30, 2);
  } else if (soDenBat == 3) {
    printCentered("ALL ON", 30, 2);
  } else if (soDenBat == 1) {
    // Bật 1 đèn -> căn giữa 1 dòng cỡ to
    if (redOn)        printCentered("Red On", 30, 2);
    else if (greenOn) printCentered("Green On", 30, 2);
    else              printCentered("Blue On", 30, 2);
  } else {
    // Bật 2 đèn -> chia 2 dòng cỡ to căn giữa cho dễ đọc và không tràn mép
    if (redOn && greenOn) {
      printCentered("Red and", 20, 2);
      printCentered("Green On", 40, 2);
    } else if (redOn && blueOn) {
      printCentered("Red and", 20, 2);
      printCentered("Blue On", 40, 2);
    } else {
      printCentered("Green and", 20, 2);
      printCentered("Blue On", 40, 2);
    }
  }

  display.display();
}

// Hàm xuất điện áp ra LED RGB và đồng bộ lên OLED
void updateSystem() {
  uint8_t r = (isPowerOn && redOn)   ? 255 : 0;
  uint8_t g = (isPowerOn && greenOn) ? 255 : 0;
  uint8_t b = (isPowerOn && blueOn)  ? 255 : 0;

  if (COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  analogWrite(PIN_RED, r);
  analogWrite(PIN_GREEN, g);
  analogWrite(PIN_BLUE, b);

  updateOLED();
}

// Tự đồng bộ trạng thái nút trên app khi ESP32 vừa nối mạng
BLYNK_CONNECTED() {
  Blynk.syncAll();
}

// V0: Công tắc nguồn tổng (ON / OFF)
BLYNK_WRITE(V0) {
  isPowerOn = (param.asInt() > 0);
  updateSystem();
}

// V1: Công tắc RED
BLYNK_WRITE(V1) {
  redOn = (param.asInt() > 0);
  updateSystem();
}

// V2: Công tắc GREEN
BLYNK_WRITE(V2) {
  greenOn = (param.asInt() > 0);
  updateSystem();
}

// V3: Công tắc BLUE
BLYNK_WRITE(V3) {
  blueOn = (param.asInt() > 0);
  updateSystem();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  // Khởi động I2C (mặc định SDA=21, SCL=22; nếu cắm D19 thì đổi thành Wire.begin(19, 22))
  Wire.begin(19, 22);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 28);
    display.println("Dang ket noi WiFi...");
    display.display(); // Xóa nhiễu tuyết ngay lúc khởi động
  } else {
    Serial.println("Loi: Khong tim thay OLED SSD1306 (kiem tra lai day SDA/SCL)!");
  }

  updateSystem();
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
}

void loop() {
  Blynk.run();
}