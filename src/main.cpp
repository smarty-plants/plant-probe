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

    if (!preferences.IsServerIPSet())
        Serial.println("No stored server IP");

    if (!preferences.IsProbeUUIDSet())
        Serial.println("No stored probe UUID");
    
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

    Command ipClearCommand("ip-clear", 0, [](int argc, char** argv) {
        preferences.ClearServerIP();
        preferences.Save();
        Serial.println("Server IP cleared");
        return OK;
    });

    Command uuidClearCommand("uuid-clear", 0, [](int argc, char** argv) {
        preferences.ClearProbeUUID();
        preferences.Save();
        Serial.println("Probe UUID cleared");
        return OK;
    });

    Command ipSetCommand("ip-set", 1, [](int argc, char** argv) {
        preferences.SetServerIP(String(argv[0]));
        preferences.Save();
        Serial.println("Server IP set");
        return OK;
    });

    Command uuidSetCommand("uuid-set", 1, [](int argc, char** argv) {
        preferences.SetProbeUUID(String(argv[0]));
        preferences.Save();
        Serial.println("Probe UUID set");
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
        preferences.SetHTTPCredentials(credentials);
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

    Command wifiAutoConnectCommand("wifi-autoconnect", 1, [](int argc, char** argv) {
        if (strcmp(argv[0], "on") == 0)
            preferences.SetAutoConnectToWiFi(true);
        else if (strcmp(argv[0], "off") == 0)
            preferences.SetAutoConnectToWiFi(false);
        else
            return ERR("wifi-autoconnect takes 1 argument: on / off");
        
        preferences.Save();
        return OK;
    });

    Command serverAutoConnectCommand("server-autoconnect", 1, [](int argc, char** argv) {
        if (strcmp(argv[0], "on") == 0)
            preferences.SetAutoConnectToServer(true);
        else if (strcmp(argv[0], "off") == 0)
            preferences.SetAutoConnectToServer(false);
        else
            return ERR("server-autoconnect takes 1 argument: on / off");
        
        preferences.Save();
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

    commandExecutor.AddCommand(std::move(ipClearCommand));
    commandExecutor.AddCommand(std::move(uuidClearCommand));
    commandExecutor.AddCommand(std::move(ipSetCommand));
    commandExecutor.AddCommand(std::move(uuidSetCommand));

    commandExecutor.AddCommand(std::move(wifiAutoConnectCommand));
    commandExecutor.AddCommand(std::move(serverAutoConnectCommand));
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

void ConnectToServer()
{
    if (!autoSet && wifiManager.IsConnected())
    {
        if (!preferences.IsServerIPSet())
        {
            Serial.println("Looking for plant-server in this subnet. This could take a while.");
            String ip;
            bool success = http.FindServer(wifiManager.GetLocalIP(), ip);

            if (!success)
            {
                Serial.println("Could not find plant-server in this subnet. Is the server running and connected to the network?");
            }
            else 
            {
                sender.SetIP(ip);
                preferences.SetServerIP(ip);
                preferences.Save();    
            }                   
        }
        else 
        {
            Serial.printf("Using stored IP: %s\n", preferences.GetServerIP().c_str());
            sender.SetIP(preferences.GetServerIP()); 
        }

        if (!preferences.IsProbeUUIDSet())
        {            
            Serial.println("Requesting probe UUID from server...");
            String uuid;
            bool success = http.RequestUUID(uuid);

            if (!success)
            {
                Serial.println("Could not request UUID. Is the server running and connected to the network?");
            }
            else 
            {
                sender.SetUUID(uuid);

                preferences.SetProbeUUID(uuid);
                preferences.Save();
            }
        }
        else 
        {
            Serial.printf("Using stored UUID: %s\n", preferences.GetProbeUUID().c_str());
            sender.SetUUID(preferences.GetProbeUUID());
        }
        
        if (!sender.IsReadyToBegin())
            return;
        sender.Begin();
        autoSet = true;
    }
}

void loop()
{
    if (preferences.GetAutoConnectToWiFi() && wifiManager.IsDisconnected())
        wifiManager.Connect();

    if (preferences.GetAutoConnectToServer())
        ConnectToServer();

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

        sender.SendMessage(temperature.c_str(), humidity.c_str(), soilMoisture.c_str(), lightLevel.c_str());
        lastUpdateTime = millis();
    }

    digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
}