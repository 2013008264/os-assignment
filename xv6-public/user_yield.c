#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[])
{
	int pid = fork();
	if(pid == 0)
	{
		while(1)
		{
			printf(2,"Child\n"); 
			yield();
		}
	}else
	{
		while(1)
		{
			printf(2, "Parent\n");
			yield();
		}
	}
	exit();
}
