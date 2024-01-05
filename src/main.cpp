#include <Arduino.h>
#include "Command.hpp"

#define COMMAND_BUFFER_SIZE 128

CommandExecutor<5> commandExecutor;

char commandBuffer[COMMAND_BUFFER_SIZE];
int commandBufferIndex = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);
    while (!Serial) { }

    Serial.println("Starting!");
    
    Command testCommand("test", 0, [](int argc, char** argv) {
        Serial.println("Hello");
        return ERR("Test error");
    });

    Command testCommand2("test2", 0, [](int argc, char** argv) {
        Serial.println("Hello 2");
        return OK;
    });

    Command testCommand3("test3", 0, [](int argc, char** argv) {
        Serial.println("Hello 3");
        return OK;
    });

    commandExecutor.AddCommand(std::move(testCommand));
    commandExecutor.AddCommand(std::move(testCommand2));
    commandExecutor.AddCommand(std::move(testCommand3));
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

    digitalWrite(LED_BUILTIN, millis() % 1000 < 500);
}