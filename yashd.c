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


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>


#include <pthread.h>

#include "input.h"
#include "process.h"
#include "logger.h"

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
pid_t daemon_pid;
struct Job jobTable[200];
int * yashSize;
int ** jobSize;

void clearBuffer(char * buf);

/*For Logging*/
void sig_pipe(int n) 
{
   perror("Broken pipe signal");
}

void sig_chld(int n)
{
  int status;

  fprintf(stderr, "Child terminated\n");
  wait(&status); /* So no zombies */
}
/*For Logging*/

/*For Jobs*/

/*For Jobs*/


void reusePort(int s){
    int one=1;
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
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
    else if (userInput[0] == 'C' && userInput[1] == 'T' && userInput[2] == 'L')
    {
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        memmove(&userInput[0], &userInput[1], strlen(userInput));
        return 2;
    }
    else
    {
        return -1;
    }
}

int processUserInput(char * userInput, char * response, int * test)
{
    bool ISDEBUG = false;
    int commandCount = 0;
    
    if (handleError(validateInput(userInput)))
    {
        char **stringList;
        trimInput(userInput, ISDEBUG);
        stringList = splitInput(userInput, ISDEBUG);

        char ***commandList = splitIntoCommands(stringList, ISDEBUG);
        
        
        if (validateStringAmnt(stringList))
        {
            commandCount = processCommands(commandList, shell_pgid, jobTable, test, userInput, response, ISDEBUG);
            if (commandCount == -1)
            {
                printf("ERROR: Invalid Command\n");
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

    return commandCount;
}

void *processThread(void *arg) {
    char userInput[1024];
    int cc;
    int threadID = *((int *) arg);
    
    clearBuffer(userInput);
    recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);
    
    //send initial #\n
    send(thr_data[threadID].psd, "#", 3, 0 );
    int jobSizeVar = 0;
    int *test;
    test = &jobSizeVar;

    while(true) 
    {
        clearBuffer(userInput);
        //logMessage("before");
        cc=recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);
            if (cc == 0) 
            {
                logCommand("Closing Connection", thr_data[threadID].ip, thr_data[threadID].port, 4);
                return;
            }
            else if (cc == -1)
            {
                return;
            }
        userInput[cc] = '\0';
        //logMessage("after");
        
        int commandStatus = trimProtocolInput(userInput);
        logCommand(userInput, thr_data[threadID].ip, thr_data[threadID].port, 3);
        
        if (commandStatus == 1)
        {
            //process command here
            char * response = malloc(10000 * sizeof(char));

            int responseSize = processUserInput(userInput, response, test);
            signal(SIGINT, SIG_DFL) == SIG_ERR;

            send(thr_data[threadID].psd, response, responseSize, 0);
            send(thr_data[threadID].psd, "\n#", 3, 0 );
            clearBuffer(userInput);
            clearBuffer(response);
            jobSizeVar++;
        }
        else if (commandStatus == 2)
        {
            //Need to kill/stop currently running process
            if (userInput == "c")
            {
                //kill(getForeGroundPid(),SIGINT);
            }
            else if (userInput == "z")
            {
                
            }
            clearBuffer(userInput);

        }
        else if (commandStatus == -1)
        {
            send(thr_data[threadID].psd, "yashd: Failed to process command!", 35, 0);
            send(thr_data[threadID].psd, "\n#", 3, 0 );
            clearBuffer(userInput);
        }
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

void listenLoop()
{
    setupSocket();
    shell_pgid = getpid();
    runLogger();

    int yashSizeVar = 0;
    yashSize = &yashSizeVar;
    
    while (true)
    {
        listen(sd,1);
        psd = accept(sd, 0, 0);

        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        int res = getpeername(psd, (struct sockaddr *)&addr, &addr_size);

        thr_data[threadsIndex].ip = malloc(20 * sizeof(char));
        thr_data[threadsIndex].ip = inet_ntoa(addr.sin_addr);
        thr_data[threadsIndex].port = addr.sin_port;
        
        logCommand("Connection Established", thr_data[threadsIndex].ip, thr_data[threadsIndex].port, 4);

        spawnThread();
        threadsIndex++;
        yashSizeVar++;
    }
    return;
}

/*This is largely derived from the daemon_init method in u-echod.c example file*/
void initDaemon()
{
/*
    pid_t daemon_pid;
    int fd;
    int i;
    static FILE *log;
    char buf[256];
    
    
    #define PATHMAX 255
    static char u_server_path[PATHMAX+1] = "/tmp/yashd"; 
    static char u_socket_path[PATHMAX+1];
    static char u_log_path[PATHMAX+1];
    static char u_pid_path[PATHMAX+1];

    strcpy(u_socket_path, u_server_path);
    strcpy(u_pid_path, u_server_path);
    strncat(u_pid_path, ".pid", PATHMAX-strlen(u_pid_path));
    strcpy(u_log_path, u_server_path);
    strncat(u_log_path, ".log", PATHMAX-strlen(u_log_path));
    
    
    //1. fork the Daemon, close the parent
    daemon_pid = fork();
    if ( ( daemon_pid = fork() ) < 0 ) {
        perror("daemon_init: cannot fork");
        exit(EXIT_SUCCESS);
    } 
    else if (daemon_pid > 0)
        
        exit(EXIT_SUCCESS);
    
    //2. Close File Descriptors
    for (i = getdtablesize()-1; i>0; i--)
      close(i);
    
    //3. Detatch From Controlling Terminal
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    else
    {
        daemon_pid = getpid();
        setpgrp();
    }
    
    //4. Move to a safe directory
    chdir(u_server_path);
    
    //5. Reset STDIO 0,1 to /dev/null
    if ( (fd = open("/dev/null", O_RDWR)) < 0) {
    perror("Open");
    exit(0);
    }
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    close (fd);
    
    //6. Make sure only one copy of daemon exists
    if ( ( i = open(u_pid_path, O_RDWR | O_CREAT, 0666) ) < 0 )
        exit(1);
    if ( lockf(i, F_TLOCK, 0) != 0)
        exit(0);
    
    //7. Write PID of daemon to file
    sprintf(buf, "%6d", daemon_pid);
    write(i, buf, strlen(buf));
    
    //8. Establish Signal Handlers
    if ( signal(SIGCHLD, sig_chld) < 0 ) {
        perror("Signal SIGCHLD");
        exit(1);
    }
    if ( signal(SIGPIPE, sig_pipe) < 0 ) {
        perror("Signal SIGPIPE");
        exit(1);
    }
    
    //9. Set umask
    umask(0);
    
    //10. Take STDERR and dup to a logfile
    log = fopen("/tmp/yashd.log", "aw");
    fd = fileno(log);
    dup2(fd, STDERR_FILENO);
    close (fd);
*/
    
    listenLoop();
    return;
}

int main(int argc, char** argv) {
    printf("Starting yashd...\n");
    initDaemon();
    
    return 0;
}




