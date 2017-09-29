#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "input.h"



/*
typedef struct Job {
    int pid;
    int pgid;
    int jobNo;
    char ground;
    char status [20];
    char command[250];
};
*/
int sd;
int status;
pid_t shell_pgid;
//struct Job jobTable[200];
int * jobSize;
char * daemonIp;
bool ISDEBUG;

bool sendCommand(char * commandString)
{
    send(sd, commandString, strlen(commandString), 0 );
    return true;
}

static void sig_handler(int signo) {
  switch(signo){
      case SIGINT:
      {
          sendCommand("CTL c\n");
          break;
      }
      case SIGTSTP:
      {
          sendCommand("CTL z\n");
          break;
      }
  }

}

void setupSignals()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    printf("signal(SIGINT) error");
  if (signal(SIGTSTP, sig_handler) == SIG_ERR)
    printf("signal(SIGTSTP) error");

}

void openSocket()
{
     struct  sockaddr_in server;
     struct hostent *hp;
    
     sd = socket (AF_INET,SOCK_STREAM,0);

     server.sin_family = AF_INET;
     hp = gethostbyname(daemonIp);
     bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
     server.sin_port = 3826;
     connect(sd, (struct  sockaddr *) &server, sizeof(server));
    
     send(sd, "Connection successfully established", 36, 0 );
     
/*
     char buf[7];
     int cc;
     cc=recv(sd,buf,7, 0);
     printf("%s\n", buf);
*/
     
}

char * combineCommands(char *** commandList)
{
    char *commandReturn = malloc (sizeof(char) * 300);
    int commandSize = 0;
    
    commandReturn[commandSize] = 'C';
    commandSize++;
    commandReturn[commandSize] = 'M';
    commandSize++;
    commandReturn[commandSize] = 'D';
    commandSize++;
    commandReturn[commandSize] = ' ';
    commandSize++;
    
    int i;
    for (i = 0; commandList[i]; i++)
    {
        int j;
        for (j = 0; commandList[i][j]; j++)
        {
            int z;
            for (z = 0; commandList[i][j][z]; z++)
            {
                commandReturn[commandSize] = commandList[i][j][z];
                commandSize++;
            }
            if (commandList[i][j + 1] || commandList[i + 1])
            {
                commandReturn[commandSize] = ' ';
                commandSize++;
            }
        }
    }
    
    commandReturn[commandSize] = '\n';
    commandSize++;
    commandReturn[commandSize] = '\0';
    return commandReturn;
}

int main(int argc, char **argv)
{
    int i;
    ISDEBUG = false;
    
    for (i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            ISDEBUG = true;
        }
    }
    
    if (argc > 1)
    {
        if (argv[1][0] != '-')
        {
            daemonIp = argv[1];
        }
    }
    else
    {
        daemonIp = "127.0.0.1";
    }
    
    char buf[1024];

    setupSignals();
    openSocket();
    

    while(!feof(stdin))	
    {
        char userInput[250];
        
        recv(sd,buf,sizeof(buf), 0);
        printf("%s ", buf);
        
       
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

            char ***commandList = splitIntoCommands(stringList, ISDEBUG);
            if (ISDEBUG){printCommands(commandList);}

            if (validateStringAmnt(stringList))
            {
                if (sendCommand(combineCommands(commandList)))
                {
                    //printf("ERROR: Invalid Command");
                }  
            }
            else
            {
                printf("INVALID INPUT: You are allowed up to 1 pipeline (2 commands), 1 IO redirect, and cannot background a pipeline.");
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
        i++;
    }

    sendCommand("CTL d\n");
    exit(0);
}

