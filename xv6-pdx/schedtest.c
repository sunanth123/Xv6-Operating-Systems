#include "types.h"
#include "user.h"

int main(void)
{
	int pid;
	for(int i = 0; i < 30; ++i)
	{
		pid = fork();
		if(pid)
		{
		}
		else
		{
			for(;;);
		}
	}

	if(pid)
	{
		for(int i = 0; i < 30; ++i)
		{
			wait();
		}
	}
	exit();
}
