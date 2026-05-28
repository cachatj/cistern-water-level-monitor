#ifndef WATER_LEVEL_SENSOR_H
#define WATER_LEVEL_SENSOR_H

class WaterLevelSensor
{
public:
    WaterLevelSensor();

    // Initializes the sensor (e.g., configuring GPIO pins)
    void begin();

    // Reads and returns the current water level in centimeters
    float readLevelCm();
};

#endif