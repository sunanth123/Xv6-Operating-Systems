#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf(2, "Error, must have 2 args after chgrp\n");
		exit();
	}
	int gid = atoi(argv[1]);
	if(gid < 0 || gid > 32767)
	{
		printf(2, "Error, GID is out of bound\n");
		exit();
	}
	if(chgrp(argv[2], gid))
	{
		printf(2, "Error performing chgrp\n");
	}
	exit();
}
#endif
