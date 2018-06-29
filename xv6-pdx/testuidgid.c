#include "types.h"
#include "user.h"

int
main (int argc, char *argv[])
{
	uint uid;
	uint gid;
	uint ppid;

	uid = getuid();
	printf(1, "Current UID is: %d\n", uid);

	printf(1, "Setting UID to 100000\n");
	if(setuid(100000))
	{
		printf(2, "Error: setuid call failed. %s at line %d\n", __FILE__, __LINE__);
	}

	printf(1, "Setting UID to -1\n");
	if(setuid(-1))
	{
		printf(2, "Error: setuid call failed. %s at line %d\n", __FILE__, __LINE__);
	}

	printf(1, "Setting UID to 100\n");
	if(setuid(100))
	{
		
		printf(2, "Error: setuid call failed. %s at line %d\n", __FILE__, __LINE__);
	}
	uid = getuid();
	printf(1, "Current UID is: %d\n", uid);

	gid = getgid();
	printf(1, "Current GID is: %d\n", gid);

	printf(1, "Setting GID to 100000\n");
	if(setgid(100000))
	{
		printf(2, "Error: setgid call failed. %s at line %d\n", __FILE__, __LINE__);
	}

	printf(1, "Setting GID to -1\n");
	if(setgid(-1))
	{
		printf(2, "Error: setgid call failed. %s at line %d\n", __FILE__, __LINE__);
	}

	printf(1, "Setting GID to 100\n");
	if(setgid(100))
	{
		
		printf(2, "Error: setgid call failed. %s at line %d\n", __FILE__, __LINE__);
	}
	uid = getgid();
	printf(1, "Current GID is: %d\n", uid);

	ppid = getppid();
	printf(1, "My parent process is: %d\n", ppid);
	printf(1, "Done!\n");
	
	exit();
}
	


