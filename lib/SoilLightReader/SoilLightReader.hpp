#ifndef SOIL_LIGHT_READER_HPP
#define SOIL_LIGHT_READER_HPP

#define PCF8591_I2C_ADDRESS 0x48

#include <PCF8591.h>

class SoilLightReader
{
private:
    PCF8591 pcf8591 = PCF8591(PCF8591_I2C_ADDRESS);

    unsigned long updateInterval;
    unsigned long lastUpdateTime;

    int soilMoisture;
    int lightLevel;

    bool isReady = false;

public:
    SoilLightReader() { }
    
    SoilLightReader(int sda, int scl, unsigned long updateInterval = 500)
    {
        pcf8591 = PCF8591(PCF8591_I2C_ADDRESS, sda, scl);
        this->updateInterval = updateInterval;
    }

    void Begin()
    {
        pcf8591.begin();
    }

    void Update()
    {
        if (millis() - lastUpdateTime > updateInterval)
        {
            auto inputs = pcf8591.analogReadAll();
            lightLevel = 255 - inputs.ain0;
            soilMoisture = 255 - inputs.ain1;

            isReady = true;
            lastUpdateTime = millis();
        }
    }

    bool IsReady() { return isReady; }

    int GetLightLevel() { return lightLevel; }
    int GetSoilMoisture() { return soilMoisture; }
};

#endif