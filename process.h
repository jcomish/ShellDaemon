#ifndef PROCESS_H
#define	PROCESS_H
typedef struct Job job;

void buildArgs(char *** commands, char ** commandArgs, int pos, bool ISDEBUG);
int processCommands(char ***command, pid_t shell_pgid_temp, struct job * pJobTable,  int *jobSize, char * userInput, bool ISDEBUG);
void signalHandler(int pidToHandle);

#endif	/* PROCESS_H */

