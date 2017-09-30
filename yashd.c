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

#include <pthread.h>

#include "input.h"
#include "process.h"

#define NUM_THREADS 25

typedef struct _thread_data_t {
  int tid;
  int stuff;
} thread_data_t;


static volatile int count=0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t thr[NUM_THREADS];
thread_data_t thr_data[NUM_THREADS];
int threadsIndex = 0;
int sd, psd;

void clearBuffer(char * buf);

void reusePort(int s)
{
    int one=1;
    
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
}   

/* thread function Ex1: thread_data_t Input; no output*/
void *processThread(void *arg) {
    char buf[1024];
    int cc;
    
    printf("Thread: %d\n", threadsIndex);
    
    clearBuffer(buf);
    recv(psd,buf,sizeof(buf), 0);
    
    printf("'%s'\n", buf);
    send(psd, "#", 3, 0 );

    for(;;) 
    {
        bool ISDEBUG = true;
        clearBuffer(buf);
        cc=recv(psd,buf,sizeof(buf), 0);
            if (cc == 0) return;
        buf[cc] = '\0';
        printf("message received: %s", buf);
       
        //char * results = handleRawInput(buf);
       
        send(psd, "Command Recieved", 16, 0);
        send(psd, "\n#", 3, 0 );
       
    }
    
    return;
}

void spawnThread()
{
    int i, rc;
    
    thr_data[threadsIndex].tid = threadsIndex;
    if ((rc = pthread_create(&thr[threadsIndex], NULL, processThread, &thr_data[threadsIndex]))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return;
    }
    //pthread_join(thr[threadsIndex], NULL);


    return;
}

void clearBuffer(char * buf)
{
    int i;
    for (i = strlen(buf); i > 0; i--)
    {
        buf[i] = '\0';
    }
}

void setupSocket()
{
    struct sockaddr_in server;
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

int main(int argc, char** argv) 
{
    setupSocket();
    
    for (;;)
    {
        listen(sd,1);
        psd = accept(sd, 0, 0);
//thr_data[threadsIndex] = accept(sd, 0, 0);
        spawnThread();
        threadsIndex++;
        
        printf("Socket Established\n");
    }

    return 0;
    


}

