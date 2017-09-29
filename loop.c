#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

int main()
{
	printf("Starting infinate loop...");

	while(1)
	{
		printf("TEST...\n");
		sleep(1);
	}

	return 0;
}
