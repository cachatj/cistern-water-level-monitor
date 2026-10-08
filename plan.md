Congratulations on receiving all your hardware shipments and setting up your repository in VSCode with PlatformIO. 

We will start today with **Phase 01: USB-Powered Bench Testing**. The goal of this phase is to validate all sensors—the JSN-SR04T ultrasonic transducer, MISOL tipping bucket, OLED debug screen, and battery telemetry circuit—on your desk using USB power before soldering or connecting field power.

---

### Step 1: Configure Your PlatformIO Project (`platformio.ini`)

In your VSCode PlatformIO workspace for **Node 1 (Outdoor Publisher)**, open your `platformio.ini` file and configure the board environment and core libraries:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/Adafruit GFX Library @ ^1.11.5
    bblanchon/ArduinoJson @ ^6.21.3
```

---

### Step 2: Assemble the Bench Test Hardware (Rev-2 Wiring)

Keep your ELEGOO ESP32 on its **30-pin screw-terminal adapter** beside your solderless breadboard, running jumper wires from the labeled screw terminals to the breadboard rails.

#### 1. Establish Power Rails on the Breadboard
*   **5V Rail:** Connect a jumper from ESP32 **VIN** (Terminal `L15`) to the breadboard red power strip (receives 5V directly from USB-C).
*   **3.3V Rail:** Connect a jumper from ESP32 **3V3** (Terminal `R15`) to the secondary power strip.
*   **GND Rail:** Connect a jumper from ESP32 **GND** (Terminal `L14` or `R14`) to the blue ground strip, joining both ground rails together.

#### 2. Wire the JSN-SR04T Ultrasonic Sensor
*   **Power:** Wire `5V` to the 5V rail and `GND` to the GND rail.
*   **Trig (Trigger):** Connect `Trig` directly to ESP32 **D5** (GPIO 5, Terminal `R8`).
*   **Echo (Voltage Divider Required):** Because the JSN-SR04T outputs 5V Echo pulses and ESP32 GPIOs are 3.3V max, pass Echo through a **1 kΩ / 2 kΩ divider**:
    1. Connect `Echo` to one side of a **1 kΩ resistor**.
    2. Connect the other side of the 1 kΩ resistor to a shared breadboard junction node and wire this junction to ESP32 **D18** (GPIO 18, Terminal `R7`).
    3. From that same junction node, place a **2.2 kΩ resistor** and a **22 kΩ resistor in parallel** going to GND (which creates an exact 2.0 kΩ resistance, scaling 5V down to ~3.33V).

#### 3. Wire the MISOL Rain Gauge
*   Cut off the RJ11 telephone plug and use a multimeter in continuity mode while tipping the bucket to identify the two switch conductors.
*   Connect **Switch Wire A** to the GND rail.
*   Connect **Switch Wire B** to ESP32 **D27** (GPIO 27, Terminal `L10`).
*   Add a **10 kΩ pull-up resistor** between D27 and the 3.3V rail to hold the pin HIGH during deep sleep.

#### 4. Wire the OLED Debug Display (SSD1306 I2C)
*   **Power:** `VCC` to 3.3V rail, `GND` to GND rail.
*   **I2C Data Bus:** `SDA` to ESP32 **D21** (GPIO 21, Terminal `R5`) and `SCL` to ESP32 **D22** (GPIO 22, Terminal `R2`).

#### 5. Build the Battery Telemetry Stand-In (Safety Check First)
*   Connect a **100 kΩ resistor** from the 5V rail to a central junction node.
*   Connect a second **100 kΩ resistor** from that junction node to GND.
*   **CRITICAL MULTIMETER CHECK:** Measure the DC voltage between the central junction node and GND. It must read **≈2.5V (strictly <3.3V)** before attaching the jumper wire.
*   Once verified, connect a jumper from the junction node to ESP32 **D34** (GPIO 34, Terminal `L4`).

---

### Step 3: Flash the Diagnostic Bench Test Code

Replace `src/main.cpp` in your PlatformIO project with this unified diagnostic code to test all peripherals simultaneously over the Serial Monitor and OLED display:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin Definitions
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int RAIN_PIN = 27;
const int BATT_PIN = 34;

// Rain Interrupt Tracker
volatile unsigned long rainTipCount = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100;

void IRAM_ATTR handleRainTip() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > DEBOUNCE_DELAY_MS) {
    rainTipCount++;
    lastDebounceTime = currentTime;
  }
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20); // JSN-SR04T requires 20us trigger pulse
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 300000); // 300ms timeout
  if (duration == 0) return -1.0;
  
  float distance = (duration * 0.0343) / 2.0;
  if (distance < 25.0) return -1.0; // Discard readings in ~25cm blind zone
  return distance;
}

float readBatteryVoltage() {
  uint32_t rawMv = analogReadMilliVolts(BATT_PIN);
  return (rawMv * 2.0) / 1000.0; // Reverse 1:1 voltage divider
}

void setup() {
  Serial.begin(115200);
  
  // Pin Modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  
  // Interrupt for Rain Gauge
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), handleRainTip, FALLING);

  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Cistern Node Bench");
    display.display();
  }
}

void loop() {
  float dist = readDistanceCm();
  float batt = readBatteryVoltage();
  float totalRainMm = rainTipCount * 0.3; // 0.3mm per tip

  // Output to Serial Monitor
  Serial.printf("Dist: %.1f cm | Batt: %.2f V | Rain: %.1f mm (Tips: %lu)\n", 
                dist, batt, totalRainMm, rainTipCount);

  // Output to Bench OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("--- BENCH DIAGNOSTIC ---");
  
  display.setCursor(0, 16);
  if (dist < 0) {
    display.println("Distance: OUT OF RANGE");
  } else {
    display.printf("Distance: %.1f cm\n", dist);
  }

  display.setCursor(0, 32);
  display.printf("Battery ADC: %.2f V\n", batt);

  display.setCursor(0, 48);
  display.printf("Rain: %.1f mm (%lu tips)\n", totalRainMm, rainTipCount);
  
  display.display();
  delay(1000);
}
```

---

### Step 4: Perform Validation Tests

1. **Verify Multimeter & ADC Math:** With USB connected, the code should report `Battery ADC: ~5.0 V` on the screen/serial log (reconstructed from the 2.5V junction reading).
2. **Verify Ultrasonic Sensor:** Aim the transducer probe at a wall >25 cm away. Move back and forth to confirm real-time cm distance updates, and verify that objects closer than 25 cm return "OUT OF RANGE" (validating the blind-zone handling).
3. **Verify Rain Gauge Interrupt:** Manually tip the rain bucket. Verify that each tip increments `rainTipCount` by 1 (adding 0.3 mm of rain) and that contact bouncing does not create false multiple tips.

💡 *Once all bench readings update cleanly in PlatformIO, we can test the ESP-NOW transmission to your indoor CrowPanel display!*