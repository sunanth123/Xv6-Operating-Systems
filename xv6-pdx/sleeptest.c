#include "types.h"
#include "user.h"
#include "param.h"

int main(void)
{
/*	int pid = fork();
	if(!pid)
	{
		sleep(10000);
		exit();
	}
	if(pid)
	{
		printf(1, "Parent is sleeping\n");
		wait();
		printf(1, "Parent woken up\n");
		for(;;);
	}
	exit();*/

	int pid = getpid();
	printf(1, "entering sleep\n");
	if(setpriority(pid,MAX))
	{
		printf(1, "error");
	}
	printf(1, "setting priority of sleep process to %d\n", MAX);
	sleep(50000);
	printf(1, "woke up from sleep and now running\n");
	if(setpriority(pid,MAX))
	{
		printf(1, "error");
	}
	printf(1, "setting priority of running process to %d\n", MAX);
	for(;;);
	exit();
}	
