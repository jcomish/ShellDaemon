#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <string.h>
#include "input.h"

int containsCommand(char ***commands, char *command);
int countArgs(char **args);
bool containsCharacter(char *args, char character);


void freeCString(char *string)
{
    int i;
    for(i = 0; string[i] != '\0'; i++)
    {
        string[i] = '\0';
    }
}

void freeArgs(char ** string)
{
    int i;
    for(i = 0; string[i] != '\0'; i++)
    {
        freeCString(string[i]);
    }
}

void freeCommandList(char *** commandList)
{
    int i;
    for (i = 0; commandList[i]; i++)
    {
        freeArgs(commandList[i]);
    }
}

bool* validateInput(char *input)
{
    bool* isValid = malloc(5 * sizeof(bool));
    int i;
    for (i = 0; i < 5; i++)
    {
        isValid[i] = true;
    }

    //Tokens will have a space before and after
    char tokens[3] = {'<', '>', '|'};

    for(i = 0; i < strlen(input); i++)
    {
        int j;
        for (j = 0; j < strlen(tokens); j++)
        {
            if (input[i] == tokens[j])
            {
                if (i == 0 || i == strlen(input) - 1)
                {
                    isValid[4] = false;
                    return isValid;
                }
                if (i - 0 > 0 && i + 1 < strlen(input))
                {
                    if (input[i + 1] != ' ' || input[i - 1] != ' ')
                    {
                        isValid[0] = false;
                        return isValid;
                    }
                }
            }
        }
    }

    //& will always be the last token in a line
    for(i = 0; i < strlen(input); i++)
        if (input[i] == '&')
        {
            int j;

            for(j = i + 1; j < strlen(input) - 1; j++)
            {
                if (input[j] != ' ')
                {
                    isValid[1] = false;
                    return isValid;
                }
            }

            break;
        }

    //Each line contains one command or two commands in one pipeline

    //Lines will not exceed 200 characters
    if (strlen(input) - 1 > 200)
        isValid[3] = false;

    return isValid;
}

void append(char *string, char c)
{
    int len = strlen(string);
    string[len] = c;
    string[len+1] = '\0';
    return;
}

void addString(char **stringList, int currentString, char *rawInput, int startIndex, int endIndex)
{
    int i;
    stringList[currentString] = malloc(200 * sizeof(char*));
    for(i = startIndex; i < endIndex; i++)
    {
        append(stringList[currentString], rawInput[i]);
    }


    return;
}


char ** splitInput(char *input, bool ISDEBUG)
{
    char **stringList = malloc(25 * sizeof(char*));
    int i;
    int currentString = 0;
    int startIndex = 0;

    for(i = 0; input[i] != '\0'; i++)
    {
        //if you encounter a space, add the previous command
        if (input[i] == ' ')
        {
            addString(stringList, currentString, input, startIndex, i);
            startIndex = i + 1;
            currentString++;
        }

        //if the command contains the last command, add it.
        if (i == strlen(input) - 1 && input[i] != ' ')
        {
            addString(stringList, currentString, input, startIndex, i + 1);
            startIndex = i + 1;
            currentString++;
            stringList[currentString] = '\0';
        }
    }

    return stringList;
}


//REQUIREMENT: Each line contains one command or two commands in one pipeline
//REQUIREMENT: You can only background a single command (no pipeline). 
//             In other words | and & are mutually exclusive.
bool validateStringAmnt(char ** strings)
{
    int i;
    int pipeAmnt = 0;
    int inputAmnt = 0;
    int outputAmnt = 0;
    int backgroundAmnt = 0;
    
    for (i = 0; strings[i] != '\0'; i++)
    {
        int j;
        for (j = 0; strings[i][j] != '\0'; j++)
        {
            if (strings[i][j] == '|')
            {
                pipeAmnt++;
            }
            if (strings[i][j] == '<')
            {
                inputAmnt++;
            }
            if (strings[i][j] == '>')
            {
                outputAmnt++;
            }
            if (strings[i][j] == '&')
            {
                backgroundAmnt++;
            }
        }
    }
    
    return (pipeAmnt < 2) && (inputAmnt < 2) && (outputAmnt < 2) && !(backgroundAmnt > 0 && pipeAmnt > 0);
}


