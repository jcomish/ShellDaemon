#ifndef LOGGER_H
#define	LOGGER_H

char * getTime();
void serverUp (void);
void clientSend (char * message);
void runLogger();
void logCommand(char *command, char *ip, int port, int priority);


#endif	/* PROCESS_H */