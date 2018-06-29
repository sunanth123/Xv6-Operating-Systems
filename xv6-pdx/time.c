#ifdef CS333_P2
#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{

	int pid;
	int start;
	int end;
	int diff;
	int second;
	int ten;
	int hundred;

	start = uptime();

	if(!argv[1])
	{
		end = uptime();
		diff = end - start;
		second = diff/1000;
		ten = (diff - (second * 1000))/100;
		hundred = (diff - (second * 1000) - (ten * 100))/10;
		printf(1, "ran in %d.%d%d seconds\n", second, ten, hundred);
		exit();
	}

	pid = fork();
	
	if(pid)
	{
		wait();
	}
	if(!pid)
	{
		if(exec(argv[1], argv+1))
		{
			printf(2, "Error performing process\n");
			exit();
		}
	}
	
	end = uptime();

	diff = end - start;
	second = diff/1000;
	ten = (diff - (second * 1000))/100;
	hundred = (diff - (second * 1000) - (ten * 100))/10;
	
	printf(1, "%s ran in %d.%d%d seconds\n", argv[1], second, ten, hundred); 	
	
	exit();
}


#endif