bool handleError(bool* isValid, int socket)
{
    dup2(socket, STDOUT_FILENO);
    if (!isValid[0])
    {
        //REQUIREMENT: A command can have both the redirection symbols (No 2>&1)
        //REQUIREMENT: Everything that can be a token (<, >, |, etc.) 
        //will have a space before and after it. 
        printf("INVALID INPUT: tokens require a space before and after\n");
        return false;
    }
    if (!isValid[1])
    {
        //REQUIREMENT: A command can have both the redirection symbols (No 2>&1)
        //REQUIREMENT: & will always be the last token in a line (only one & makes sense)
        printf("INVALID INPUT: '&' can only be placed at the very end of a string\n");
        return false;
    }
    if (!isValid[2])
    {
        printf("INVALID INPUT: Invalid amount of commands\n");
        return false;
    }
    if (!isValid[3])
    {
        //REQUIREMENT: Lines will not exceed 200 characters
        printf("INVALID INPUT: Max line size is 200\n");
        return false;
    }
    if (!isValid[4])
    {
        printf("INVALID INPUT: Token cannot be the first or last character\n");
        return false;
    }

    free(isValid);
    return true;
}

void trimInput(char *rawInput, bool ISDEBUG)
{
    int i;
    for(i = 0; rawInput[i] != '\0'; i++)
    {
        if (rawInput[i] == ' ')
        {
            if (rawInput[i + 1] == ' ')
            {
                int j;
                for(j = i; rawInput[j] != '\0'; j++)
                {
                    rawInput[j] = rawInput[j + 1];
                }
                i--;
            }
        }
        if (rawInput[i] == '\n')
        {
            rawInput[i] = '\0';
        }
    }

    return;
}

void printCommands(char ***strings)
{
    int i;
    
    for (i = 0; strings[i] != '\0'; i++)
    {
/*
        printf("DEBUG: Command (%d): ", i);
*/
        int j;
        for (j = 0; strings[i][j] != '\0'; j++)
        {
            printf("%s ", strings[i][j]);
        }
        printf("\n");
    }
}

bool isToken(char command)
{
    if (command == '<' || command == '>' || command == '|' || command == '&')
    {
        return true;
    }
    return false;
}

int countArgs(char **args)
{
    int argCount;
    for (argCount = 0; args[argCount] != '\0'; argCount++){ continue; }
    
    return argCount;
}

int containsCommand(char ***commands, char *command)
{
    int i;
    for (i = 0; commands[i]; i++)
    {
        if (strcmp(commands[i][0], command) == 0)
        {
            return i;
        }
    }
    return -1;
}

bool containsCharacter(char *args, char character)
{
    int i;
    for (i = 0; args[i] != '\0'; i++)
    {
        if (args[i] == character)
        {
            return true;
        }
    }
    return false;
}

void addCommand(char ***commandList, char **argList, int pos, bool ISDEBUG)
{
    char **argCpy = malloc(25 * sizeof(char*));
    int i;
    for (i = 0; argList[i] != '\0'; i++)
    {
        argCpy[i] = argList[i];
        
        if (ISDEBUG)
        {
            printf("DEBUG: add arg: '%s'\n", argList[i]);
        }
    }
    
    commandList[pos] = argCpy;
    
    return;
}

//REQUIREMENT: Any redirections will follow the command after all its args
char *** splitIntoCommands(char **strings, bool ISDEBUG)
{
    int i;
    char **command = malloc(25 * sizeof(char*));
    int argAmnt = 0;
    int commandAmnt = 0;
    char ***returnCommands = malloc(50 * sizeof(char**));
    int j;
    
    for (i = 0; strings[i] != '\0'; i++ )
    {
        if (!isToken(strings[i][0]))
        {
            command[argAmnt] = strings[i];
            argAmnt++;
        }
            
        if (isToken(strings[i][0]))
        {
            //add the command
            addCommand(returnCommands, command, commandAmnt, ISDEBUG);
            commandAmnt++;
            argAmnt = 0;
            for(j = 0; command[j] != '\0'; j++)
            {
                command[j] = '\0';
            }
            
            //add the token
            command[argAmnt] = strings[i];
            addCommand(returnCommands, command, commandAmnt, ISDEBUG);
            commandAmnt++;
            for(j = 0; command[j] != '\0'; j++)
            {
                command[j] = '\0';
            }
        }
    }
    
    //Add last command if not a background token
    if (containsCommand(returnCommands, "&") == -1)
    {
        addCommand(returnCommands, command, commandAmnt, ISDEBUG);
    }
    
    for (j = 0; command[j] != '\0'; j++)
    {
        command[j] = '\0';
    }

    return returnCommands;
}

bool areStringsValid(char *** commands)
{
    if (containsCommand(commands, "&") && containsCommand(commands, "|"))
    {
        return false;
    }
    
    return true;
}

void clearBuffer(char * buf){
    int i;
    for (i = strlen(buf); i >= 0; i--)
    {
        buf[i] = '\0';
    }
}

int countBufferSize(char * buf)
{
    int i;
    for (i = 0; buf[i] != '\0'; i++)
    {
        
    }
    return i;
}