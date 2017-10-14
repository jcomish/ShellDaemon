#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "input.h"
#include "process.h"
#include "logger.h"

#define NUM_THREADS 250

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
} Job;

typedef struct userInputContainer {
    int * jobSize;
    int threadID;
    char userInput[1024];
} userInputContainer;

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
static void sig_int(int signo) {
    //printf("Sending signals to group:%d\n",pid_ch1); // group id is pid of first in pipeline
    //printf("Killing pid: %d\n", getForeGroundPid());
    kill(getForeGroundPid(),SIGINT);
    //Needed to do this so you can repeadetedly interrupt
}
static void sig_tstp(int signo) {
    //printf("Sending signals to group:%d\n",pid_ch1); 
    kill(getForeGroundPid(),SIGSTOP);
}
/*For Jobs*/


 

int trimProtocolInput(char *userInput){
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

void processUserInput(void * args)
{
    bool ISDEBUG = false;
    int commandStatus = 0;
    
    struct userInputContainer * data = (struct userInputContainer *) args;    
    
    char *userInput = data->userInput;
    int *test = data->jobSize;
    int ephThreadsIndex = data->threadID;
    
    if (handleError(validateInput(userInput)))
    {
        char **stringList;
        trimInput(userInput, ISDEBUG);
        stringList = splitInput(userInput, ISDEBUG);

        char ***commandList = splitIntoCommands(stringList, ISDEBUG);
        
        
        if (validateStringAmnt(stringList))
        {
            commandStatus = processCommands(commandList, shell_pgid, jobTable, test, 
                    userInput, thr_data[ephThreadsIndex].psd, ISDEBUG);
            
            if (commandStatus == -1)
            {
                printf("ERROR: Invalid Command\n");
            }
            
        }
        else
        {
            printf("INVALID INPUT: You are allowed up to 1 pipeline (2 commands), 1 IO redirect, and cannot background a pipeline.");
        }

        clearBuffer(stringList);
        freeCommandList(commandList);

    }   
    clearBuffer(userInput);
    
    //dup2(thr_data[ephThreadsIndex].psd, STDOUT_FILENO);
    fflush(stdout);
    //int flag = 1;

    //setsockopt(thr_data[ephThreadsIndex].psd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    //send(thr_data[threadID].psd, "\n#", 3, 0 );
    
    if (commandStatus >= 0)
        send(thr_data[ephThreadsIndex].psd, "\n#", 3, 0 );

    return;
}



int processUserInput_fork(char * userInput, int * jobSize, int ephThreadsIndex)
{
    bool ISDEBUG = false;
    int commandStatus = 0;
    
/*
    struct userInputContainer * data = (struct userInputContainer *) args;    
    
    char *userInput = data->userInput;
    int *test = data->jobSize;
    int ephThreadsIndex = data->threadID;
*/
    
    if (handleError(validateInput(userInput)))
    {
        char **stringList;
        trimInput(userInput, ISDEBUG);
        stringList = splitInput(userInput, ISDEBUG);

        char ***commandList = splitIntoCommands(stringList, ISDEBUG);
        
        
        if (validateStringAmnt(stringList))
        {
            commandStatus = processCommands(commandList, shell_pgid, jobTable, jobSize, 
                    userInput, thr_data[ephThreadsIndex].psd, ISDEBUG);
            
            if (commandStatus == -1)
            {
                printf("ERROR: Invalid Command\n");
            }
            
        }
        else
        {
            printf("INVALID INPUT: You are allowed up to 1 pipeline (2 commands), 1 IO redirect, and cannot background a pipeline.");
        }

        clearBuffer(stringList);
        freeCommandList(commandList);

    }   
    clearBuffer(userInput);
    
    //dup2(thr_data[ephThreadsIndex].psd, STDOUT_FILENO);
    fflush(stdout);
    //int flag = 1;

    //setsockopt(thr_data[ephThreadsIndex].psd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    //send(thr_data[threadID].psd, "\n#", 3, 0 );
    
    if (commandStatus >= 0)
        send(thr_data[ephThreadsIndex].psd, "\n#", 3, 0 );

    return commandStatus;
}






void *processThread(void *arg) {
    char userInput[1024];
    int cc;
    int threadID = *((int *) arg);
    int * status = malloc(sizeof(int));

    clearBuffer(userInput);
    recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);

    //send initial #\n
    send(thr_data[threadID].psd, "\n#", 3, 0 );

    int jobSizeVar = 0;
    int *jobSizeToPass;
    jobSizeToPass = &jobSizeVar;

    pthread_t commandThr[NUM_THREADS];
    int numCommands = 0;
    
/*
    sem_t mysem;
    int ret;
    ret = sem_init(&mysem, 0, 1);
    if (ret != 0) {
        perror("Unable to initialize the semaphore");
        abort();
    }
    
    ret = sem_wait(&mysem);
    ret = sem_post(&mysem);
*/
    
    while(true) 
    {
        clearBuffer(userInput);

        cc=recv(thr_data[threadID].psd,userInput,sizeof(userInput), 0);
        if (cc == 0) 
        {
            logCommand("Closing Connection", thr_data[threadID].ip, thr_data[threadID].port, 4);
            *status = -1;
            return (void *) status;
        }
        else if (cc == -1)
        {
            *status = -1;
            return (void *) status;
        }
        userInput[cc] = '\0';

        
        int commandStatus = trimProtocolInput(userInput);
        logCommand(userInput, thr_data[threadID].ip, thr_data[threadID].port, 3);
        if (commandStatus == 1)
        {
            struct userInputContainer data;
            strcpy(data.userInput, userInput);
            data.jobSize = jobSizeToPass;         
            data.threadID = threadID;

/*
            int rc;
            if ((rc = pthread_create(&commandThr[numCommands], NULL, processUserInput, (void*) &data))) {
              fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
              *status = -1;
              return (void *) status;
            }
*/
            int status;
            int cpid = fork();
            
            if (cpid > 0)
            {
                
                //waitpid(cpid, &status, WUNTRACED | WCONTINUED);
                 pid_t return_pid;
                do
                {
                    return_pid = waitpid(cpid, &status, WNOHANG);
                    if (return_pid == -1) {
                        /* error */
                    } else if (return_pid == 0) {
                        
                        
                        fd_set rfds;
                        struct timeval tv;
                        int retval;

                        
                        FD_ZERO(&rfds);
                        FD_SET(thr_data[threadID].psd, &rfds);
                        tv.tv_sec = 0;
                        tv.tv_usec = 0;
                        retval = select(thr_data[threadID].psd + 1, &rfds, NULL, NULL, &tv);
                        
                        
                        
                        
                        
                        
                        /* when it is found....*/
                        if (retval == -1)
                        {}
                        else if (retval){
                            char input[1024];
                            clearBuffer(input);
                            printf("Found Data!\n");
                            recv(thr_data[threadID].psd,input,sizeof(input), 0);
                            printf("%s\n", input);
                            
                            if (trimProtocolInput(input) == 2)
                            {
                                //Need to kill/stop currently running process
                                if (strcmp(input, "c") == 0)
                                {
                                    //call_sig_int(0);
                                    //resetStdIo();
                                    kill(cpid, SIGINT);
                                    //send(thr_data[threadID].psd, "\n#", 3, 0 );
                                }
                                else if (strcmp(input, "z") == 0)
                                {
                                    //call_sig_tstp(0);
                                    //resetStdIo();
                                    kill(cpid, SIGTSTP);
                                    //send(thr_data[threadID].psd, "\n#", 3, 0 );
                                    //resetStdIo();
                                }
                            }

                        }
                            
                            
                            

                        
                        
                        
                      
                        
                        
                    } else if (return_pid == cpid) {
                        printf("Finished!\n");
                        printf("%d, %d\n", return_pid, cpid);
                        /* child is finished. exit status in   status */
                    }
                } while (return_pid != cpid);
            }
            else
            {
                setupSignals(false);
                processUserInput_fork(data.userInput, jobSizeToPass, data.threadID);
                exit(0);
                
                
            }

            
            
                    
            numCommands++;
            signal(SIGINT, SIG_DFL) == SIG_ERR;
        }
        else if (commandStatus == -1)
        {
            send(thr_data[threadID].psd, "yashd: Failed to process command!", 35, 0);
            send(thr_data[threadID].psd, "\n#", 3, 0 );
            *status = -1;
        }
    }
    
    return (void *) status;
}

