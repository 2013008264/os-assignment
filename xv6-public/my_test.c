#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[])
{
	while(fork() > 0)
		;
	sleep(1000);
}
