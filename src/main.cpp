#include <Arduino.h>
#include <Command.hpp>
#include <WiFiManager.hpp>
#include <iostream>
#include <string>

#include <Pins.h>

#include <SensorReader.hpp>
#include <Sender.hpp>
#include <MyHTTPClient.hpp>

#define COMMAND_BUFFER_SIZE 128
#define SENSOR_UPDATE_INTERVAL 2000


CommandExecutor<20> commandExecutor;
WiFiManager wifiManager;
Sender sender;
MyHTTPClient http;

char commandBuffer[COMMAND_BUFFER_SIZE];
int commandBufferIndex = 0;
unsigned long lastUpdateTime;
bool autoSet = false;

SensorReader sensors(PIN_DHT, PIN_SDA, PIN_SCL);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);
    while (!Serial) { }

    Serial.println("Starting!");
    preferences.Load();

    sensors.Begin();

    if (!preferences.AreWiFiCredentialsSet())
        Serial.println("No stored WiFi credentials");

    if (!preferences.AreHTTPCredentialsSet())
        Serial.println("No stored HTTP credentials");
    
    Command wifiSetCommand("wifi-set", 2, [](int argc, char** argv) {
        wifiManager.SetCredentials(argv[0], argv[1]);
        Serial.println("Credentials updated");
        return OK;
    });

    Command wifiConnectCommand("wifi-connect", 0, [](int argc, char** argv) {
        if (!wifiManager.Connect())
            return ERR("No credentials set. Use wifi-set <SSID> <password> to set credentials.");
        return OK;
    });

    Command wifiDisconnectCommand("wifi-disconnect", 0, [](int argc, char** argv) {
        wifiManager.Disconnect();
        return OK;
    });

    Command wifiClearCommand("wifi-clear", 0, [](int argc, char** argv) {
        wifiManager.ClearStoredCredentials();
        Serial.println("Cleared stored credentials");
        return OK;
    });

    Command httpClearCommand("http-clear", 0, [](int argc, char** argv) {
        http.ClearStoredCredentials();
        Serial.println("Cleared stored credentials");
        return OK;
    });

    Command pollTemperatureCommand("get-temperature", 0, [](int argc, char** argv) {
        if (!sensors.IsDHTReady())
            return ERR("No read from DHT yet");

        Serial.printf("Temperature: %f oC\n", sensors.GetTemperature());
        return OK;
    });

    Command pollHumidityCommand("get-humidity", 0, [](int argc, char** argv) {
        if (!sensors.IsDHTReady())
            return ERR("No read from DHT yet");

        Serial.printf("Humidity: %f%%\n", sensors.GetHumidity());
        return OK;
    });

    Command pollSoilMoistureCommand("get-soil-moisture", 0, [](int argc, char** argv) {
        if (!sensors.IsSoilLightReady())
            return ERR("No read from soil/light yet");

        Serial.printf("Soil moisture: %f%%\n", sensors.GetSoilMoisture());
        return OK;
    });

    Command pollLightLevelCommand("get-light-level", 0, [](int argc, char** argv) {
        if (!sensors.IsSoilLightReady())
            return ERR("No read from soil/light yet");

        Serial.printf("Light level: %f%%\n", sensors.GetLightLevel());
        return OK;
    });

    Command serverInfo("server-info", 2, [](int argc, char** argv) {
        HTTPCredentials credentials;
        strncpy(credentials.ip, argv[0], 32);
        strncpy(credentials.uuid, argv[1], 64);
        preferences.SaveHTTPCredentials(credentials);
        preferences.Save();

        Serial.println("Server info set");
        return OK;
    });

    Command webSocketStart("send-begin", 0, [](int argc, char** argv) {
        if(!wifiManager.IsConnected())
            return ERR("Wifi not connected");
        if(!sender.IsReadyToBegin())
            return ERR("No info about destination. Use server-info <IP> <UUID>");
        sender.Begin();
        return OK;
    });

    Command testEepromCommand("test-eeprom", 0, [](int argc, char** argv) {
        EEPROM.begin(sizeof(Preferences) + 1);
        for (unsigned int i = 0; i < sizeof(Preferences) + 1; i++)
            Serial.printf("%02x", EEPROM.read(i));
        EEPROM.end();
        return OK;
    });


    /*Command test("test", 0, [](int argc, char** argv) {
        sender.SetIP("192.168.43.46");
        http.setIP("192.168.43.46");
        String uuid = http.requestForUUID();
        if(!sender.isReadyToBegin())
            return ERR("");
        sender.begin();
        autoSet = true;
        return OK;
    });*/

    Command test("test", 0, [](int argc, char** argv) {
        preferences.ClearHTTPCredentials();
        preferences.Save();
        return OK;
    });

    commandExecutor.AddCommand(std::move(wifiSetCommand));
    commandExecutor.AddCommand(std::move(wifiConnectCommand));
    commandExecutor.AddCommand(std::move(wifiDisconnectCommand));
    commandExecutor.AddCommand(std::move(wifiClearCommand));

    commandExecutor.AddCommand(std::move(pollTemperatureCommand));
    commandExecutor.AddCommand(std::move(pollHumidityCommand));
    commandExecutor.AddCommand(std::move(pollSoilMoistureCommand));
    commandExecutor.AddCommand(std::move(pollLightLevelCommand));

    commandExecutor.AddCommand(std::move(serverInfo));
    commandExecutor.AddCommand(std::move(webSocketStart));
    commandExecutor.AddCommand(std::move(testEepromCommand));
    commandExecutor.AddCommand(std::move(httpClearCommand));
    commandExecutor.AddCommand(std::move(test));
}

