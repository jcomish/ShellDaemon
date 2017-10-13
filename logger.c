#define _POSIX_SOURCE
#include <errno.h>

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
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

#include <fcntl.h>           /* For O_* constants */
/* mq_* functions */
#include <mqueue.h>
#include <sys/stat.h>
/* ctime() and time() */
#include <time.h>

#include "input.h"

/* name of the POSIX object referencing the queue */
#define MSGQOBJ_NAME    "/yashdQueuetest"
/* max length of a message (just for this process) */
#define MAX_MSG_LEN     1024

char msgcontent[MAX_MSG_LEN];
sem_t mysem;

/*I got much of my message queue code from paxdiablo at 
 * https://stackoverflow.com/questions/5625845/mq-receive-message-too-long*/

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

static void serverReceive (mqd_t svrHndl) {
    int rc;
    char buffer[2048];
    clearBuffer(buffer);
    
    FILE *log = fopen("/tmp/yashd.log", "a");
    if (log == NULL)
    {
        printf("Error! can't open log file.");
        return;
    }
   
    rc = mq_receive (svrHndl, buffer, sizeof (buffer), NULL);
    if (rc < 0) {
        printf ("Error on server mq_receive.\n");
        exit (1);
    }
    
    fprintf(log, buffer);
    
    fclose(log);
    
}

void serverUp (void) {
    int rc;
    mqd_t svrHndl;
    struct mq_attr mqAttr;

    rc = mq_unlink (MSGQOBJ_NAME);

    mqAttr.mq_maxmsg = 10;
    mqAttr.mq_msgsize = 2048;
    svrHndl = mq_open (MSGQOBJ_NAME, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR, &mqAttr);
    if (svrHndl < 0) {
        printf ("Error on server mq_open.\n");
        exit (1);
    }
    
    while(true)
    {
        serverReceive(svrHndl);
    }
    
}

static void clientSend (char * message) {
    mqd_t cliHndl;
    int rc;
    //printf ("Client sending.\n");
    cliHndl = mq_open (MSGQOBJ_NAME, O_RDWR);
    if (cliHndl < 0) {
        printf ("Error on client mq_open.\n");
        exit (1);
    }
    
    rc = mq_send (cliHndl, message, strlen(message), 1);
    if (rc < 0) {
        printf ("Error on client mq_send.\n");
        exit (1);
    }

    mq_close (cliHndl);
}

void runLogger()
{
    int rc;
    pthread_t * listenerTid;
    char * args = "";
    int ret;
    ret = sem_init(&mysem, 0, 1);
    if (ret != 0) {
        /* error. errno has been set */
        perror("Unable to initialize the semaphore");
        abort();
    }
    
    //Spawn the listener
    if ((rc = pthread_create(&listenerTid, NULL, serverUp, (void*) args))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return;
    }
    

    return;
}


void logCommand(char *command, char *ip, int port, int priority)
{
    int ret;

    char *date = getTime();  
    clearBuffer(msgcontent);
    snprintf(msgcontent, MAX_MSG_LEN, "%s yashd[%s:%d]: %s\n", date, ip, port, command);
    
    //clientSend("\nNOTE: Entering Critical Section...\n");
    ret = sem_wait(&mysem);
    //clientSend("NOTE: In Critical Section\n");
    clientSend (msgcontent);
    //clientSend("NOTE: Leaving Critical Section\n\n");
    ret = sem_post(&mysem);
    
    clearBuffer(date);
    free(date);
    return;
}


