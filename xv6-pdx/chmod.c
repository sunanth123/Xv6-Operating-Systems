#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf(2, "Error, expected 2 args after chmod\n");
		exit();
	}
	char* mode = argv[1];
	
	if(strlen(mode) != 4)
	{
		printf(2, "Error, expect 4 octal numbers for mode\n");
		exit();
	}
	if(mode[0] != '0' && mode[0] != '1')
	{
		printf(2, "Error, first octal digit is 0 or 1 (setuid)\n");
		exit();
	}
	for(int i = 1; i < 4; ++i)
	{
		if(mode[i] < '0' || mode[i] > '7')
		{
			printf(2, "Error, must only include octal digits for mode\n");
			exit();
		}
	}

	int dec_mode;
	int pow = 0;
	int count = 0;
	for(int i = 3; i >= 0; --i)
	{
		if(!pow)
		{
			pow = 1;
		}
		else
		{
			pow = 1;
			for(int x = 0; x < count; ++x)
			{
				pow *= 8;
			}
		}	
		++count;
		dec_mode += ((mode[i] - '0') * (pow));
	}

	if(chmod(argv[2], dec_mode))
	{
		printf(2, "Error performing chmod\n");
	}
        exit();
}

#endif