void spawnThread(int ephThreadsIndex){
    int rc;
    int *index = malloc(sizeof(*index));
    *index = ephThreadsIndex;

    if ((rc = pthread_create(&thr[ephThreadsIndex], NULL, processThread, (void*) index))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return;
    }

    return;
}

void reusePort(int s){
    int one=1;
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
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
        thr_data[threadsIndex].psd  = accept(sd, 0, 0);
        int ephThreadsIndex = threadsIndex;
        
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        getpeername(thr_data[threadsIndex].psd, (struct sockaddr *)&addr, &addr_size);

        thr_data[threadsIndex].ip = malloc(20 * sizeof(char));
        thr_data[threadsIndex].ip = inet_ntoa(addr.sin_addr);
        thr_data[threadsIndex].port = addr.sin_port;
        
        logCommand("Connection Established", thr_data[threadsIndex].ip, thr_data[threadsIndex].port, 4);

        spawnThread(ephThreadsIndex);
        threadsIndex++;
        yashSizeVar++;
    }
    return;
}

/*This is largely derived from the daemon_init method in u-echod.c example file*/
void initDaemon()
{
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
        printf("yashd started");
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
    //dup2(fd, STDOUT_FILENO);
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
    
    listenLoop();
    return;
}

int main(int argc, char** argv) {
    bool ISDEBUG = false;
    if (argc > 1)
    {
        if (argv[1][1] == 'd')
        {
            ISDEBUG = true;
        }
    }
    
    if (!ISDEBUG)
    {
        initDaemon();
    }
    listenLoop();
    
    return 0;
}




