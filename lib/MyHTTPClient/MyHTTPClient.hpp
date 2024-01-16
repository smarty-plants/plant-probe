#ifndef MYHTPPCLIENT_HPP
#define MYHTPPCLIENT_HPP

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <iostream>
#include <EEPROM.h>
#include <WiFiClient.h>

class MyHTTPClient 
{
private:
    String ip = "";

public:
    MyHTTPClient()
    {

    }

    String requestForIP(String currIP)
    {
        WiFiClient wifiClient;
        HTTPClient http;
        IPAddress ipAddress;
        ipAddress.fromString(currIP);
        for(int i = 1; i < 255; i++)
        {
            String ip_ = String(ipAddress[0]) + "." + String(ipAddress[1]) + "." + String(ipAddress[2]) + "." + String(i);
            http.begin(wifiClient , "http://" + ip_ + ":8000/api/");
            int httpResponseCode = http.GET();
            Serial.println(ip_);
            Serial.print(".");
            http.end();
            if(httpResponseCode >= 200 && httpResponseCode < 300) 
            {
                Serial.println(ip_);
                return ip_;
            }
        }
        return "";
    }

    String requestForUUID()
    {
        WiFiClient wifiClient;
        HTTPClient http;
        http.begin(wifiClient , "http://" + ip + ":8000/api/probe/create/");
        int httpResponseCode = http.POST("");

        if(httpResponseCode >= 200 && httpResponseCode < 300) 
        {
            String response = http.getString();
            DynamicJsonDocument jsonResponse(1024);
            deserializeJson(jsonResponse, response);

            String probeId = jsonResponse["probe_id"];
            http.end();
            Serial.println("UUID POBRANE: " + probeId);
            return probeId;
        }
        else
        {
            Serial.println("NIE UDAŁO SIĘ POBRAĆ UUID SONDY, WPROWADŹ UUID RĘCZNIE");
            http.end();
            return "";
        }
    }

    void setIP(String newIP)
    {
        ip = newIP;
    }

    String getIP()
    {
        return ip;
    }
};

#endif