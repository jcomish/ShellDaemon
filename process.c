#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>

#include "input.h"

int getForeGroundPid();
void signalHandler(int pidToHandle);

/*I figured out most of this from your tutorials, so you will notice
 *some pieces from them in here
 *
 */

typedef struct Job {
    int pid;
    int pgid;
    int jobNo;
    char ground;
    char *status;
    char command[250];
    //bool is
} Job;

int pipefd[2];
int status, pid_ch1, pid_ch2, pid, pgid;

struct Job * processJobTable;
int * jobSize;
bool ISDEBUG;
int stdintemp;
int stdouttemp;

static void sig_int(int signo) {
    //printf("Sending signals to group:%d\n",pid_ch1); // group id is pid of first in pipeline
    kill(getForeGroundPid(),SIGINT);
    //Needed to do this so you can repeadetedly interrupt
}
static void sig_tstp(int signo) {
    //printf("Sending signals to group:%d\n",pid_ch1); 
    kill(getForeGroundPid(),SIGSTOP);
}
static void sig_chld(int signo) {
    //printf("Sending signals to group:%d\n",pid_ch1); 
    //kill(pid_ch1,SIGSTOP);
    //printf("signal(SIGCHLD) error");
}

void setupSignals()
{
    if (signal(SIGINT, sig_int) == SIG_ERR)
        if (ISDEBUG){printf("signal(SIGINT) error");}
    if (signal(SIGTSTP, sig_tstp) == SIG_ERR)
        if (ISDEBUG){printf("signal(SIGTSTP) error");}
    if (signal(SIGCHLD, sig_chld) == SIG_ERR)
        printf("signal(SIGCHLD) error");

}

bool isBackgroundProcess(int pid)
{
    int i;
    for (i = 0; i < *jobSize; i++)
    {
        if (processJobTable[i].pid == pid)
        {
            if (processJobTable[i].ground == '-')
            {
                return true;
            }
        }
    }
    return false;
}

void redirectInput(char ***commands, int inputRedirectIndex)
{
    //REQUIREMENT: < will replace stdin with the file that is the next token
    if (ISDEBUG){printf("DEBUG: Redirecting Input to %s\n", commands[inputRedirectIndex + 1][0]);}
    int in = open(commands[inputRedirectIndex + 1][0], O_RDONLY);
    //Error out if the file does not exist
    if (in == -1)
    {
        //REQUIREMENT: fail command if input redirection (a file) does not exist
        printf("ERROR: \"%s\" is not a valid file\n", commands[inputRedirectIndex + 1][0]);

        return;
    }
    else
    {
        dup2(in, 0);
        close(in);
    }
}

