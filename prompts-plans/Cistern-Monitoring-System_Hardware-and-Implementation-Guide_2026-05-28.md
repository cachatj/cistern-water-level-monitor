Here is the comprehensive hardware manifest, component configuration guide, and step-by-step order of operations formatted for your transition into Google Antigravity.

### Hardware Manifest & Configuration Guide

**1\. ELEGOO ESP32 Development Board (USB-C)**

* **Purpose:** The primary outdoor microcontroller (Node 1\) responsible for sensor data collection and transmission 1, 2\.  
* **Connections:** 5V (VIN) and GND to power pipeline; Pin 5 and 18 to Ultrasonic Sensor; Pin 34 to Voltage Divider; Pin 27 to Rain Gauge 3-5.  
* **Software:** Requires deep sleep implementation 2\. Must integrate HomeSpan or ESP HomeKit SDK for native Apple HomeKit/Google Home Wi-Fi communication, alongside the ESP-NOW fallback protocol to transmit data directly to the indoor display if Wi-Fi fails 6\.

**2\. ELECROW CrowPanel 5.79" E-Ink Display (ESP32-S3)**

* **Purpose:** The indoor dashboard receiver (Node 2\) featuring a built-in ESP32-S3 7, 8\.  
* **Connections:** Internal SPI interface (MOSI, CLK, CS, DC, RST, BUSY) requiring exact GPIO mapping based on Elecrow's documentation 9\.  
* **Software:** Requires GxEPD2 and Adafruit\_GFX libraries for rendering the UI 10\. Runs an asynchronous ESP-NOW receiver callback to parse incoming JSON payloads 11\.

**3\. JSN-SR04T Waterproof Ultrasonic Sensor**

* **Purpose:** Measures the distance to the water surface 12, 13\.  
* **Connections:** 5V to ESP32 5V (VIN); GND to ESP32 GND; Trig to ESP32 Pin 5; Echo to ESP32 Pin 18 4, 5\.

**4\. MISOL Tipping Bucket Rain Gauge**

* **Purpose:** Tracks localized rainfall to correlate with cistern level changes 14, 15\.  
* **Connections:** One wire to ESP32 GND, the other to a free digital GPIO (e.g., Pin 27\) 3, 15\.  
* **Software:** Requires a hardware interrupt function and a software debounce algorithm (ignoring signals within 50-100ms) to prevent false multiple readings from the mechanical reed switch 15\.

**5\. DROK DS18B20 Waterproof Temperature Sensor**

* **Purpose:** Tracks ambient cistern temperatures for freeze-risk monitoring 16\.  
* **Connections:** Utilizes the 1-Wire protocol requiring a single GPIO pin and a 4.7kΩ pull-up resistor from your kit 17\.

**6\. Power Management Pipeline**

* **EverExceed 5W Solar Panel:** Supplies 5V continuous power 16, 18\. Connects to the TP4056 IN+ and IN- pads 19, 20\.  
* **1N5819 Schottky Diode:** Prevents nighttime battery drain 18, 21\. Soldered inline on the positive solar wire, with the silver stripe facing the TP4056 IN+ pad 19, 22\.  
* **AEDIKO TP4056 (Type-C):** Safely charges the 18650 batteries 23\. Connects to solar (IN), battery holder (B), and ESP32 power (OUT) 19, 20\.  
* **18650 Batteries & Holder:** Wired in parallel to maintain 3.7V while doubling capacity 24, 25\.

**7\. Voltage Divider (Battery Telemetry)**

* **Purpose:** Scales the 4.2V battery maximum down to a safe \<3.3V range for the ESP32 analog pin 21, 26\.  
* **Connections:** Two 100kΩ (1% tolerance) resistors 26, 27\. Resistor A connects to TP4056 B+, Resistor B connects to GND, and the twisted middle intersection connects to ESP32 Pin 34 5, 28\.  
* **Software:** The raw ADC reading must be multiplied by 2 in the code to reverse the divider, then mapped to a 0-100% capacity float 2\.

**8\. Housing & Assembly**

* **RTHIEAI IP67 Junction Box & PG7 Cable Glands:** Provides waterproof housing and sealed entry points for the solar and sensor wires 1, 18\.  
* **EPLZON Perfboard & Jumper Wires:** Moves the circuit from a temporary breadboard prototype to a permanent, vibration-resistant soldered layout 4, 29\.

### Suggested Order of Operations

**Phase 1: Component Bench Testing**

1. **Test Secondary Sensors:** Wire the MISOL Rain Gauge and DS18B20 Temperature sensor to your breadboard alongside the working ultrasonic sensor. Write and verify the C++ debounce logic for the rain gauge interrupt and the 1-Wire protocol for the temperature sensor.  
2. **Build Power Pipeline:** Wire the solar panel, 1N5819 diode, TP4056 charger, and 18650 batteries on the breadboard 19, 30\.  
3. **Verify Voltage Divider:** Build the 100kΩ resistor divider. Use a digital multimeter to measure the output at the center junction. Ensure it outputs \<3.3V before connecting it to ESP32 Pin 34 to avoid damaging the board 28, 31\.

**Phase 2: Software Development (Google Antigravity/VSCode)**4\.  **Node 1 (Outdoor Publisher):** Write the main ESP32 firmware. Integrate the HomeSpan library for direct Apple HomeKit communication. Implement the ESP-NOW fallback logic using the CrowPanel MAC address 2, 6\. Combine all sensor logic (Ultrasonic, Rain, Temp, Battery) into a JSON payload 2\. Implement deep sleep cycles 2.5.  **Node 2 (Indoor Subscriber):** Write the CrowPanel firmware. Map the SPI pins for the display 9\. Integrate the ESP-NOW receiver callback to parse the JSON string 11\. Implement the GxEPD2 rendering loop using the UI layout from your repository.

**Phase 3: Hardware Assembly & Deployment**6\.  **Soldering:** Transfer the working ESP32, TP4056, and voltage divider from the breadboard to the EPLZON perfboard 29, 32\. Ensure power lines (solar/battery) do not cross sensitive data lines 33.7.  **Waterproofing:** Drill holes in the RTHIEAI junction box, install the PG7 cable glands, and pass the solar and sensor wires through 4\. Apply silicone sealant around the glands for absolute IP67 integrity 4.8.  **Final Installation:** Mount the sealed box outside, drop the JSN-SR04T probe into the concrete access tube, angle the solar panel south, and plug in your indoor CrowPanel display 34\.

