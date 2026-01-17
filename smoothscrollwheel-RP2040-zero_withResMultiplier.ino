#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Wire.h>

// =========================================================
//                  USER TUNING VARIABLES
// =========================================================

// 1. INVERT_SCROLL: Set to true to reverse direction
const bool INVERT_SCROLL = true;

// 2. MIN_INPUT_THRESHOLD: Controls sensitivity to small movements.
const int MIN_INPUT_THRESHOLD = 1; 

// 3. PRECISION_SCALE: Controls the "Base Speed" for slow movements.
const float PRECISION_SCALE = 0.15; 

// 4. ACCELERATION: How quickly speed ramps up.
const float ACCELERATION = 0.35;

// 5. MAX_SCALE: Speed limit for fast flicks.
const float MAX_SCALE = 4.0; 

// =========================================================

// === AS5600 Configuration ===
#define AS5600_ADDRESS 0x36
#define RAW_ANGLE_HIGH 0x0C
#define SDA_PIN 4
#define SCL_PIN 5
#define LED_PIN 16

// === HID Configuration ===
#define REPORT_ID_MOUSE 1
#define RES_MULTIPLIER 120 

// Custom HID Report Descriptor
const uint8_t custom_desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
  
  0x85, REPORT_ID_MOUSE, // Report ID (1)
  
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
    
      // Buttons (Left, Right, Middle)
      0x05, 0x09,    //     Usage Page (Button)
      0x19, 0x01,    //     Usage Minimum (0x01)
      0x29, 0x03,    //     Usage Maximum (0x03)
      0x15, 0x00,    //     Logical Minimum (0)
      0x25, 0x01,    //     Logical Maximum (1)
      0x95, 0x03,    //     Report Count (3)
      0x75, 0x01,    //     Report Size (1)
      0x81, 0x02,    //     Input (Data,Var,Abs)
      
      // Padding
      0x95, 0x01,    //     Report Count (1)
      0x75, 0x05,    //     Report Size (5)
      0x81, 0x03,    //     Input (Const,Var,Abs)
      
      // X, Y Axes
      0x05, 0x01,    //     Usage Page (Generic Desktop Ctrls)
      0x09, 0x30,    //     Usage (X)
      0x09, 0x31,    //     Usage (Y)
      0x15, 0x81,    //     Logical Minimum (-127)
      0x25, 0x7F,    //     Logical Maximum (127)
      0x75, 0x08,    //     Report Size (8)
      0x95, 0x02,    //     Report Count (2)
      0x81, 0x06,    //     Input (Data,Var,Rel)

      // Scroll Wheel
      0x09, 0x38,    //     Usage (Wheel)
      0x15, 0x81,    //     Logical Minimum (-127)
      0x25, 0x7F,    //     Logical Maximum (127)
      0x75, 0x08,    //     Report Size (8)
      0x95, 0x01,    //     Report Count (1)
      0x81, 0x06,    //     Input (Data,Var,Rel)
      
      // --- Resolution Multiplier Feature ---
      0x05, 0x01,    //     Usage Page (Generic Desktop Ctrls)
      0x09, 0x48,    //     Usage (Resolution Multiplier)
      0x15, 0x00,    //     Logical Minimum (0)
      0x25, 0x01,    //     Logical Maximum (1) 
      0x35, 0x01,    //     Physical Minimum (1)
      0x45, 0x78,    //     Physical Maximum (120) - Tells Windows 1 notch = 120 units
      0x75, 0x08,    //     Report Size (8)
      0x95, 0x01,    //     Report Count (1)
      0xB1, 0x02,    //     Feature (Data,Var,Abs)

    0xC0,            //   End Collection
  0xC0               // End Collection
};

Adafruit_USBD_HID usb_hid;

// Variables
int lastAngle = 0;
int currentAngle = 0;
float accumulatedScroll = 0;
unsigned long lastUpdateTime = 0;

// Callbacks
uint16_t get_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_FEATURE) {
    // We return logical 1. Windows maps this to physical 120 based on the descriptor.
    buffer[0] = 1; 
    return 1;
  }
  return 0;
}

void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  // Windows sends a set report to enable high-res. 
  // Since we always want it, we don't need to save the state, but we must acknowledge it exists.
}

// I2C
int readAS5600(byte registerAddress) {
  Wire.beginTransmission(AS5600_ADDRESS);
  Wire.write(registerAddress);
  Wire.endTransmission();
  Wire.requestFrom(AS5600_ADDRESS, 2);
  if (Wire.available() >= 2) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    return (highByte << 8) | lowByte;
  }
  return -1;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(custom_desc_hid_report, sizeof(custom_desc_hid_report));
  usb_hid.setReportCallback(get_report_callback, set_report_callback);
  usb_hid.begin();

  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  lastAngle = readAS5600(RAW_ANGLE_HIGH);
  lastUpdateTime = millis();
}

void loop() {
  if (!TinyUSBDevice.mounted()) return;

  unsigned long now = millis();
  unsigned long deltaTime = now - lastUpdateTime;
  
  if (deltaTime < 2) return;
  lastUpdateTime = now;

  currentAngle = readAS5600(RAW_ANGLE_HIGH);
  if (currentAngle < 0) return;

  int angleDiff = currentAngle - lastAngle;
  
  // Wraparound
  if (angleDiff > 2048) angleDiff -= 4096;
  else if (angleDiff < -2048) angleDiff += 4096;

  lastAngle = currentAngle;

  if (abs(angleDiff) < MIN_INPUT_THRESHOLD) return;

  if (INVERT_SCROLL) angleDiff = -angleDiff;

  // Dynamic Scale
  float velocity = abs(angleDiff) / (float)deltaTime;
  float dynamicScale = PRECISION_SCALE + (velocity * ACCELERATION);
  if (dynamicScale > MAX_SCALE) dynamicScale = MAX_SCALE;

  accumulatedScroll += (angleDiff * dynamicScale);

  int scrollToSend = (int)accumulatedScroll;

  if (scrollToSend != 0) {
    uint8_t report[4] = { 0, 0, 0, (uint8_t)scrollToSend };
    
    if (usb_hid.ready()) {
      usb_hid.sendReport(REPORT_ID_MOUSE, report, 4);
      accumulatedScroll -= scrollToSend;
      
      if (abs(scrollToSend) > 5) { 
         digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
    }
  }
}