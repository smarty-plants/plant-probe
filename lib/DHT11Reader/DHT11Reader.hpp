#ifndef DHT_READER_HPP
#define DHT_READER_HPP

#include <DHTesp.h>

class DHT11Reader
{
private:
    DHTesp dht;

    int pin;

    float temperature;
    float humidity;

    bool isReady;

    unsigned long lastUpdateTime;

public:
    DHT11Reader() { }

    DHT11Reader(int pin) 
    {
        this->pin = pin;
    }

    void Begin()
    {
        dht.setup(pin, DHTesp::DHT11);
    }

    void Update()
    {
        if (millis() - lastUpdateTime > (unsigned long)dht.getMinimumSamplingPeriod())
        {
            temperature = dht.getTemperature();
            humidity = dht.getHumidity();

            isReady = true;
            lastUpdateTime = millis();
        }
    }

    bool IsReady() { return isReady; }

    float GetTemperature() { return temperature; }
    float GetHumidity() { return humidity; }
};

#endif