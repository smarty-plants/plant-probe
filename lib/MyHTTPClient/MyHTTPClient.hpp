#ifndef MY_HTTP_CLIENT_HPP
#define MY_HTTP_CLIENT_HPP

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <EEPROM.h>
#include <WiFiClient.h>

#include <Preferences.hpp>

#define API_PORT "8000"
#define API_URL(address, path) ("http://" + (address) + ":" + (API_PORT) + (path))

class MyHTTPClient 
{
private:
    WiFiClient wifiClient;
    HTTPClient httpClient;

public:
    MyHTTPClient()
    {

    }

    void Begin(String address, String apiEndpoint) 
    {
        httpClient.begin(wifiClient, API_URL(address, apiEndpoint));
    }

    void End() 
    {
        httpClient.end();
    }

    void ClearStoredCredentials() 
    {
        preferences.ClearHTTPCredentials();
        preferences.Save();
    }

    String FindServer(String currIP)
    {
        IPAddress ipAddress;
        ipAddress.fromString(currIP);
       
        for (int i = 1; i < 255; i++)
        {
            String ip_ = String(ipAddress[0]) + "." + String(ipAddress[1]) + "." + String(ipAddress[2]) + "." + String(i);

            Serial.printf("Trying %s...\n", ip_.c_str());

            Begin(ip_, "/api/");
            int httpResponseCode = httpClient.GET();
            End();

            if (httpResponseCode >= 200 && httpResponseCode < 300) 
            {
                Serial.printf("Found server at %s!\n", ip_.c_str());
                return ip_;
            }
        }
        return "";
    }

    String RequestUUID()
    {
        auto credentials = preferences.GetHTTPCredentials();
        Begin(credentials.ip, "/api/probe/create/");
        int httpResponseCode = httpClient.POST("");

        if(httpResponseCode >= 200 && httpResponseCode < 300) 
        {
            String response = httpClient.getString();
            DynamicJsonDocument jsonResponse(1024);
            deserializeJson(jsonResponse, response);

            String probeId = jsonResponse["probe_id"];
            End();

            Serial.println("Obtained UUID: " + probeId);
            return probeId;
        }
        else
        {
            Serial.printf("Couldn't obtain UUID. You need to enter it manually using server-info {ip} {uuid}\n(error code %d)\n", httpResponseCode);
            End();
            return "";
        }
    }
};

#endif