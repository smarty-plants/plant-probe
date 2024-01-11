#ifndef SENSOR_READER_HPP
#define SENSOR_READER_HPP

#include <DHT11Reader.hpp>
#include <SoilLightReader.hpp>

class SensorReader
{
private:
    DHT11Reader dht11Reader;
    SoilLightReader soilLightReader;

public:
    SensorReader(int dht11Pin, int sdaPin, int sclPin)
    {
        dht11Reader = DHT11Reader(dht11Pin);
        soilLightReader = SoilLightReader(sdaPin, sclPin);
    }

    void Begin()
    {
        dht11Reader.Begin();
        soilLightReader.Begin();
    }

    void Update()
    {
        dht11Reader.Update();
        soilLightReader.Update();
    }

    bool IsDHTReady() { return dht11Reader.IsReady(); }
    bool IsSoilLightReady() { return soilLightReader.IsReady(); }

    float GetTemperature() { return dht11Reader.GetTemperature(); }                                 // From 0 to 60 (Celsius)
    float GetHumidity() { return dht11Reader.GetHumidity(); }                                       // From 0% to 100%
    float GetSoilMoisture() { return (float)soilLightReader.GetSoilMoisture() / 255.0f * 100.0f; }  // From 0% to 100%
    float GetLightLevel() { return (float)soilLightReader.GetLightLevel() / 255.0f * 100.0f; }      // From 0% to 100%
};

#endif