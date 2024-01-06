#include <Arduino.h>
#include <Command.hpp>
#include <WiFiManager.hpp>

#define COMMAND_BUFFER_SIZE 128

CommandExecutor<5> commandExecutor;
WiFiManager wifiManager;

char commandBuffer[COMMAND_BUFFER_SIZE];
int commandBufferIndex = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);
    while (!Serial) { }

    Serial.println("Starting!");

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

    commandExecutor.AddCommand(std::move(wifiSetCommand));
    commandExecutor.AddCommand(std::move(wifiConnectCommand));
    commandExecutor.AddCommand(std::move(wifiDisconnectCommand));
    commandExecutor.AddCommand(std::move(wifiClearCommand));
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

    digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
}