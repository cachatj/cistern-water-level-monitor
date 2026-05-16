#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// --- CONFIGURATION ---
const char *ssid = "Forest-Enclave-IoT";
const char *password = "c@nn@b!skiss420";

// CrowPanel MAC Address
uint8_t crowPanelMac[] = {0x44, 0x1B, 0xF6, 0x95, 0x55, 0x24};

// Pin Definitions
const int trigPin = 5;
const int echoPin = 18;
const int batPin = 34;

// Deep Sleep Settings (10 Minutes)
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 600

// Global Variables
float distanceCm;
float batteryPercentage;
String jsonPayload;
esp_now_peer_info_t peerInfo;

void setup()
{
  Serial.begin(115200);

  // 1. Read JSN-SR04T Sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;

  // 2. Read Battery Voltage
  int analogValue = analogRead(batPin);
  float pinVoltage = (analogValue / 4095.0) * 3.3;
  float batteryVoltage = pinVoltage * 2;

  batteryPercentage = ((batteryVoltage - 3.2) / (4.2 - 3.2)) * 100;
  if (batteryPercentage > 100)
    batteryPercentage = 100;
  if (batteryPercentage < 0)
    batteryPercentage = 0;

  jsonPayload = "{\"distance\":" + String(distanceCm) + ",\"battery\":" + String(batteryPercentage) + "}";
  Serial.println("Payload ready: " + jsonPayload);

  // 3. Connect to Wi-Fi (For future phone access)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 10)
  {
    delay(500);
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi connected. IP: " + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("Wi-Fi connection failed.");
  }

  // 4. Transmit Data via ESP-NOW to CrowPanel
  if (esp_now_init() == ESP_OK)
  {
    memcpy(peerInfo.peer_addr, crowPanelMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK)
    {
      esp_err_t result = esp_now_send(crowPanelMac, (uint8_t *)jsonPayload.c_str(), jsonPayload.length());
      if (result == ESP_OK)
      {
        Serial.println("ESP-NOW transmission successful.");
      }
      else
      {
        Serial.println("ESP-NOW transmission failed.");
      }
      delay(500); // Give ESP-NOW time to transmit
    }
  }

  // 5. Go to Deep Sleep
  Serial.println("Entering 10-minute deep sleep...");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop()
{
}
