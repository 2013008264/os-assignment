#include "types.h"
#include "stat.h"
#include "user.h"

int 
main(int argc, char * argv[])
{
	int pid = fork();
	if(pid < 0){
		printf(1, "Fork failed\n");
		exit();
	}

	if(pid == 0) {
		int cpid = fork();
		if(cpid < 0)
			exit();
		if(cpid == 0) {
			for(;;)
				printf(1, "Grandchild, level : %d\n", getlev());
		}
		else
			for(;;)
				printf(1, "Child, level : %d\n", getlev());
	}else {
		for(;;)
			printf(1, "Parent, level : %d\n", getlev());
	}
	exit();
}
