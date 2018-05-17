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
		//setpriority(100);
			for(int i = 0; i < 300; i++) {
				printf(1, "parent\n");
				yield();
			}
			wait();
	}
	else if (pid == 0){
		//setpriority(2);
		
		int cpid = fork();

		if(cpid < 0) {
			printf(1, "Process allocation failed..\n");
			exit();
		}

		if(cpid == 0) {
			for(int i = 0; i < 100; i++) {
				printf(1, "Grandchild\n");
				yield();
			}
			//printf(1, "Grand child parent pid : %d\n", getppid());
		}else if(cpid > 0) {
			//setpriority(2);
			for(int i = 0; i < 100; i++) {
				if(i == 50) 
					setpriority(getpid(), 2);
				if(i == 75)
					setpriority(cpid, 100);
				printf(1, "child\n");
				yield();
			}
			wait();
			//printf(1, "Child's parent pid : %d\n", getppid());
		}
	}
	exit();
}
