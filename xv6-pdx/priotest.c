#include "types.h"
#include "user.h"
#include "param.h"


int main(void)
{
	int pid;
	int child;
	for(int i = 0; i < 11; ++i)
	{
		pid = fork();
		if(!pid)
		{
			child = getpid();
			if(child%2)
			{
				setpriority(child, MAX);
			}
			for(;;);
		}		
	}

	if (pid)
	{
		for(int i = 0; i < 11; ++i)
		{
			wait();
		}
	}
	exit();
	
}
