#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <WiFiCredentials.hpp>
#include <HTTPCredentials.hpp>

#define WIFI_CREDENTIALS_SAVED 0b10000000
#define HTTP_CREDENTIALS_SAVED 0b01000000

struct Preferences 
{
    WiFiCredentials wifiCredentials;
    HTTPCredentials httpCredentials;
};

class PreferencesManager
{
private:
    Preferences preferences;
    unsigned char saveFlags = 0b00000000;

public:
    WiFiCredentials GetWiFiCredentials() { return preferences.wifiCredentials; }
    HTTPCredentials GetHTTPCredentials() { return preferences.httpCredentials; }

    void SaveWiFiCredentials(WiFiCredentials credentials)
    {
        preferences.wifiCredentials = credentials;
        saveFlags |= WIFI_CREDENTIALS_SAVED;
    }

    void SaveHTTPCredentials(HTTPCredentials credentials)
    {
        preferences.httpCredentials = credentials;
        saveFlags |= HTTP_CREDENTIALS_SAVED;
    }

    
    void SaveHTTPCredentials(String ip = "", String uuid = "") 
    {
        HTTPCredentials credentials;
        strncpy(credentials.ip, ip.c_str(), 32);
        strncpy(credentials.uuid, uuid.c_str(), 64);
        SaveHTTPCredentials(credentials);
        Save();
    }

    void SaveIP(String ip) 
    { 
        auto credentials = GetHTTPCredentials();
        SaveHTTPCredentials(ip, String(credentials.uuid)); 
    }

    void SaveUUID(String uuid) 
    { 
        auto credentials = GetHTTPCredentials();
        SaveHTTPCredentials(String(credentials.ip), uuid); 
    }

    void ClearWiFiCredentials() 
    {
        saveFlags &= ~WIFI_CREDENTIALS_SAVED;
    }

    void ClearHTTPCredentials() 
    {
        saveFlags &= ~HTTP_CREDENTIALS_SAVED;
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
    bool AreHTTPCredentialsSet() { return (saveFlags & HTTP_CREDENTIALS_SAVED) != 0; }
};

static PreferencesManager preferences;

#endif