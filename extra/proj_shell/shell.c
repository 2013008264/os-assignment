#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

//
// HYU 2013008264 LEE
// Simple shell project
// Update date 03/23/2018
//

#define BUF_SIZE 512

char ** split_semi(char * string, int count); // split string with ; token
char ** split_string(char * string); // split string with 'space' & '\n'
int r_wait(int * pid); // error return -1
int func_cd(char ** args); // error return 1
int loop(FILE * fp); // Launch the loop for interactive mode error return 1
int exec_func(char ** args); //execute the function.

int main(int argc, char * argv[])
{
	if(argc == 1) 
	{
		//Execute interactive mode
		if(loop(stdin))
			return 1;
	}

	//Execute batch mode
	FILE * fp;
	int i;
	for(i = 1; i < argc; i++)
	{
		fp = fopen(argv[i], "r");
		if(fp == NULL){
			perror("Failed to open file");
			continue;
		}
		else if(loop(fp)){
			return 1;
		}fclose(fp);
	}
	return 0;
}

int loop(FILE * fp)
{
	char ** argvs;
	char ** args;
	int len, i, count;
	char input[BUF_SIZE];
	char cwd[BUF_SIZE];
	int childpid;
	
	for(;;)
	{
		if(fp == stdin){
			getcwd(cwd, BUF_SIZE);
			printf("prompt : %s > ", cwd);
		}
		if(fgets(input, BUF_SIZE, fp) == NULL)
		{
			if(fp == stdin)
				printf("\n");
			return 0;
		}
		if(fp != stdin)
		{
			printf("%s", input);
		}
		len = strlen(input);
		count = 0;
		
		for(i = 0; i < len; i++)
		{
			//To count effective ';'
			if(input[i] == ';' && input[i+1] != '\0' && input[i+1] != ';')
				count++;
		}

		argvs = split_semi(input, count);
		
		for(i = 0; i <= count; i++)
		{
			//To avoid segmentation fault when user input is only single character ';'
			if(argvs[i][0] == '\n')
				break;	

			args = split_string(argvs[i]);
			//To avoid segmentation fault when user input is only single character ' '
			if(args[0] == NULL);
			else if(!(strcmp(args[0], "quit")) || !(strcmp(args[0],"exit")))
				return 0;
			else if(!strcmp(args[0], "cd"))
				func_cd(args);
			else{
				childpid = fork();
				if(childpid == 0){
					if(args[0] != NULL)
					{	
						execvp(args[0], args);
						perror("Failed to execute");
						return 1;
					}
				}
			}
			free(args);
		}
		//Wait all processes.		
		while(r_wait(NULL) > 0){}
		free(argvs);
	}
}

char ** split_string(char * string)
{
	char * ptr, **result;
	int len = strlen(string), i = 0;
	
	if((result = (char**)malloc(sizeof(char*)*len)) == NULL)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}
	result[i++] = strtok_r(string, " \n", &ptr);
	while((result[i++] = strtok_r(NULL, " \n", &ptr)) != NULL){}
	
	return result;
}

char ** split_semi(char * string, int count)
{
	char * ptr, ** result;
	int i;

	if((result = (char**)malloc(sizeof(char *)*(count+1))) == NULL)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}
	
	result[0] = strtok_r(string, ";", &ptr);
	for(i = 1; i <= count; i++)
		result[i] = strtok_r(NULL, ";", &ptr);
	
	return result;
}

int func_cd(char ** args)
{
	if(strcmp(args[0], "cd") || args[1] == NULL)
	{
		fprintf(stderr, "Failed to cd : args[0] is not cd or args[1] is NULL pointer\n");
		return 0;
	}
	if(!strcmp(args[1], "~"))
	{
		struct passwd * user;
		user = getpwuid(getuid());
		char * username;
		username = user->pw_name;
		char buf[BUF_SIZE] = "/home/";
		strcat(buf, username);
		args[1] = buf;
	}
	
	if(chdir(args[1]))
	{
		fprintf(stderr, "Failed to cd : error in chdir func check directory name\n");
		return 0;
	}
	
	if(args[2] != NULL)
	{
		fprintf(stderr, "Failed to cd : there are too many arguments\n");
		return 1;
	}

	return 0;
}

int r_wait(int * pid)
{
	//Wait even if interrupted.
	int retval;
	while(((retval = wait(pid)) == -1) && (errno == EINTR)){};
	return retval;
}
