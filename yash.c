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

int sd;
int status;
pid_t shell_pgid;
int * jobSize;
char * daemonIp;
bool ISDEBUG;
char userInput[250];
bool isSignalSent;

bool sendCommand(char * commandString){
    send(sd, commandString, strlen(commandString), 0 );
    return true;
}

static void sig_handler(int signo) {
  switch(signo){
      case SIGINT:
      {
          sendCommand("CTL c\n");
          clearBuffer(userInput);
          isSignalSent = true;
          return;
      }
      case SIGTSTP:
      {
          sendCommand("CTL z\n");
          clearBuffer(userInput);
          isSignalSent = true;
          return;
      }
  }
}

void setupSignals(){
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    printf("signal(SIGINT) error");
  if (signal(SIGTSTP, sig_handler) == SIG_ERR)
    printf("signal(SIGTSTP) error");
}

void openSocket(){
     struct  sockaddr_in server;
     struct hostent *hp;
    
     sd = socket (AF_INET,SOCK_STREAM,0);

     server.sin_family = AF_INET;
     hp = gethostbyname(daemonIp);
     bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
     server.sin_port = 3826;
     
     if (connect(sd, (struct  sockaddr *) &server, sizeof(server)) == -1)
     {
         printf("Socket failed to connect\n");
     }
    
     send(sd, "Connection successfully established", 36, 0 );
}

/*
char * combineCommands(char *** commandList){
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
*/

int main(int argc, char **argv){
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
        recv(sd,buf,sizeof(buf), 0);
        printf("%s ", buf);
        
        fgets(userInput, 250, stdin);
        //need this to immediately terminate
        //printf("%d\n", isSignalSent);
        if (feof(stdin)){ 
            break;
        }
        
        
        
        sendCommand(userInput);
        clearBuffer(userInput);
    }
    
    close(sd);
    printf("\n");
    exit(0);
}