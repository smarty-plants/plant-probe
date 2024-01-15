#include <Arduino.h>
#include <Command.hpp>
#include <WiFiManager.hpp>
#include <iostream>
#include <string>

#include <Pins.h>

#include <SensorReader.hpp>
#include <Sender.hpp>

#define COMMAND_BUFFER_SIZE 128
#define UPDATEINTERVAL 2000


CommandExecutor<15> commandExecutor;
WiFiManager wifiManager;
Sender sender;

char commandBuffer[COMMAND_BUFFER_SIZE];
int commandBufferIndex = 0;
unsigned long lastUpdateTime;

SensorReader sensors(PIN_DHT, PIN_SDA, PIN_SCL);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);
    while (!Serial) { }

    Serial.println("Starting!");

    sensors.Begin();

    if (!wifiManager.UseStoredCredentials())
        Serial.println("No stored WiFi credentials");
    
    Command wifiSetCommand("wifi-set", 2, [](int argc, char** argv) {
        wifiManager.SetCredentials(argv[0], argv[1]);
        wifiManager.StoreCredentials();
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
        sender.SetIP(argv[0]);
        sender.SetUUID(argv[1]);
        Serial.println("Server info set");
        return OK;
    });

    Command webSocketStart("send-begin", 0, [](int argc, char** argv) {
        if(!wifiManager.IsConnected())
            return ERR("No credentials set. Use wifi-set <SSID> <password> to set credentials.");
        if(!sender.isReadyToBegin())
            return ERR("No info about destination. Use server-info <IP> <UUID>");
        sender.begin();
        return OK;
    });


    /*Command test("test", 0, [](int argc, char** argv) {
        Serial.println((std::to_string(sensors.IsDHTReady())).c_str());
        Serial.println((std::to_string(sensors.IsSoilLightReady())).c_str());
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
    //commandExecutor.AddCommand(std::move(test));
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

void loop()
{
    sender.loop();
    HandleCommands();

    wifiManager.Update();
    sensors.Update();

    if ((millis() - lastUpdateTime > UPDATEINTERVAL) && 
        wifiManager.IsConnected() && 
        sender.isReady() &&
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