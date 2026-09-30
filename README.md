# Cistern Water Level Monitor

This repository contains the firmware and hardware documentation for the "Cistern Water Level Monitor" project. This system utilizes a solar-powered outdoor ESP32 sensor node to track cistern water levels, localized rainfall, and ambient temperature. The data is transmitted to an indoor e-ink dashboard and integrates directly with Apple HomeKit/Google Home.

## System Architecture

The project is split into two primary nodes:
*   **Node 1 (Outdoor Publisher):** A solar-powered ELEGOO ESP32 that collects sensor data, implements deep sleep cycles for power efficiency, and publishes JSON payloads via Wi-Fi (HomeKit) or ESP-NOW (fallback).
*   **Node 2 (Indoor Subscriber):** An ELECROW CrowPanel 5.79" E-Ink Display (ESP32-S3) that acts as an asynchronous ESP-NOW receiver, parsing the JSON payloads and updating the UI dashboard.

## Hardware Manifest

### Microcontrollers
*   **ELEGOO ESP32 Development Board (USB-C):** Outdoor publisher.
*   **ELECROW CrowPanel 5.79" E-Ink Display:** Indoor subscriber.

### Sensors
*   **JSN-SR04T Waterproof Ultrasonic Sensor:** Measures distance to the water surface.
*   **MISOL Tipping Bucket Rain Gauge:** Tracks localized rainfall (requires hardware interrupt and debounce logic).
*   **DROK DS18B20 Waterproof Temperature Sensor:** Tracks ambient cistern temperatures (uses 1-Wire protocol and 4.7kΩ pull-up resistor).

### Power & Telemetry (Outdoor Node)
*   **EverExceed 5W Solar Panel (5V)**
*   **AEDIKO TP4056 (Type-C) Charging Module**
*   **18650 Batteries & Holder:** Wired in parallel (3.7V).
*   **1N5819 Schottky Diode:** Prevents nighttime battery drain to the solar panel.
*   **Voltage Divider:** Two 100kΩ (1%) resistors to scale the 4.2V max battery down to <3.3V for the ESP32 ADC pin.

### Enclosure
*   **RTHIEAI IP67 Junction Box** with PG7 Cable Glands.
*   **EPLZON Solderable Perfboard** for permanent installation.

## Wiring Guide (Outdoor Node)

**Power Pipeline:**
1.  Solar Panel (+) -> 1N5819 Diode (silver stripe out) -> TP4056 IN+
2.  Solar Panel (-) -> TP4056 IN-
3.  TP4056 B+ / B- -> 18650 Battery Holder
4.  TP4056 OUT+ -> ESP32 5V (VIN)
5.  TP4056 OUT- -> ESP32 GND

**Voltage Divider (Battery Telemetry):**
*   TP4056 B+ -> 100kΩ Resistor A -> **ESP32 Pin 34** -> 100kΩ Resistor B -> GND

**Sensors:**
*   **JSN-SR04T:** 5V -> ESP32 5V, GND -> ESP32 GND, Trig -> Pin 5, Echo -> Pin 18
*   **Rain Gauge:** Wire 1 -> GND, Wire 2 -> Pin 27
*   **DS18B20:** Signal -> GPIO (with 4.7kΩ pull-up to 3.3V), VCC -> 3.3V, GND -> GND

## Software Requirements

### Outdoor Node (ESP32)
*   **HomeSpan / ESP HomeKit SDK:** For native Apple HomeKit integration.
*   **ESP-NOW:** For direct device-to-device communication without a router.
*   **ArduinoJSON:** For packing sensor data into a structured payload.

### Indoor Node (CrowPanel ESP32-S3)
*   **GxEPD2:** For driving the e-ink display.
*   **Adafruit_GFX:** For drawing UI elements, shapes, and text.
*   **ESP-NOW:** Asynchronous receiver callbacks.

## Setup & Deployment

1.  **Bench Testing:** Build the circuit on a solderless breadboard first. Verify sensor readings and strictly test the voltage divider output (<3.3V) with a multimeter *before* connecting to ESP32 Pin 34.
2.  **Firmware Flash:** Update the CrowPanel MAC address in the outdoor node's firmware to establish the ESP-NOW pairing. 
3.  **Assembly:** Transfer the verified circuit to the perfboard and solder all connections.
4.  **Weatherproofing:** Mount inside the IP67 box and seal all cable gland entries with silicone sealant.