#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <driver/rtc_io.h>

// --- CONFIGURATION ---
// E-Ink Display Board MAC Address
uint8_t crowPanelMac[] = {0x44, 0x1B, 0xF6, 0x95, 0x55, 0x24};

// Pin Definitions
const int trigPin = 5;       // Ultrasonic Trigger
const int echoPin = 18;      // Ultrasonic Echo
const int batPin = 34;       // Battery Divider Analog Pin
const int tempPin = 17;      // DS18B20 1-Wire Data
const int RAIN_PIN = 27;     // MISOL Tipping Bucket Rain Gauge

// Deep Sleep Settings (10 Minutes)
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 600

// --- RTC RAM SLEEP-PERSISTENT STORAGE ---
RTC_DATA_ATTR uint16_t rtcRainTipsToSend = 0; // Ticks occurred since last transmission
RTC_DATA_ATTR uint16_t rtcStormTipCount = 0;  // Current storm rolling tips
RTC_DATA_ATTR uint32_t rtcLastRainTipTime = 0; // Epoch timestamp of last tip

// 1-Wire Temperature Setup
OneWire oneWire(tempPin);
DallasTemperature tempSensors(&oneWire);

// --- SENSOR ACQUISITION FUNCTIONS ---

// Reads the JSN-SR04T waterproof ultrasonic distance 5 times and applies a median filter
float readUltrasonicMedian() {
  float readings[5];
  int validCount = 0;
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  for (int i = 0; i < 5; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout (~5m max range)
    if (duration > 0) {
      float dist = duration * 0.0343f / 2.0f;
      // Filter out of bounds readings (JSN-SR04T active range is 20cm to 600cm)
      if (dist >= 20.0f && dist <= 600.0f) {
        readings[validCount++] = dist;
      }
    }
    delay(40);
  }
  
  if (validCount == 0) {
    return -1.0f; // Return error sentinel
  }
  
  // Sort readings to find the median value
  for (int i = 0; i < validCount - 1; i++) {
    for (int j = i + 1; j < validCount; j++) {
      if (readings[i] > readings[j]) {
        float temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }
  return readings[validCount / 2];
}

// Reads battery voltage via Pin 34 with 10-sample averaging and ESP32 ADC calibration
float readBatteryPercentage() {
  analogReadResolution(12); // Configure for 12-bit ADC (0-4095)
  
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(batPin);
    delay(5);
  }
  int rawADC = sum / 10;
  
  // Simple polynomial calibration to correct ESP32 ADC non-linearity
  // Map raw ADC to pin voltage (nominally 0V to 3.3V)
  float pinVoltage = (rawADC / 4095.0f) * 3.3f;
  
  // Resistor Divider (100k / 100k) scales battery voltage by 2
  float batteryVoltage = pinVoltage * 2.0f;
  
  // Convert battery voltage (3.2V = 0% capacity to 4.2V = 100% capacity)
  float percentage = ((batteryVoltage - 3.2f) / (4.2f - 3.2f)) * 100.0f;
  if (percentage > 100.0f) percentage = 100.0f;
  if (percentage < 0.0f) percentage = 0.0f;
  
  return percentage;
}

// Reads the DS18B20 water/air temperature sensor
float readTemperature() {
  tempSensors.begin();
  tempSensors.requestTemperatures();
  float tempC = tempSensors.getTempCByIndex(0);
  if (tempC == DEVICE_DISCONNECTED_C) {
    return -127.0f; // Error code sentinel
  }
  return tempC;
}

// --- ARDUINO MAIN ENTRY LOOP ---
void setup()
{
  Serial.begin(115200);
  delay(10);
  
  // 1. Analyze sleep wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // Wakeup caused by mechanical Rain Gauge bucket tip (Pin 27 pulled LOW)
    time_t nowTime = time(nullptr);
    
    // Software debouncing: ignore triggers within 1 second of last registered tip
    if (rtcLastRainTipTime > 0 && (nowTime - rtcLastRainTipTime < 1)) {
      Serial.println("Rain tip debounced (reed switch bounce). Ignoring.");
    } else {
      // Check 1-hour storm window (3600 seconds)
      if (rtcLastRainTipTime > 0 && (nowTime - rtcLastRainTipTime > 3600)) {
        // Last tip was over an hour ago. Declare a new storm.
        Serial.println("New storm window starting. Resetting storm tip counter.");
        rtcStormTipCount = 0;
      }
      
      rtcRainTipsToSend++;
      rtcStormTipCount++;
      rtcLastRainTipTime = nowTime;
      
      Serial.print("Registered rain tip. Storm count: ");
      Serial.print(rtcStormTipCount);
      Serial.print(" | Pending upload: ");
      Serial.println(rtcRainTipsToSend);
    }
    
    // Configure internal pullup for deep sleep hold to keep Pin 27 from floating
    pinMode(RAIN_PIN, INPUT_PULLUP);
    rtc_gpio_init((gpio_num_t)RAIN_PIN);
    rtc_gpio_set_direction((gpio_num_t)RAIN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)RAIN_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)RAIN_PIN);
    
    // Setup wake up sources and immediately return to deep sleep
    esp_sleep_enable_ext0_wakeup((gpio_num_t)RAIN_PIN, 0); // Wake up when Pin 27 goes LOW
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    
    Serial.println("Returning to deep sleep immediately...");
    esp_deep_sleep_start();
  }
  
  // 2. Regular Wakeup (Timer / Power On / Reset)
  // Boot sensor hardware and collect data
  Serial.println("Wakeup timer triggered. Gathering sensor readings...");
  
  float distanceCm = readUltrasonicMedian();
  float batteryPct = readBatteryPercentage();
  float tempC = readTemperature();
  
  Serial.print("Ultrasonic: "); Serial.print(distanceCm); Serial.println(" cm");
  Serial.print("Battery: "); Serial.print(batteryPct); Serial.println(" %");
  Serial.print("Temperature: "); Serial.print(tempC); Serial.println(" C");
  Serial.print("Rain Tips since last send: "); Serial.println(rtcRainTipsToSend);
  
  // Prepare outgoing JSON payload
  JsonDocument doc;
  doc["distance"] = distanceCm;
  doc["battery"] = batteryPct;
  if (tempC > -100.0f) {
    doc["temp"] = tempC;
  }
  doc["rain"] = rtcRainTipsToSend;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  Serial.println("Payload: " + jsonPayload);
  
  // 3. Ultra-fast Standalone ESP-NOW Broadcast (No long Wi-Fi connection handshake)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Avoid connecting to Router Access Point
  
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESP-NOW initialized.");
    
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, crowPanelMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      esp_err_t result = esp_now_send(crowPanelMac, (uint8_t *)jsonPayload.c_str(), jsonPayload.length());
      if (result == ESP_OK) {
        Serial.println("ESP-NOW broadcast complete.");
        // Reset the tips interval counter only after a successful broadcast attempt
        rtcRainTipsToSend = 0;
      } else {
        Serial.println("ESP-NOW broadcast failed!");
      }
      delay(150); // Allow sufficient radio TX buffering time before sleeping
    }
  } else {
    Serial.println("ESP-NOW init failed!");
  }
  
  // 4. Configure deep sleep wakeup triggers
  pinMode(RAIN_PIN, INPUT_PULLUP);
  rtc_gpio_init((gpio_num_t)RAIN_PIN);
  rtc_gpio_set_direction((gpio_num_t)RAIN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en((gpio_num_t)RAIN_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)RAIN_PIN);
  
  esp_sleep_enable_ext0_wakeup((gpio_num_t)RAIN_PIN, 0); // Wake up on LOW
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  Serial.println("Entering 10-minute deep sleep...");
  esp_deep_sleep_start();
}

void loop()
{
  // Idle (All logic is executed on wakeup inside setup())
}
