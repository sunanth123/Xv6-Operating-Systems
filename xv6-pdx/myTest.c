#include "types.h"
#include "user.h"

int main(void)
{
/*
	int i;

	for (i = 0; i < 11; ++i)
	{
		if(fork() == 0)
		{
			sleep(60000);
			exit();
		}
	}
	
	for (i = 0; i < 11; ++i)
	{
		wait();
	}
*/
	int pid;
	for(int i = 0; i < 20; ++i)
	{
		pid = fork();
		if(!pid)
		{
			for(;;);
		}
	}
	if(pid)
	{
		for(int i = 0; i < 20; ++i)
		{
			wait();
		}
	}
	exit();
}
		
