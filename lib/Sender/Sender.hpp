#ifndef SENDER_HPP
#define SENDER_HPP

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

    void Begin()
    {
        isBegin = true;
        webSockets.begin(ip, 8000, "/ws/probe/" + uuid + "/");
    }

    void Update()
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

    // void StoreUUID(String newUuid) 
    // {
    //     HTTPCredentials credentials;
    //     newUuid.toCharArray(credentials.uuid, sizeof(credentials.uuid));
    //     preferences.SaveHTTPCredentials(credentials);
    //     preferences.Save();
    // }

    bool HasStoredUUID() { return preferences.AreHTTPCredentialsSet() && strcmp(preferences.GetHTTPCredentials().uuid, "") != 0; }

    void ClearStoredUUID()
    {
        preferences.ClearHTTPCredentials();
        preferences.Save();
    }

    void SetIP(String newIP)
    {
        ip = newIP;
    }

    void SetUUID(String newUUID)
    {
        uuid = newUUID;
    }

    bool IsReady()
    {
        return !(ip.equals("") || uuid.equals("") || !isBegin);
    }

    bool IsReadyToBegin()
    {
        return !(ip.equals("") || uuid.equals(""));
    }
};

#endif