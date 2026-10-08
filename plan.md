# Cistern Monitor Build Plan: Outdoor Node

This plan follows the README (rev 2 wiring). The node measures **water level** (JSN-SR04T ultrasonic) and **rainfall** (MISOL tipping bucket), plus battery voltage. Readings go to the indoor CrowPanel over ESP-NOW. The CrowPanel is the only device on the home Wi-Fi network. It shows the readings and passes them on to Apple Home. There is no temperature sensor.

**Current phase: Phase 01, USB-powered bench testing.**

## Parts Status

| Status       | Parts                                                                                                                                                                                                                                                        |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| In hand      | ELEGOO ESP32 (on 30-pin screw-terminal adapter), CrowPanel 5.79" e-paper, JSN-SR04T-V3.0, MISOL rain gauge, 0.96" OLED, EverExceed 5 W panel, TP4056 USB-C (protected), RTHIEAI IP67 box, PG7 glands, EPLZON mini boards, DEYUE resistor kit, Dupont jumpers |
| Ordered      | 2× 18650 cells, 2-slot parallel holder, Pololu U3V16F5 5 V boost regulator                                                                                                                                                                                   |
| Still needed | Half-size (400-point) solderless breadboard, e.g. Adafruit #64                                                                                                                                                                                               |

| Phase | What                                           | Needs                                  |
| ----- | ---------------------------------------------- | -------------------------------------- |
| 01    | USB bench test of every sensor                 | Parts in hand + breadboard             |
| 02    | Home network and ESP-NOW link                  | Router access, CrowPanel (can run now) |
| 03    | Field power stage on the bench                 | Cells, holder, U3V16F5                 |
| 04    | Solder the junction board                      | Phases 01 and 03 passing               |
| 05    | Field firmware: deep sleep, rain wake, ESP-NOW | Phases 02 and 04                       |
| 06    | Enclosure and install at the cistern           | Phase 05                               |

## Pin Map

Wire to the **labels printed next to the screw terminals**. Firmware uses the GPIO number, which matches the D number on this board.

| Terminal  | GPIO    | Spot     | Connects to                             |
| --------- | ------- | -------- | --------------------------------------- |
| D5        | 5       | R8       | JSN-SR04T Trig (direct)                 |
| D18       | 18      | R7       | JSN-SR04T Echo through the divider      |
| D27       | 27      | L10      | Rain gauge switch, 10 kΩ pull-up to 3V3 |
| D34       | 34      | L4       | Battery divider midpoint                |
| D21 / D22 | 21 / 22 | R5 / R2  | OLED SDA / SCL (bench only)             |
| VIN       | —       | L15      | 5 V rail                                |
| 3V3       | —       | R15      | 3.3 V rail                              |
| GND       | —       | L14, R14 | Ground                                  |

Spot counts **labeled** terminals from the antenna end: L = left block, R = right block. Each terminal block has one extra unlabeled screw at the top. Leave it empty.

---

## Phase 01: USB-Powered Bench Testing

Goal: prove every sensor works on the desk under USB power, before any soldering or field power.

### Step 1: Configure PlatformIO (`platformio.ini`)

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

### Step 2: Assemble the Bench Hardware

#### 0. Check the Terminal Adapter

Set the multimeter to continuity. Touch each terminal you will use (D5, D18, D27, D34, VIN, 3V3, GND) and then its pin on the ESP32 itself. Each pair should beep. This catches a label shifted by one on the adapter.

#### 1. Set Up the Breadboard Rails

Keep the ESP32 on its screw-terminal adapter beside the breadboard. Run jumper wires from the labeled terminals to the breadboard.

*   **5V rail:** ESP32 **VIN** (`L15`) → the + strip on one side. Over USB, VIN carries about 4.7–5.0 V. Mark this strip with tape.
*   **3.3V rail:** ESP32 **3V3** (`R15`) → the + strip on the other side.
*   **GND:** ESP32 **GND** (`L14` or `R14`) → the − strip. Add a short jumper between the two − strips so both sides share ground.
*   Never plug anything from the 5V strip into the 3V3 strip. 5 V on the 3V3 pin can damage the ESP32 and the OLED.

#### 2. Wire the JSN-SR04T Ultrasonic Sensor

