#ifndef SENDER_HPP
#define SENDER_HPP

#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <iostream>

class Sender 
{
private:
    WebSocketsClient webSockets;
    String ip = "";
    String uuid = "";
    bool isBegin = false;

public:
    Sender()
    {
        
    }

    void begin()
    {
        isBegin = true;
        webSockets.begin(ip, 80, "/ws/probe/" + uuid + "/");
    }

    void loop()
    {
        webSockets.loop();
    }

    void sendMessage(String temperature, String humidity, String soilMoisture, String lightLevel)
    {
        DynamicJsonDocument json(1024);
        json["temperature"] = temperature;
        json["humidity"] = humidity;
        json["soil_moisture"] = soilMoisture;
        json["light_level"] = lightLevel;

        char message[1024];
        serializeJson(json, message);

        webSockets.sendTXT(message);
    }

    void SetIP(String newIP)
    {
        ip = newIP;
    }

    void SetUUID(String newUUID)
    {
        uuid = newUUID;
    }

    bool isReady()
    {
        return !(ip.equals("") || uuid.equals("") || !isBegin);
    }

    bool isReadyToBegin()
    {
        return !(ip.equals("") || uuid.equals(""));
    }
};

#endif