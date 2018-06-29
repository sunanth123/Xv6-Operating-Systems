#include "types.h"
#include "user.h"
#include "param.h"

int main(void)
{
	int pid;
	int priority = 0;
	for (int i = 0; i < 11; ++i)
	{
		pid = fork();
		if(!pid)
		{
			for(;;);
		}
	}
	if(pid)
	{
		sleep(1000);
		for(;;)
		{
			sleep(10000);
			printf(1, "Setting PID 4 to priority %d\n", priority);
			if(setpriority(4, priority))
			{
				printf(1, "Error setting PID 4 to priority %d\n", priority);
			}
			++priority;
		}
	}

	exit();
}
/*		
#include "types.h"
#include "user.h"

// Must match NPRIO in proc.h
#define PrioCount 7
#define numChildren 10

void
countForever(int i)
{
  int j, p, rc;
  unsigned long count = 0;

  j = getpid();
  p = i%PrioCount;
  rc = setpriority(j, p);
  if (rc == 0) 
    printf(1, "%d: start prio %d\n", j, p);
  else {
    printf(1, "setpriority failed. file %s at %d\n", __FILE__, __LINE__);
    exit();
  }

  while (1) {
    count++;
    if ((count & (0x1FFFFFFF)) == 0) {
      p = (p+1) % PrioCount;
      rc = setpriority(j, p);
      if (rc == 0) 
	printf(1, "%d: new prio %d\n", j, p);
      else {
	printf(1, "setpriority failed. file %s at %d\n", __FILE__, __LINE__);
	exit();
      }
    }
  }
}

int
main(void)
{
  int i, rc;

  for (i=0; i<numChildren; i++) {
    rc = fork();
    if (!rc) { // child
      countForever(i);
    }
  }
  // what the heck, let's have the parent waste time as well!
  countForever(1);
  exit();
}
*/			 	
