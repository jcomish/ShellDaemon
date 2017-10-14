#ifndef PROCESS_H
#define	PROCESS_H
typedef struct Job job;
typedef struct process_vars;



void buildArgs(char *** commands, char ** commandArgs, int pos, bool ISDEBUG);
int processCommands(pid_t shell_pgid_temp, int socket, struct process_vars * process_vars_ptr);
void signalHandler(int pidToHandle);
int getForeGroundPid();
void resetStdIo();

#endif	/* PROCESS_H */

