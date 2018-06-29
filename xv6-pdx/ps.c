#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"
#define MAX 16
#define TESTCOUNT 4


//this is test function that will be called in main for test case purpose. In order to activate test just include another argument after ps command. 
void 
test()
{
  int entries;
  int max[4] = {1, 16, 64, 72};
  int i;
  int z;
  int el_seconds;
  int el_ten;
  int el_hundred;
  int el_thousand;
  int cpu_seconds;
  int cpu_ten;
  int cpu_hundred;
  int cpu_thousand;
  int diff;


  for (z = 0; z < TESTCOUNT; ++z)
  {
	  struct uproc* table = malloc(max[z] * sizeof(struct uproc));
	  entries = getprocs(max[z], table);
	  if(entries < 1)
	  {
		printf(2, "Unable to create uproc table\n");
		exit();
	  }
	  printf(1, "\n");
	  printf(1, "TEST MAX: %d\n", max[z]);
	  printf(1, "PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\t  Size\n");
	 
	  for(i = 0; i < entries; ++i)
	  {
		diff = table[i].elapsed_ticks;
		el_seconds = (diff)/1000;
		el_ten = (diff - (el_seconds * 1000))/100;
		el_hundred = (diff - (el_seconds * 1000) - (el_ten * 100))/10;
		el_thousand = (diff - (el_seconds * 1000)) - (el_ten * 100) - (el_hundred * 10);
		diff = table[i].CPU_total_ticks;
		cpu_seconds = (diff)/1000;
		cpu_ten = (diff - (cpu_seconds * 1000))/100;
		cpu_hundred = (diff - (cpu_seconds * 1000) - (cpu_ten * 100))/10;
		cpu_thousand = (diff - (cpu_seconds * 1000)) - (cpu_ten * 100) - (cpu_hundred * 10);

		printf(1, "%d\t%s\t%d\t%d\t%d\t%d\t%d.%d%d%d\t%d.%d%d%d\t%s\t  %d\n", table[i].pid, table[i].name, table[i].uid, table[i].gid, table[i].ppid, table[i].priority, el_seconds, el_ten, el_hundred, el_thousand, cpu_seconds, cpu_ten, cpu_hundred, cpu_thousand, table[i].state, table[i].size);
	  }
	  
	  free(table);  
  }
}



int
main(int argc, char* argv[])
{
  int entries;
  int max = MAX;
  int i;
  uint el_seconds;
  uint el_ten;
  uint el_hundred;
  uint el_thousand;
  uint cpu_seconds;
  uint cpu_ten;
  uint cpu_hundred;
  uint cpu_thousand;
  uint diff;

  if(argc > 1)
  {
	test();
	exit();
  }

  struct uproc* table = malloc(max * sizeof(struct uproc));
  entries = getprocs(max, table);
  if(entries < 1)
  {
	printf(2, "Unable to create uproc table\n");
	exit();
  }
  printf(1, "PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\t  Size\n");
 
  for(i = 0; i < entries; ++i)
  {
        diff = table[i].elapsed_ticks;
	el_seconds = (diff)/1000;
        el_ten = (diff - (el_seconds * 1000))/100;
	el_hundred = (diff - (el_seconds * 1000) - (el_ten * 100))/10;
	el_thousand = (diff - (el_seconds * 1000)) - (el_ten * 100) - (el_hundred * 10);
	diff = table[i].CPU_total_ticks;
	cpu_seconds = (diff)/1000;
	cpu_ten = (diff - (cpu_seconds * 1000))/100;
	cpu_hundred = (diff - (cpu_seconds * 1000) - (cpu_ten * 100))/10;
	cpu_thousand = (diff - (cpu_seconds * 1000)) - (cpu_ten * 100) - (cpu_hundred * 10);

	printf(1, "%d\t%s\t%d\t%d\t%d\t%d\t%d.%d%d%d\t%d.%d%d%d\t%s\t  %d\n", table[i].pid, table[i].name, table[i].uid, table[i].gid, table[i].ppid, table[i].priority, el_seconds, el_ten, el_hundred, el_thousand, cpu_seconds, cpu_ten, cpu_hundred, cpu_thousand, table[i].state, table[i].size);
  }
  
  free(table);  

  exit();
}
#endif
