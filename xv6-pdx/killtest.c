#include "types.h"
#include "user.h"

int main(void)
{
	int pid = fork();
	if(pid)
	{
		sleep(1000);
		printf(1, "perform kill shell command...\n");
		sleep(10000);
		while(wait() == -1)
		{
		}
		printf(1, "reaped pid: %d\n", pid);
		sleep(10000);
	}
	else
	{
		for(;;);
	}
	exit();
}
