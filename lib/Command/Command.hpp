#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <Arduino.h>
#include <Result.hpp>

typedef Result<bool, const char*> CommandResult;
typedef CommandResult (*CommandHandler)(int argc, char** argv);

#define OK CommandResult(true)
#define ERR(error) CommandResult(error)

char errorBuf[128];

class Command {
private:
    char name[16];
    unsigned int argCount;
    CommandHandler handler;

public:
    Command() { }

    Command(const char* name, unsigned int argCount, CommandHandler handler) 
    {
        strncpy(this->name, name, 16);
        this->argCount = argCount;
        this->handler = handler;
    }

    CommandResult ExecuteCommand(unsigned int argc, char** argv) 
    {
        if (argc != argCount)
        {
            sprintf(errorBuf, "Wrong argument count: expected %d, got %d", argCount, argc);
            return ERR(errorBuf);
        }

        if (handler == nullptr)
            return ERR("Unassigned handler");

        return handler(argc, argv);
    }

    char* GetName()
    {
        return name;
    }
};

template <size_t maxCommands, size_t bufferSize = 128>
class CommandExecutor {
private:
    Command commands[maxCommands];
    unsigned int commandCount = 0;

    char buffer[bufferSize];

public:
    CommandExecutor() 
    { 
    }

    void AddCommand(Command&& command)
    {
        if (commandCount >= maxCommands)
            return;
        commands[commandCount++] = command;
    }

    CommandResult ExecuteCommand(const char* line)
    {
        strncpy(buffer, line, bufferSize);

        char* token = strtok(buffer, " ");
        unsigned int argc = 0;
        char* argv[10];

        while (token != nullptr && argc < 10) 
        {
            argv[argc++] = token;
            token = strtok(nullptr, " ");
        }

        if (argc > 0)
        {
            const char* commandName = argv[0];

            for (unsigned int i = 0; i < commandCount; i++)
            {
                Command& command = commands[i];
                if (strcmp(command.GetName(), commandName) == 0)
                    return command.ExecuteCommand(argc - 1, &argv[1]);
            }

            return ERR("Unknown command");
        }

        return ERR("Nothing supplied");
    }
};

#endif
