#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Arduino.h>
#include <cstdarg>

#define LOG_BUFFER_SIZE 512

// FIXME: Using this results in a linker error for some reason
class Logger
{
private:
    static char buffer[LOG_BUFFER_SIZE];

    static void PrintMessage(const char* severity, const char* format, va_list args)
    {
        vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
        Serial.printf("[%s] %s\n", severity, buffer);
    }

public:
    static void Log(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        PrintMessage("INFO", format, args);
        va_end(args);
    }

    static void Warn(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        PrintMessage("WARN", format, args);
        va_end(args);
    }

    static void Error(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        PrintMessage("ERROR", format, args);
        va_end(args);
    }
};

#endif
