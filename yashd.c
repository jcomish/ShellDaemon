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
#include <time.h>
#include <netdb.h>


#include <pthread.h>

#include "input.h"
#include "process.h"

#define NUM_THREADS 25

typedef struct _thread_data_t {
  int tid;
  int psd;
  int stuff;
  char *ip;
  int port;
  
} thread_data_t;

static volatile int count=0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t thr[NUM_THREADS];
thread_data_t thr_data[NUM_THREADS];
int threadsIndex = 0;
int sd, psd;
struct sockaddr_in server;

void clearBuffer(char * buf);

void reusePort(int s){
    int one=1;
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
}   

char * getTime(){
    time_t timer;
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    int month = tm_info->tm_mon;
    int day = tm_info->tm_mday;
    int hour = tm_info->tm_hour;
    int minutes = tm_info->tm_min;
    int seconds = tm_info->tm_sec;
    
    char *dateTime = malloc(sizeof(char) * 40);
    
    char *months[12] = {
       "Jan", "Feb", "Mar", "Apr", "May", "Jun",
       "Jul", "Aug", "Sept", "Oct", "Nov", "Dec" };
    
    snprintf(dateTime, sizeof(char) * 40, "%s %d %d:%d:%d", months[month], day, hour, minutes, seconds);

    return dateTime;
}

int trimProtocolInput(char *userInput){
    int status;
    if (userInput[strlen(userInput) - 1] == '\n')
    {
        userInput[strlen(userInput) - 1] = '\0';
    }
    else
    {
        return -1;
    }
    
    if (userInput[0] == 'C' && userInput[1] == 'M' && userInput[2] == 'D')
    {
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        return 1;
    }
/*
    else if (userInput[0] == 'C' && userInput[1] == 'T' && userInput[2] == 'L')
    {
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        return 2;
    }
*/
    else
    {
        return -1;
    }
}

void logCommand(char * userInput, int threadID){
    char *date = getTime();
    FILE *log = fopen("/tmp/yashd.log", "a");
    if (log == NULL)
    {
        printf("Error! can't open log file.");
        return -1;
    }
    
    printf("%s yashd[%s:%d]: %s\n", date, thr_data[threadID].ip, thr_data[threadID].port, userInput);
    fprintf(log, "%s yashd[%s:%d]: %s\n", date, thr_data[threadID].ip, thr_data[threadID].port, userInput);
    clearBuffer(date);
    free(date);
    
    fclose(log);
}

int processUserInput(char * userInput)
{
    userInput = malloc(200 * sizeof(char));
    userInput = "Command Recieved\0";
/*
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

    //sendCommand("CTL d\n");
    close(sd);
    exit(0);
 */
    return 200;
}

void *processThread(void *arg) {
    char userInput[1024];
    int cc;
    int threadID = *((int *) arg);
    
    clearBuffer(userInput);
    recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);
    
    //send initial #\n
    printf("%s\n", userInput);
    send(thr_data[threadID].psd, "#", 3, 0 );

    while(true) 
    {
        clearBuffer(userInput);
        cc=recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);
            if (cc == 0 || cc == -1) return;
        userInput[cc] = '\0';
        
        int commandStatus = trimProtocolInput(userInput);
        
        logCommand(userInput, threadID);
        
        //process command here
        char * response = malloc(200 * sizeof(char));
        int responseSize = processUserInput(userInput);
        //printf("%s\n", response);

        send(thr_data[threadID].psd, "Command Response", 16, 0);
        send(thr_data[threadID].psd, "\n#", 3, 0 );
        clearBuffer(userInput);
    }
    
    return;
}

void spawnThread(){
    int rc;
    
    int *index = malloc(sizeof(*index));
    *index = threadsIndex;
    thr_data[threadsIndex].tid = threadsIndex;
    thr_data[threadsIndex].psd = psd;

    if ((rc = pthread_create(&thr[threadsIndex], NULL, processThread, (void*) index))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return;
    }

    return;
}

void setupSocket(){
    sd = socket (AF_INET,SOCK_STREAM,0);
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = 3826;
    
    reusePort(sd);
    if (bind( sd, (struct sockaddr *) &server, sizeof(server)) < 0 ) 
    {
        close(sd);
        perror("binding name to stream socket");
        exit(-1);
    }
}

int main(int argc, char** argv) {
    setupSocket();
    
    //TODO: Daemonize
    //int daemon = fork();
    //if (daemon > 0)
    //{
        for (;;)
        {
            listen(sd,1);
            psd = accept(sd, 0, 0);

            struct sockaddr_in addr;
            socklen_t addr_size = sizeof(struct sockaddr_in);
            int res = getpeername(psd, (struct sockaddr *)&addr, &addr_size);

            thr_data[threadsIndex].ip = malloc(20 * sizeof(char));
            thr_data[threadsIndex].ip = inet_ntoa(addr.sin_addr);
            thr_data[threadsIndex].port = addr.sin_port;

            spawnThread();
            threadsIndex++;
        }
    //}
    //else
    //{
        return 0;
    //}
}




