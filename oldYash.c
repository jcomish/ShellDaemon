#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <string.h>

#include "input.h"
#include "process.h"



typedef struct Job {
    int pid;
    int pgid;
    int jobNo;
    char ground;
    char status [20];
    char command[250];
};

int status;
pid_t shell_pgid;
struct Job jobTable[200];
int * jobSize;

//REQUIREMENT: All child processes will be dead on exit
void killProcesses()
{
    int i;
    for (i = 0; i < *jobSize; i++)
        kill(jobTable[i].pid, SIGINT);
    //free(jobTable);
}

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
        freeCString(string);
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

int main(int argc, char **argv)
{
    int i;
    bool ISDEBUG = false;
    
    for (i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            ISDEBUG = true;
        }
    }
    
    atexit(killProcesses);
    
    shell_pgid = getpid();

    int jobSizeVar = 0;
    jobSize = &jobSizeVar;

    //REQUIREMENT: Ctrl-c must quit current foreground process (if one exists)  
    //and not the shell and should not print the process (unlike bash)
    signal(SIGINT, SIG_IGN);
    //REQUIREMENT: The shell will not be stopped on SIGTSTP
    signal(SIGTSTP, SIG_IGN);

    while(!feof(stdin))	
    {
        char userInput[250];

        //REQUIRMENT: The prompt must be printed as a ‘# ‘ 
        //(hashtag-sign with a space after it) before accepting user input.
        printf("# ");
        fgets(userInput, 250, stdin);
        //need this to immediately terminate
        if (feof(stdin))
        {
            break;
        }

        if (handleError(validateInput(userInput)))
        {
            char **stringList;
            trimInput(userInput, ISDEBUG);
            stringList = splitInput(userInput, ISDEBUG);

            //TODO: Collect tokens
            char ***commandList = splitIntoCommands(stringList, ISDEBUG);
            if (ISDEBUG){printCommands(commandList);}

            //Run commands
            if (validateStringAmnt(stringList))
            {
                if (processCommands(commandList, shell_pgid, jobTable, jobSize, userInput, ISDEBUG) == -1)
                {
                    printf("ERROR: Invalid Command\n");
                }  
            }
            else
            {
                //REQUIREMENT: Only one | must be present in each pipeline
                printf("INVALID INPUT: You are allowed up to 1 pipeline (2 commands), 1 IO redirect, and cannot background a pipeline.\n");
            }

            int i;
            for (i = 0; stringList[i] != '\0'; i++)
            {
                stringList[i] = "\0";
            }

            freeCommandList(commandList);

        }   
        int i;
        for (i = 0; userInput[i] != '\0'; i++)
        {
            userInput[i] = '\0';
        }       
    }

    exit(0);
}

//Other requirements implicitly met
//REQUIREMENT: Ctrl-d will exit the shell
//REQUIREMENT: All characters will be ASCII
//REQUIREMENT: A command chain with a  pipeline
//             is considered a single job as seen in the example above
//REQUIREMENT: Your shell must search the PATH environment variable for every executable
//REQUIREMENT: Children must inherit the environment from the parent
