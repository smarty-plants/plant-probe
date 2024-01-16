#ifndef SENDER_HPP
#define SENDER_HPP

#define CREDENTIALS_STORED 0x80

#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <HTTPCredentials.hpp>

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
        webSockets.begin(ip, 8000, "/ws/probe/" + uuid + "/");
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

    void StoreUUID(String newUuid) 
    {
        HTTPCredentials credentials;
        newUuid.toCharArray(credentials.uuid, sizeof(credentials.uuid));
        EEPROM.begin(sizeof(HTTPCredentials) + 1);
        EEPROM.write(sizeof(WiFiCredentials) + 1, CREDENTIALS_STORED);
        EEPROM.put(sizeof(WiFiCredentials) + 2, credentials);
        EEPROM.end();
    }

    bool HasStoredUUID()
    {
        EEPROM.begin(sizeof(HTTPCredentials) + 1);
        if (EEPROM.read(sizeof(WiFiCredentials) + 1) != CREDENTIALS_STORED)
        {
            EEPROM.end();
            return false;
        }

        HTTPCredentials credentials;
        EEPROM.get(sizeof(WiFiCredentials) + 2, credentials);
        EEPROM.end();
        uuid = String(credentials.uuid);
        Serial.println("Załadowano UUID z pamięci!");

        return true;   
    }

    void ClearStoredUUID()
    {
        EEPROM.begin(sizeof(HTTPCredentials) + 1);
        EEPROM.write(sizeof(WiFiCredentials) + 1, 0x00);
        EEPROM.put(sizeof(WiFiCredentials) + 2, HTTPCredentials());
        EEPROM.end();
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