void HandleCommands()
{
    if (Serial.available() > 0)
    {
        char receivedChar = Serial.read();

        if (receivedChar == '\r') return;   // Ignore carriage return

        if (receivedChar == '\x08')     // Backspace
        {
            if (commandBufferIndex > 0) commandBufferIndex--;
        }
        else if (receivedChar != '\n')  // Any character
        {
            if (commandBufferIndex < COMMAND_BUFFER_SIZE) commandBuffer[commandBufferIndex++] = receivedChar;
        }
        else    // Enter
        {
            commandBuffer[commandBufferIndex] = '\0';
            
            auto result = commandExecutor.ExecuteCommand(commandBuffer);
            if (result.HasError())
                Serial.println("Error: " + String(result.GetError()));
            else
                Serial.println("OK");

            commandBufferIndex = 0;
        }       
    }
}

void StartAutoConnection()
{
    if (!autoSet && wifiManager.IsConnected())
    {
        // TODO: The logic in here is still a bit messy, needs refactoring
        if (!preferences.AreHTTPCredentialsSet())
        {
            Serial.println("Looking for plant-server in this subnet. This could take a while.");
            auto ip = http.FindServer(wifiManager.GetLocalIP());
            sender.SetIP(ip);

            Serial.println("Looking for UUID for device.");
            auto uuid = http.RequestUUID();
            sender.SetUUID(uuid);

            preferences.SaveUUID(uuid);
            preferences.SaveIP(ip);
        }
        else 
        {
            Serial.printf("Using stored IP: %s\n", preferences.GetHTTPCredentials().ip);
            Serial.printf("Using stored UUID: %s\n", preferences.GetHTTPCredentials().uuid);
            sender.SetUUID(String(preferences.GetHTTPCredentials().uuid));
            sender.SetIP(preferences.GetHTTPCredentials().ip); 
        }
        
        if (!sender.IsReadyToBegin())
            return;
        sender.Begin();
        autoSet = true;
    }
}

void loop()
{
    StartAutoConnection();

    sender.Update();
    HandleCommands();

    wifiManager.Update();
    sensors.Update();

    if ((millis() - lastUpdateTime > SENSOR_UPDATE_INTERVAL) && 
        wifiManager.IsConnected() && 
        sender.IsReady() &&
        sensors.IsAllReady())
    {
        auto temperature = std::to_string(sensors.GetTemperature());
        auto humidity = std::to_string(sensors.GetHumidity());
        auto soilMoisture = std::to_string(sensors.GetSoilMoisture());
        auto lightLevel = std::to_string(sensors.GetLightLevel());

        sender.sendMessage(temperature.c_str(), humidity.c_str(), soilMoisture.c_str(), lightLevel.c_str());
        lastUpdateTime = millis();
    }

    digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
}