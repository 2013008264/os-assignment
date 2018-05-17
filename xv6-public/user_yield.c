#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[])
{
/*	int pid = fork();
	if(pid < 0)
	{
		printf(2, "Fork error!\n");
		exit();
	}
	if(pid == 0)
	{
		for(int i = 0; i < 100; i++)
		{
			printf(1,"ppid : %d\n", getppid());
			printf(2,"Child\n"); 
			yield();
		}
	}else
	{
		for(int i = 0; i < 100; i++)
		{
			printf(1, "ppid : %d\n", getppid());
			printf(2, "Parent\n");
			yield();
		}
	}*/
	setpriority(getpid(), 100);
	printf(1, "Hello xv6!\n");
	
	exit();
}
