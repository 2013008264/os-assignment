#include "types.h"
#include "defs.h"

//Simple system call
//2018.03.22

int 
myfunction(char * str)
{
	//in xv6 we use cprintf instead printf
	cprintf("%s\n", str);
	return 0xABCDABCD;
}

int
sys_myfunction(void)
{
	char * str;
	if(argstr(0, &str) < 0)
		return -1;
	return myfunction(str);
}