*   **Power:** `5V` → 5V rail, `GND` → GND rail.
*   **Trig:** `Trig` → ESP32 **D5** (`R8`) directly. The ESP32's 3.3 V reads as HIGH on the sensor.
*   **Echo (divider required):** Powered at 5 V, Echo sends 5 V pulses, and ESP32 pins take 3.3 V at most.
    1. `Echo` → one end of a **1 kΩ** resistor.
    2. The other end of the 1 kΩ → a breadboard junction. Run a jumper from that junction to ESP32 **D18** (`R7`).
    3. From the same junction to GND, place a **2.2 kΩ** and a **22 kΩ** side by side (in parallel). Together they make exactly 2.0 kΩ, so D18 sees about 3.3 V.
*   Never connect Echo straight to D18.

#### 3. Wire the MISOL Rain Gauge

*   Cut off the RJ11 plug. Set the multimeter to continuity and tip the bucket to find the two switch wires, normally the center pair.
*   **Switch wire A** → GND rail.
*   **Switch wire B** → ESP32 **D27** (`L10`).
*   Add a **10 kΩ pull-up** from D27 to the 3.3V rail. It holds the pin HIGH, including during deep sleep later.

#### 4. Wire the OLED Debug Display (SSD1306, I2C)

*   **Power:** `VCC` → 3.3V rail, `GND` → GND rail.
*   **I2C:** `SDA` → ESP32 **D21** (`R5`), `SCL` → ESP32 **D22** (`R2`).
*   Most of these OLED modules already have I2C pull-up resistors. If the display does not start, add a 4.7 kΩ from SDA to 3V3 and another from SCL to 3V3 (both in the kit).

#### 5. Build the Battery-Sense Stand-In (Check Before Connecting)

*   **100 kΩ** from the 5V rail → a junction. A second **100 kΩ** from that junction → GND.
*   **Multimeter check:** Measure from the junction to GND. It should read about 2.4–2.5 V and **must be under 3.3 V**. Write the reading down.
*   Only then run a jumper from the junction to ESP32 **D34** (`L4`).

### Step 3: Flash the Bench Diagnostic Code

Replace `src/main.cpp` with this. It is bench-only code. The field firmware (deep sleep, ESP-NOW) comes in Phase 05.

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pins (board labels in comments)
const int TRIG_PIN = 5;   // D5
const int ECHO_PIN = 18;  // D18, via 1k / (2.2k || 22k) divider
const int RAIN_PIN = 27;  // D27, 10k pull-up to 3V3
const int BATT_PIN = 34;  // D34, 100k / 100k divider (input-only pin)

const float MM_PER_TIP      = 0.3f;     // MISOL spec; calibrate with a measured pour
const float SOUND_CM_PER_US = 0.0343f;  // speed of sound at ~20 C; calibrate at install
const float BLIND_ZONE_CM   = 25.0f;    // JSN-SR04T can't measure closer than this

// ---- Rain gauge --------------------------------------------------------
// Count one tip per switch closure. The switch stays closed for a few hundred
// ms and bounces on release, so the counter only re-arms after the pin has
// been HIGH (switch open) for 50 ms.
volatile uint32_t rainTipCount = 0;
volatile bool rainArmed = true;
bool waitingForRelease = false;
unsigned long releaseStart = 0;

void IRAM_ATTR handleRainTip() {
  if (rainArmed) {
    rainTipCount++;
    rainArmed = false;
  }
}

void serviceRainGauge() {
  if (rainArmed) return;
  if (digitalRead(RAIN_PIN) == HIGH) {
    if (!waitingForRelease) {
      waitingForRelease = true;
      releaseStart = millis();
    } else if (millis() - releaseStart >= 50) {
      waitingForRelease = false;
      rainArmed = true;
    }
  } else {
    waitingForRelease = false;
  }
}

// ---- Ultrasonic --------------------------------------------------------
float readDistanceOnceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20);                                // JSN-SR04T needs a 20 us trigger
  digitalWrite(TRIG_PIN, LOW);

  unsigned long us = pulseIn(ECHO_PIN, HIGH, 60000UL);  // 60 ms covers the 6 m maximum
  if (us == 0) return -1.0f;
  float cm = us * SOUND_CM_PER_US / 2.0f;
  return (cm < BLIND_ZONE_CM) ? -1.0f : cm;
}

