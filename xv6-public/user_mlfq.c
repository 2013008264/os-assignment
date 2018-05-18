#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[])
{
	int pid = fork();
	if(pid < 0) {
		printf(1, "Process allocation failed..\n");
		exit();
	}
	if(pid > 0) {
			for(int i = 0; i < 300; i++) {
				printf(1, "parent, lev : %d\n", getlev());
				yield();
			}
			wait();
	}
	else if (pid == 0){
		
		int cpid = fork();

		if(cpid < 0) {
			printf(1, "Process allocation failed..\n");
			exit();
		}

		if(cpid == 0) {
			setpriority(getpid(), 100);
			for(int i = 0; i < 100; i++) {
				printf(1, "Grandchild, lev : %d\n", getlev());
				yield();
			}
		}else if(cpid > 0) {
			for(int i = 0; i < 100; i++) {
				if(i == 50)
					setpriority(getpid(), 2);
				printf(1, "child, lev : %d\n", getlev());
				yield();
			}
			wait();
		}
	}
	exit();
}
