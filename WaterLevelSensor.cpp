#include "WaterLevelSensor.h"
#include <Arduino.h>

WaterLevelSensor::WaterLevelSensor()
{
}

void WaterLevelSensor::begin()
{
    // TODO: Initialize sensor hardware/pins here
}

float WaterLevelSensor::readLevelCm()
{
    // TODO: Implement actual sensor reading logic
    // Returning a dummy mock value for early testing
    return 50.0f;
}