// Median of 5 pings, 60 ms apart, to reject the occasional bad echo.
float readDistanceCm() {
  float r[5];
  int n = 0;
  for (int i = 0; i < 5; i++) {
    float d = readDistanceOnceCm();
    if (d > 0) r[n++] = d;
    delay(60);
  }
  if (n == 0) return -1.0f;
  for (int i = 1; i < n; i++) {                         // insertion sort
    float k = r[i];
    int j = i - 1;
    while (j >= 0 && r[j] > k) { r[j + 1] = r[j]; j--; }
    r[j + 1] = k;
  }
  return r[n / 2];
}

// ---- Battery -------------------------------------------------------------
// Average 16 calibrated ADC readings, then undo the 2:1 divider.
float readBatteryVolts() {
  uint32_t sumMv = 0;
  for (int i = 0; i < 16; i++) {
    sumMv += analogReadMilliVolts(BATT_PIN);
    delay(2);
  }
  return (sumMv / 16.0f) * 2.0f / 1000.0f;
}

// ---- Report ----------------------------------------------------------------
void report() {
  float dist = readDistanceCm();
  float batt = readBatteryVolts();
  uint32_t tips = rainTipCount;
  float rainMm = tips * MM_PER_TIP;

  if (dist < 0) Serial.printf("Dist: no echo | ");
  else          Serial.printf("Dist: %.1f cm | ", dist);
  Serial.printf("Batt: %.2f V | Rain: %.1f mm (%lu tips)\n", batt, rainMm, (unsigned long)tips);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BENCH DIAGNOSTIC");          // lines kept <= 21 chars (128 px)
  display.setCursor(0, 16);
  if (dist < 0) display.println("Dist: no echo");
  else          display.printf("Dist: %.1f cm\n", dist);
  display.setCursor(0, 32);
  display.printf("Batt: %.2f V\n", batt);
  display.setCursor(0, 48);
  display.printf("Rain: %.1f mm (%lu)\n", rainMm, (unsigned long)tips);
  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT_PULLUP);  // external 10k does the real work
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), handleRainTip, FALLING);

  Wire.begin(21, 22);               // D21 = SDA, D22 = SCL
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found at 0x3C - check wiring");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Cistern Node Bench");
    display.display();
  }
}

void loop() {
  serviceRainGauge();

  static unsigned long lastReport = 0;
  if (millis() - lastReport >= 1000) {
    lastReport = millis();
    report();
  }
  delay(5);
}
```

### Step 4: Validate

1.  **Battery-sense math:** The reported `Batt` should be about **2 × the junction voltage** you wrote down, within about 0.1 V. Over USB that is usually 4.7–5.0 V.
2.  **Ultrasonic:** Aim the probe at a wall more than 25 cm away and move it. The distance should track a tape measure within a couple of centimeters. Inside 25 cm it should show "no echo" (the blind zone).
3.  **Rain gauge:**
    *   Tip the bucket by hand 10 times. The count should go up by exactly 10.
    *   Tip it slowly and hold it halfway. It should still count only once per tip.
4.  **Display:** All three readings refresh about once a second on the OLED and in the Serial Monitor.

**Phase 01 is done when** all four checks pass.

---

## Phase 02: Home Network and ESP-NOW Link

This runs on USB power and doesn't need the ordered parts, so it can happen now. Only the CrowPanel joins Wi-Fi. The outdoor node never does. It sends its readings to the CrowPanel over ESP-NOW, a direct radio link.

### Step 1: Router Settings

1.  **2.4 GHz band:** Confirm the IoT network has one. ESP32 radios can't use 5 GHz. If the router uses one network name for both bands, that's fine.
2.  **Fixed channel:** Set the 2.4 GHz channel to **1, 6, or 11** instead of auto, and write the number down.
    *   While the CrowPanel is joined to Wi-Fi, its radio stays on the router's channel.
    *   ESP-NOW only works when the outdoor node transmits on that same channel.

### Step 2: Keep Credentials Out of GitHub

In each repo:
*   Create `include/secrets.h` and add it to `.gitignore`.
*   Commit a `secrets.example.h` with the same lines and placeholder values.

CrowPanel repo (`cistern-water-level-monitor-eink-display`):

```cpp
#pragma once
#define WIFI_SSID     "your-network-name"   // exactly as the router shows it
#define WIFI_PASSWORD "your-password"
```

Outdoor node repo (`cistern-water-level-monitor`):

```cpp
#pragma once
#define WIFI_SSID     "your-network-name"   // used only to find the channel; the node never joins
#define WIFI_CHANNEL  6                     // the fixed channel from Step 1
#define CROWPANEL_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // printed by the CrowPanel in Step 3
```

### Step 3: CrowPanel Joins Wi-Fi (Receiver Test)

Flash this test sketch to the CrowPanel. Keep the board and USB-serial settings already in your display repo's `platformio.ini`.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "secrets.h"

// Arduino-ESP32 2.x signature. On core 3.x the first argument is
// const esp_now_recv_info_t *info, and the sender MAC is info->src_addr.
void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  Serial.printf("From %02X:%02X:%02X:%02X:%02X:%02X  %.*s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len, (const char *)data);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // Wi-Fi power saving can drop ESP-NOW packets
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf("\nJoined %s on channel %d, IP %s\n",
                WIFI_SSID, WiFi.channel(), WiFi.localIP().toString().c_str());
  Serial.printf("CrowPanel MAC for the outdoor node: %s\n", WiFi.macAddress().c_str());

  if (esp_now_init() != ESP_OK) Serial.println("ESP-NOW init failed");
  esp_now_register_recv_cb(onRecv);
}

void loop() { delay(1000); }
```

