#ifndef WIFIMANAGER_HPP
#define WIFIMANAGER_HPP

#define CREDENTIALS_STORED 0x80

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Preferences.hpp>
#include <stdlib.h>

enum WiFiManagerState {
    Disconnected,
    Connecting,
    Connected,
    ConnectionTimedOut
};

class WiFiManager
{
private:
    wl_status_t wifiStatus;

    WiFiManagerState state = WiFiManagerState::Disconnected;
    WiFiManagerState prevState = WiFiManagerState::Disconnected;

    unsigned long lastUpdateTime;
    unsigned long lastDotTime;
    unsigned long lastConnectTime;

    unsigned long connectTimeout = 20000;

    unsigned long updateInterval;

    void HandleStates()
    {
        if (state == WiFiManagerState::Connecting)
        {
            if (millis() - lastDotTime >= 1000)
            {
                Serial.print(".");
                lastDotTime = millis();
            }

            if (millis() - lastConnectTime > connectTimeout)
            {
                state = WiFiManagerState::ConnectionTimedOut;
            }

            if (wifiStatus == WL_CONNECTED) 
            {
                state = WiFiManagerState::Connected;
            }
            else if (wifiStatus == WL_CONNECT_FAILED)
            {
                state = WiFiManagerState::Disconnected;
            }
        }

        if (state == WiFiManagerState::Connected)
        {
            if (wifiStatus == WL_DISCONNECTED || wifiStatus == WL_IDLE_STATUS)
            {
                state = WiFiManagerState::Disconnected;
            }
        }
    }

    void HandleStateTransitions()
    {
        if (state == WiFiManagerState::Connected && prevState == WiFiManagerState::Connecting)
        {
            Serial.printf("\nConnected to %s! Local IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        }

        if (state == WiFiManagerState::Disconnected && prevState == WiFiManagerState::Connecting)
        {
            Serial.printf("\nConnection error!\n");
        }

        if (state == WiFiManagerState::Disconnected && prevState == WiFiManagerState::Connected)
        {
            Serial.println("Disconnected!");
        }

        if (state == WiFiManagerState::ConnectionTimedOut)
        {
            Serial.println("\nCouldn't connect - timed out");
            state = WiFiManagerState::Disconnected;
        }
    }

public:
    WiFiManager(unsigned long updateInterval = 100)
    {
        this->updateInterval = updateInterval;
    }

    void SetCredentials(const char* ssid, const char* password)
    {
        WiFiCredentials credentials;
        strncpy(credentials.ssid, ssid, 32);
        strncpy(credentials.password, password, 32);
        preferences.SetWiFiCredentials(credentials);
        preferences.Save();
    }

    void ClearStoredCredentials()
    {
        preferences.ClearWiFiCredentials();
        preferences.Save();
    }

    void PrintCredentials()
    {
        auto credentials = preferences.GetWiFiCredentials();
        Serial.printf("SSID: %s\nPassword: %s\n", credentials.ssid, credentials.password);
    }

    bool Connect()
    {
        if (!preferences.AreWiFiCredentialsSet()) return false;

        auto credentials = preferences.GetWiFiCredentials();

        Serial.printf("Connecting to %s...\n", credentials.ssid);

        state = WiFiManagerState::Connecting;
        WiFi.begin(credentials.ssid, credentials.password);

        lastConnectTime = millis();

        return true;
    }

    void Disconnect()
    {
        auto credentials = preferences.GetWiFiCredentials();
        Serial.printf("Disconnecting from %s...\n", credentials.ssid);
        WiFi.disconnect();
    }

    bool IsConnected()
    {
        return state == WiFiManagerState::Connected;
    }

    bool IsDisconnected()
    {
        return state == WiFiManagerState::Disconnected;
    }

    void Update()
    {
        if (millis() - lastUpdateTime < updateInterval) return;
        
        wifiStatus = WiFi.status();

        HandleStates();
        HandleStateTransitions();

        lastUpdateTime = millis();
        prevState = state;
    }

    String GetLocalIP()
    {
        return WiFi.localIP().toString();
    }
};

#endif