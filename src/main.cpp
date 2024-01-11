#include <Arduino.h>
#include <Command.hpp>
#include <WiFiManager.hpp>

#include <Pins.h>

#include <SensorReader.hpp>

#define COMMAND_BUFFER_SIZE 128

CommandExecutor<10> commandExecutor;
WiFiManager wifiManager;

char commandBuffer[COMMAND_BUFFER_SIZE];
int commandBufferIndex = 0;

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


    commandExecutor.AddCommand(std::move(wifiSetCommand));
    commandExecutor.AddCommand(std::move(wifiConnectCommand));
    commandExecutor.AddCommand(std::move(wifiDisconnectCommand));
    commandExecutor.AddCommand(std::move(wifiClearCommand));

    commandExecutor.AddCommand(std::move(pollTemperatureCommand));
    commandExecutor.AddCommand(std::move(pollHumidityCommand));
    commandExecutor.AddCommand(std::move(pollSoilMoistureCommand));
    commandExecutor.AddCommand(std::move(pollLightLevelCommand));
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
    HandleCommands();

    wifiManager.Update();
    sensors.Update();

    digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
}