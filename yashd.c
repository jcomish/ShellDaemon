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
#include "process.h"

void clearBuffer(char * buf);

int sd, psd;

void reusePort(int s)
{
    int one=1;
    
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
}   

void setupSocket()
{
    struct   sockaddr_in server;
    sd = socket (AF_INET,SOCK_STREAM,0);
    
    
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = 3826;
    
    reusePort(sd);
    if ( bind( sd, (struct sockaddr *) &server, sizeof(server) ) < 0 ) {
	close(sd);
	perror("binding name to stream socket");
	exit(-1);
        }
    
    listen(sd,1);
    psd = accept(sd, 0, 0);
    
    char buf[1024];
    
    clearBuffer(buf);
    recv(psd,buf,sizeof(buf), 0);
    
    printf("'%s'\n", buf);
    send(psd, "#", 3, 0 );
    
}

void clearBuffer(char * buf)
{
    int i;
    for (i = strlen(buf); i > 0; i--)
    {
        buf[i] = '\0';
    }
}

int main(int argc, char** argv) 
{
   char   buf[1024];
   int    cc;

   setupSocket();
   
   for(;;) 
   {
       bool ISDEBUG = true;
       clearBuffer(buf);
       cc=recv(psd,buf,sizeof(buf), 0);
           if (cc == 0) exit (0);
       buf[cc] = '\0';
       printf("message received: %s", buf);
       char **stringList;
       stringList = splitInput(buf, ISDEBUG);
       char ***commandList = splitIntoCommands(stringList, ISDEBUG);
       
       printCommands(commandList);
       
       char *results;
       results = malloc(sizeof(char) * 1024);
       results = "Command Results";
       
       send(psd, results, 15, 0);
       send(psd, "\n#", 3, 0 );
       
   }

}