Check the Serial Monitor:
*   The channel it prints must match the router setting from Step 1.
*   Copy the MAC address into the outdoor node's `secrets.h`. For example, `A0:B7:65:12:34:56` becomes `{0xA0, 0xB7, 0x65, 0x12, 0x34, 0x56}`.

### Step 4: Outdoor Node Sends Over ESP-NOW (Sender Test)

Save the Phase 01 `src/main.cpp` as `bench_main.txt` so it doesn't get compiled. Then flash this to the bench ESP32:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include "secrets.h"

uint8_t crowPanelMac[] = CROWPANEL_MAC;
uint8_t channel = WIFI_CHANNEL;
volatile bool sendDone = false;
volatile bool sendOk = false;

// Arduino-ESP32 2.x signature (the official PlatformIO espressif32 platform).
void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  sendOk = (status == ESP_NOW_SEND_SUCCESS);
  sendDone = true;
}

// Put the radio on the router's channel and register the CrowPanel as the peer.
void startEspNow(uint8_t ch) {
  esp_now_deinit();
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_send_cb(onSent);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, crowPanelMac, 6);
  peer.channel = ch;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

// Find the router's current channel without joining the network.
int findRouterChannel() {
  int n = WiFi.scanNetworks();
  int ch = -1;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == WIFI_SSID) { ch = WiFi.channel(i); break; }
  }
  WiFi.scanDelete();
  return ch;
}

bool sendJson(const char *buf, size_t len) {
  sendDone = false;
  if (esp_now_send(crowPanelMac, (const uint8_t *)buf, len) != ESP_OK) return false;
  unsigned long t0 = millis();
  while (!sendDone && millis() - t0 < 200) delay(1);  // wait for the CrowPanel's ACK
  return sendOk;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // radio on, but never connects to the router
  WiFi.disconnect();
  startEspNow(channel);
}