void redirectOutput(char ***commands, int outputRedirectIndex)
{
    //REQUIREMENT: > will replace stdout with the file that is the next token
    //REQUIREMENT: with creation of files if they don't exist for output redirection
    if (ISDEBUG){printf("DEBUG: Redirecting Output to %s\n", commands[outputRedirectIndex + 1][0]);}
    int out = open(commands[outputRedirectIndex + 1][0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    dup2(out, 1);
    close(out);
}

int getForeGroundPid()
{
    int i;
    int currentPid = -1;
    for (i = 1; i <= *jobSize; i++)
    {
        if (processJobTable[i].ground == '+')
        {
            currentPid = processJobTable[i].pid;
        }
    }

    return currentPid;
}

int getLatestStoppedProcessId()
{
    int i;
    int currentLargestValue = 0;
    int currentPid = -1;
    for (i = 1; i <= *jobSize; i++)
    {
        if (processJobTable[i].ground == '-' &&
                processJobTable[i].jobNo > currentLargestValue)
        {
            currentLargestValue = processJobTable[i].jobNo;
            currentPid = processJobTable[i].pid;
        }
    }

    return currentPid;
}

int getCurrentProcessId()
{
    int i;
    int currentLargestValue = 0;
    int currentPid = -1;
    for (i = 1; i <= *jobSize; i++)
    {
        if (processJobTable[i].jobNo > currentLargestValue)
        {
            currentLargestValue = processJobTable[i].jobNo;
            currentPid = processJobTable[i].pid;
        }
    }

    return currentPid;
}

int getLatestJobNo()
{
    int i;
    int currentLargestValue = 1;
    
    //handle first case
    if (*jobSize < 1)
    {
        return 0;
    }
    
    for (i = 1; i <= *jobSize; i++)
    {
        if (processJobTable[i].jobNo > currentLargestValue)
        {
            currentLargestValue = processJobTable[i].jobNo;
        }
    }

    return currentLargestValue;
}

int findJobIndex(int pid)
{
    int i;
    for (i = 1; i <= *jobSize; i++)
    {
        if (processJobTable[i].pid == pid)
            return i;
    }
    //The pid does not exist
    return -1;
}

void writeToJobTable(char * userInput, int pgid, int pid, bool fg)
{
    int jobNo = getLatestJobNo() + 1;
    processJobTable[jobNo].pid = pid;
    processJobTable[jobNo].pgid = pgid;
    
    //REQUIREMENT: with a [<jobnum>]
    processJobTable[jobNo].jobNo = jobNo;

    //REQUIREMENT: a + or - indicating  the current job. Which job would be run with an fg,
    //is indicated with a + and, all others with a -
    if (fg)
    {
        processJobTable[jobNo].ground = '+';
    }
    else
    {
        processJobTable[jobNo].ground = '-';
    }

    //REQUIREMENT: a “Stopped” or “Running” indicating the status of the process
    processJobTable[jobNo].status = malloc(sizeof(char));
    processJobTable[jobNo].status = "Running";
    //REQUIREMENT: and finally the original command
    strcpy(processJobTable[jobNo].command, userInput);
    
    *jobSize += 1;
    
    return;
}

void printJob(struct Job job, int i)
{
    if (strcmp(job.status, "Stopped") == 0 || strcmp(job.status, "Running") == 0)
    {
        printf("[%d] %c %s   %s", i, job.ground, job.status, job.command);
        if(ISDEBUG){printf("      %d  %d",job.pid, job.pgid);}
        printf("\n");
    }  
    return;
}

void printJobTable()
{
    int i;
    
    for (i = 1; i < (*jobSize) + 1; i++)
    {
        printJob(processJobTable[i], i);
    }
    return;
}

bool isShellProcess(char ***commands)
{
    int fgIndex = containsCommand(commands, "fg");
    int bgIndex = containsCommand(commands, "bg");
    int jobsIndex = containsCommand(commands, "jobs");
    int jobPid;
    int jobIndex;

    
    if (fgIndex == 0 && countArgs(commands[0]) == 2)
    {
        if (containsCharacter(commands[0][1], '%'))
        {
            char *jobIndexChar = commands[0][1];
            memmove(commands[0][1], commands[0][1] + 1, strlen(commands[0][1]));
            jobIndex = atoi(jobIndexChar);
            jobPid = processJobTable[jobIndex].pid;
        }
        else
        {
            char *jobPidChar = commands[0][1];
            jobPid = atoi(jobPidChar);
            jobIndex = findJobIndex(jobPid);
        }
        if (processJobTable[jobIndex].ground == '+')
        {
            printf("ERROR: Cannot bring a process to the forground that is already there.\n");
            return true;
        }
    }
    else
    {
        jobPid = getLatestStoppedProcessId();
        jobIndex = findJobIndex(jobPid);
    }
    pid_ch1 = jobPid;
    
    if (jobsIndex == 0)
    {
        printJobTable(jobSize);
        return true;
    }
    if (fgIndex == 0)
    {     
        //REQUIREMENT: fg must send SIGCONT to the most recent background or stopped process, 
        //             print the process name  to stdout, and wait for completion
        kill(jobPid, SIGCONT);
        
        processJobTable[jobIndex].ground = '+';
        processJobTable[jobIndex].status = "Running";
        printJob(processJobTable[jobIndex], 1);
        
        processJobTable[jobIndex].status = "Done";
        
        status = 0;
        
        setupSignals();
        pid = waitpid(jobPid, &status, WUNTRACED | WCONTINUED);
        
        signalHandler(jobPid);
        if (pid == -1) 
        {
            perror("waitpid");
            return false;
        }
        
        
        return true;
    }
    //REQUIREMENT: bg must send SIGCONT to the most recent stopped process, print 
    //             the process name to stdout in the jobs format, and not wait for 
    //             completion (as if &)
    if (bgIndex == 0)
    {
        if (countArgs(commands[0]) == 1)
        {
            char *jobIndexChar = commands[0][0];
            memmove(commands[0][0], commands[0][0] + 1, strlen(commands[0][0]));
            jobIndex = atoi(jobIndexChar);
            jobPid = processJobTable[jobIndex].pid;
        }
        else if (countArgs(commands[0]) == 2)
        {
            char *jobPidChar = commands[0][1];
            jobPid = atoi(jobPidChar);
            jobIndex = findJobIndex(jobPid);
        }
        else
        {
            jobPid = getLatestStoppedProcessId();
            jobIndex = findJobIndex(jobPid);
        }
        
        kill(jobPid, SIGCONT);
        processJobTable[jobIndex].status = "Running";
        waitpid(jobPid, &status, WNOHANG);
        fflush(stdin);
        fflush(stdout);
        tcsetpgrp(processJobTable[0].pid, STDOUT_FILENO);
        tcsetpgrp(processJobTable[0].pid, STDIN_FILENO);
        //signalHandler(jobPid);
        return true;
        //REQUIREMENT: Terminated background jobs will be printed after the 
        //             newline character sent on stdin with a Done in place of the Stopped or Running.
    }
    return false;
}

void signalHandler(int pidToHandle)
{
    int jobIndex = findJobIndex(pidToHandle);
    if (WIFEXITED(status)) 
        {
            if (ISDEBUG){printf("child %d exited, status=%d\n", pidToHandle, WEXITSTATUS(status));}
            processJobTable[jobIndex].status = "Done";
        } 
        else if (WIFSIGNALED(status)) 
        {
            //REQUIREMENT: Ctrl-c must quit current foreground process (if one exists)  
            //and not the shell and should not print the process (unlike bash)
            if (ISDEBUG){printf("child %d killed by signal %d\n", pidToHandle, WTERMSIG(status));}
            processJobTable[jobIndex].status = "Terminated";

            //This is for the pipes
            dup2(stdintemp, 0);
            close(stdintemp);
            dup2(stdouttemp, 1);
            close(stdouttemp);
            
            
            return;
        } 
        else if (WIFSTOPPED(status)) 
        {
            //REQUIREMENT: Ctrl-z must send SIGTSTP to the current foreground 
            //process and should not print the process (unlike bash)
            if (ISDEBUG){printf("%d stopped by signal %d\n", pidToHandle,WSTOPSIG(status));}
            processJobTable[jobIndex].ground = '-';
            processJobTable[jobIndex].status = "Stopped";
        } 
        else if (WIFCONTINUED(status)) 
        {
            if (ISDEBUG){printf("Continuing %d\n",pidToHandle);}
            processJobTable[jobIndex].status = "Running";
        }
}

void waitForSignals(int pipeIndex, int backgroundIndex)
{
    //REQUIREMENT: Background a job using &
    if (backgroundIndex == -1)
    {
        pid = waitpid(getCurrentProcessId(), &status, WUNTRACED | WCONTINUED);

        if (pid == -1) 
        {
            perror("waitpid");
            return;
        }
        
        signalHandler(pid);
        
    }
    return;
}

int processCommands(char ***commands, pid_t shell_pgid_temp, struct Job * jobTable, int * pJobSize, char * userInput, bool pISDEBUG)
{
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        if (ISDEBUG){printf("signal(SIGINT) error");}
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
        if (ISDEBUG){printf("signal(SIGTSTP) error");}
    stdouttemp = dup(1);
    stdintemp = dup(0);
    
    //set your globals
    ISDEBUG = pISDEBUG;
    processJobTable = jobTable;
    jobSize = pJobSize;
    
    processJobTable[0].pid = shell_pgid_temp;
    processJobTable[0].jobNo = 0;
    processJobTable[0].ground = 'N';
    processJobTable[0].status = malloc(sizeof(char));
    processJobTable[0].status = "NA";
    //REQUIREMENT: and finally the original command
    strcpy(processJobTable[0].command, "Shell");

    
    //REQUIREMENT: Background a job using &
    while (1)
    {
        pid = waitpid(-1, &status, WNOHANG);
        break;
    }
    
    //check for output redirect
    int outputRedirectIndex = containsCommand(commands, ">");
    int inputRedirectIndex = containsCommand(commands, "<");
    int pipeIndex = containsCommand(commands, "|");
    int backgroundIndex = containsCommand(commands, "&");
    
    if (!isShellProcess(commands))
    {
        if (pipeIndex != -1)
        {
            //Open pipe
            if (pipe(pipefd) == -1) 
            {
                perror("pipe");
                return 1;
            }
        }
        
        pid_ch1 = fork();
        if (pid_ch1 > 0)
        {
            if (pipeIndex == -1)
            {
                setupSignals(ISDEBUG);
                if(ISDEBUG){printf("Child1 pid = %d\n",pid_ch1);}

                writeToJobTable(userInput, pgid, pid_ch1, (backgroundIndex == -1));
                waitForSignals(pipeIndex, backgroundIndex);

                //Reset the FDs
                dup2(stdintemp, 0);
                close(stdintemp);
                dup2(stdouttemp, 1);
                close(stdouttemp);

                return 0;

            }

            if (pipeIndex != -1)
            {
                pid_ch2 = fork();
                if (pid_ch2 > 0)
                {
                    setupSignals(ISDEBUG);
                    
                    //REQUIREMENT: Also, you cannot use Ctrl-z to put a 
                    //pipelined command chain into background
                    signal(SIGTSTP, SIG_IGN);
                    if (ISDEBUG){printf("Child2 pid = %d\n",pid_ch2);}
                    
                    writeToJobTable(userInput, pgid, pid_ch2, true);

                    close(pipefd[0]);
                    close(pipefd[1]);
                    
                    waitForSignals(2, backgroundIndex);

                    dup2(stdintemp, 0);
                    close(stdintemp);
                    dup2(stdouttemp, 1);
                    close(stdouttemp);

                    return 0;
                }
                else 
                {
                    //REQUIREMENT: The right command will have 
                    //stdin replaced with the output from the same pipe
                    setpgid(0,pid_ch1);
                    close(pipefd[1]);
                    dup2(pipefd[0],STDIN_FILENO);
                    if (execvp(commands[2][0], commands[2]) != 0)
                    {
                        return -1;
                    }
                }
            }
        } 
        else 
        {
            //RESET Signals to default. Just doing this to be 100% sure...
            if (signal(SIGINT, SIG_DFL) == SIG_ERR)
                if (ISDEBUG){printf("signal(SIGINT) error");}
            if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
                if (ISDEBUG){printf("signal(SIGTSTP) error");}
            
            //Redirect input if in line
            if (inputRedirectIndex != -1)
            {
                redirectInput(commands, inputRedirectIndex);
            }

            //Redirect output if in line
            if (outputRedirectIndex != -1)
            {
                redirectOutput(commands, outputRedirectIndex);
            }

            //Pipe if there is a pipe
            if (pipeIndex != -1)
            {
                //REQUIREMENT: The left command will have stdout replaced 
                //with the input to a pipe 
                if (ISDEBUG){printf("DEBUG: Piping\n");}
                close(pipefd[0]); // close the read end
                dup2(pipefd[1],STDOUT_FILENO);
            }

            pgid = setsid();
            if (execvp(commands[0][0], commands[0]) != 0)
            {
                return -1;
            }
        }
    }
    
    return 0;
}
