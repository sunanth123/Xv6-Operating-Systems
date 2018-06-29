#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf(2, "Error, must have 2 args after chown\n");
		exit();
	}
	int uid = atoi(argv[1]);
	if(uid < 0 || uid > 32767)
	{
		printf(2, "Error, UID is out of bound\n");
		exit();
	}
	if(chown(argv[2], uid))
	{
		printf(2, "Error performing chown\n");
	}
	exit();
}

#endif
