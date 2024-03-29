#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <Arduino.h>
#include <WiFiCredentials.hpp>
#include <HTTPCredentials.hpp>

#define WIFI_CREDENTIALS_SAVED      0b10000000
#define SERVER_IP_SAVED             0b01000000
#define PROBE_UUID_SAVED            0b00100000

struct Preferences 
{
    WiFiCredentials wifiCredentials;
    HTTPCredentials httpCredentials;

    bool autoConnectToWiFi;
    bool autoConnectToServer;
};

class PreferencesManager
{
private:
    Preferences preferences;
    unsigned char saveFlags = 0b00000000;

public:
    WiFiCredentials GetWiFiCredentials() { return preferences.wifiCredentials; }
    HTTPCredentials GetHTTPCredentials() { return preferences.httpCredentials; }
    String GetServerIP() { return GetHTTPCredentials().ip; }
    String GetProbeUUID() { return GetHTTPCredentials().uuid; }

    bool GetAutoConnectToWiFi() { return preferences.autoConnectToWiFi; }
    bool GetAutoConnectToServer() { return preferences.autoConnectToServer; }

    void SetAutoConnectToWiFi(bool value) { preferences.autoConnectToWiFi = value; }
    void SetAutoConnectToServer(bool value) { preferences.autoConnectToServer = value; }

    void SetWiFiCredentials(WiFiCredentials credentials)
    {
        preferences.wifiCredentials = credentials;
        saveFlags |= WIFI_CREDENTIALS_SAVED;
    }

    void SetServerIP(String ip)
    {
        strncpy(preferences.httpCredentials.ip, ip.c_str(), 32);
        saveFlags |= SERVER_IP_SAVED;
    }

    void SetProbeUUID(String uuid)
    {
        strncpy(preferences.httpCredentials.uuid, uuid.c_str(), 64);
        saveFlags |= PROBE_UUID_SAVED;
    }

    void SetHTTPCredentials(HTTPCredentials credentials)
    {
        SetHTTPCredentials(credentials.ip, credentials.uuid);
    }
    
    void SetHTTPCredentials(String ip = "", String uuid = "") 
    {
        SetServerIP(ip);
        SetProbeUUID(uuid);
    }

    void ClearServerIP()
    {
        saveFlags &= ~SERVER_IP_SAVED;
    }

    void ClearProbeUUID()
    {
        saveFlags &= ~PROBE_UUID_SAVED;
    }

    void ClearWiFiCredentials() 
    {
        saveFlags &= ~WIFI_CREDENTIALS_SAVED;
    }

    void ClearHTTPCredentials() 
    {
        ClearServerIP();
        ClearProbeUUID();
    }

    Preferences Load() 
    {
        EEPROM.begin(sizeof(Preferences) + 1);
        saveFlags = EEPROM.read(0);
        EEPROM.get(1, preferences);
        EEPROM.end();
        return preferences;
    }

    void Save() 
    {
        EEPROM.begin(sizeof(Preferences) + 1);
        EEPROM.write(0, saveFlags);
        EEPROM.put(1, preferences);
        EEPROM.end();
    }

    bool AreWiFiCredentialsSet() { return (saveFlags & WIFI_CREDENTIALS_SAVED) != 0; }
    bool IsServerIPSet() { return (saveFlags & SERVER_IP_SAVED) != 0; }
    bool IsProbeUUIDSet() { return (saveFlags & PROBE_UUID_SAVED) != 0; }
};

static PreferencesManager preferences;

#endif