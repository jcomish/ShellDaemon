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

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

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
    
    char buf[10000];
    setupSignals();
    openSocket();

    while(!feof(stdin))	
    {
        
        
        
        do 
        {
            clearBuffer(buf);
            recv(sd,buf,sizeof(buf), 0);
            if (strcmp(buf, "\n#") == 0)
                printf("# ");
            else
                printf("%s", buf);
            
            fflush(stdout);
        } while (strcmp(buf, "\n#") != 0);

        
        fgets(userInput, 250, stdin);
        //need this to immediately terminate
        if (feof(stdin)){ 
            break;
        }

        sendCommand(concat("CMD ", userInput));
        
        clearBuffer(userInput);
    }
    
    close(sd);
    printf("\n");
    exit(0);
}