void loop() {
  static uint32_t seq = 0;
  StaticJsonDocument<200> doc;  // ESP-NOW payloads max out at 250 bytes
  doc["seq"] = seq;
  doc["uptime_s"] = millis() / 1000;
  char buf[200];
  size_t len = serializeJson(doc, buf, sizeof(buf));

  if (sendJson(buf, len)) {
    Serial.printf("Sent seq %lu on channel %d\n", (unsigned long)seq, channel);
  } else {
    int ch = findRouterChannel();
    if (ch > 0) channel = ch;
    startEspNow(channel);  // the scan moves the radio, so always reset the channel
    Serial.printf("Send failed. Router found on channel %d. Retrying.\n", ch);
  }
  seq++;
  delay(2000);
}
```

### Step 5: Validate

1.  **CrowPanel:** A packet arrives every 2 seconds and `seq` goes up by one each time.
2.  **Outdoor node:** It prints `Sent seq N on channel C` with no failures.
3.  **Channel recovery:**
    *   Change the router's channel, for example from 6 to 11, and wait for the CrowPanel to rejoin.
    *   The outdoor node should report a failed send, find the new channel and carry on.
    *   Set the router back afterward.

### Step 6: Apple Home (HomeSpan on the CrowPanel)

This can wait until after Phase 05.

*   **Wi-Fi:** Add HomeSpan to the CrowPanel firmware. HomeSpan manages the Wi-Fi connection itself, using the same credentials.
*   **ESP-NOW:** Start it only after Wi-Fi is connected, and keep power saving off.
*   **Pairing:** Pair the CrowPanel in the Home app with its setup code.
*   **Open decisions:**
    *   HomeKit has no water-level or rainfall sensor type, so we need to decide how those readings appear in the Home app.
    *   HomeSpan works with Apple Home only. Google Home would need a different approach.

**Phase 02 is done when** the CrowPanel stays joined to Wi-Fi and receives the outdoor node's packets on the fixed channel.

---

## Phase 03: Field Power Stage on the Bench

Do this when the cells, holder and U3V16F5 arrive. Check each item before it connects to anything else.

### Checks on Arrival

1.  **Cells:** Each 18650 reads within about 0.1 V of the other. Charge the lower one first if they differ.
2.  **Holder:** With both cells in, the leads read **≤ 4.2 V**. A reading of about 7–8 V means it is wired in series. Don't use it.
3.  **U3V16F5:** Solder on the loose header pins. It has no reverse-polarity protection, so double-check VIN and GND before applying power.

### Wire the Power Stage

1.  **Solar panel → TP4056:** If the panel came with a USB-C adapter, plug it into the TP4056's USB-C port for now. The plug only gets cut off in Phase 06, when the cable goes through its gland.
2.  **Panel voltage:** In full sun, with no cells connected yet, measure across the TP4056 input. It must read **≤ 6.0 V**, because the TP4056's absolute maximum is 6.5 V.
3.  **Cells → TP4056:** Holder leads → TP4056 **B+ / B−**.
4.  **TP4056 → boost:** **OUT+** → U3V16F5 **VIN**, and **OUT−** → U3V16F5 **GND**.
5.  **Boost output:** With nothing on the output, **VOUT** reads **4.8–5.2 V**.

### Run the Bench Circuit on Battery Power

1.  **Unplug USB from the ESP32.** From here on, read the values on the OLED.
2.  **5 V source:** U3V16F5 **VOUT** → the breadboard 5V strip, and U3V16F5 GND / TP4056 OUT− → the GND strip. Leave the VIN jumper in place. The strip now feeds the ESP32 instead of the ESP32 feeding the strip.
3.  **Battery divider:** Move the top 100 kΩ from the 5V strip to **TP4056 B+**. `Batt` on the OLED should match a multimeter reading at B+ within about 0.1 V.
4.  **Ground rule:** Load grounds go to **OUT−**, never to B−. B− and IN− connect only to the cells and the panel.
5.  **Charging:** In sun, the TP4056 charge LED lights, and it changes color when the cells are full.
6.  **Range check:**
    *   Flash the Phase 02 sender test.
    *   Carry the battery-powered setup to where the cistern box will go.
    *   Confirm the CrowPanel still receives every packet.
    *   Put the Phase 01 code back afterward.

**Phase 03 is done when** the bench circuit runs from the cells with all Phase 01 checks still passing.

---

## Phase 04: Solder the Junction Board

One EPLZON board (17 columns × rows A–J) replaces the breadboard. In each numbered column, holes A–E are connected and holes F–J are connected. Each of those 5-hole strips acts as a small rail.

### Before Soldering

*   Set the multimeter to continuity. Holes in the same column and half (e.g. 9A and 9D) beep. Holes in different halves (9A and 9J) don't.
*   Hold the board component side up, column 1 on the left, row J at the top. See `junction-board-layout.png`.
*   Wire colors: red = 5V, orange = 3V3, black = GND.

### Resistors and Jumper

Lay the resistors flat with their legs 4 holes apart.

| Part         | Holes     | Bands (1% metal film)          |
| ------------ | --------- | ------------------------------ |
| R1 · 1 kΩ    | 1A – 5A   | brown black black brown brown  |
| R2a · 2.2 kΩ | 5B – 9B   | red red black brown brown      |
| R2b · 22 kΩ  | 5C – 9C   | red red black red brown        |
| R3 · 10 kΩ   | 13A – 17A | brown black black red brown    |
| R4 · 100 kΩ  | 1J – 5J   | brown black black orange brown |
| R5 · 100 kΩ  | 5I – 9I   | brown black black orange brown |
| Jumper wire  | 9E – 9F   | joins the two GND strips       |

### Wires

| Strip   | Hole | Wire to                             |
| ------- | ---- | ----------------------------------- |
| 5V      | 13F  | U3V16F5 VOUT                        |
| 5V      | 13G  | ESP32 VIN terminal                  |
| 5V      | 13H  | JSN-SR04T 5V                        |
| 3V3     | 17C  | ESP32 3V3 terminal                  |
| GND     | 9J   | TP4056 OUT−                         |
| GND     | 9H   | U3V16F5 GND                         |
| GND     | 9G   | ESP32 GND terminal                  |
| GND     | 9D   | JSN-SR04T GND                       |
| GND     | 9A   | Rain gauge wire A                   |
| B+      | 1H   | TP4056 B+ (second wire on that pad) |
| Battery | 5G   | ESP32 D34 terminal                  |
| Echo in | 1C   | JSN-SR04T Echo                      |
| Echo    | 5D   | ESP32 D18 terminal                  |
| Rain    | 13E  | Rain gauge wire B                   |
| Rain    | 13D  | ESP32 D27 terminal                  |

These go direct, not through the junction board:
*   JSN-SR04T Trig → D5 terminal
*   TP4056 OUT+ → U3V16F5 VIN
*   Panel → TP4056 IN+/IN−
*   Holder → TP4056 B+/B−

### After Soldering

1.  **No shorts:** With nothing connected, continuity between the 5V (13F), 3V3 (17C) and GND (9A) strips must **not** beep.
2.  **Wire it up:** Connect the board to the ESP32 terminals and the power stage, with USB unplugged.
3.  **Retest:** Temporarily add the OLED wires to the 3V3, GND, D21 and D22 terminals alongside the existing wires. Repeat the Phase 01 checks.

---

## Phase 05: Field Firmware (Plan Next)

To be designed in detail after Phase 04. The agreed behavior:

*   **Wake cycle:** Wake on a timer, take a median-of-5 distance reading and a battery reading, send them, then go back to deep sleep.
*   **Rain wake:** D27 (GPIO 27, an RTC pin) wakes the board on LOW (`ext0`).
    *   Add each tip to a counter kept in `RTC_DATA_ATTR` memory.
    *   Wait for the pin to return HIGH before sleeping again.
    *   Report the running total on the next timer wake.
*   **Transport:** JSON payload over ESP-NOW to the CrowPanel, based on the Phase 02 sender code. The node never joins Wi-Fi.
    *   It transmits on the router's fixed channel.
    *   After a failed send, it rescans for the channel and keeps it in `RTC_DATA_ATTR` memory so it survives deep sleep.
*   **Calibration at install:**
    *   Set `SOUND_CM_PER_US` so a known tape-measured distance reads correctly.
    *   Confirm `MM_PER_TIP` with a measured pour of water through the gauge.
*   **Power check:** Measure deep-sleep current in series with the battery. Expect roughly 15–20 mA with this dev board (its regulator, power LED and the sensor's idle draw stay on).

---

## Phase 06: Enclosure and Install

1.  **Glands:** Drill three 1/2" (12.5 mm) holes for PG7 glands: solar cable, ultrasonic probe cable, rain gauge cable.
2.  **Cables:**
    *   Pass each cable through its gland first: dome nut, then seal, then the gland body.
    *   Then cut the solar USB plug and solder the panel wires to TP4056 IN+/IN−. Check polarity with the multimeter first: normally red = +, black = −. Insulate any unused data wires.
    *   The rain gauge's RJ11 plug is already off from Phase 01.
3.  **Mounting plate:** Mount the ESP32 adapter, junction board, TP4056, U3V16F5 and cell holder on the box's mounting plate.
4.  **Ultrasonic probe:** Mount it at least 25 cm above the highest (overflow) water level, pointing straight down and clear of pipes and walls.
5.  **Seal:** Tighten the glands and seal them with silicone.
6.  **Calibrate:** Run the Phase 05 calibration.