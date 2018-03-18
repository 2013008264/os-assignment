#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

//
// 2013008264 LEE
// Simple shell project
// Finished date 2018.03.16
//

#define BUF_SIZE 512

char ** split_semi(char * string, int count); // split string with ; token
char ** split_string(char * string); // split string with 'space' & '\n'
int r_wait(int * pid); // error return -1
int func_shell(char ** args); // error return 1
int func_cd(char ** args); // error return 1
int loop(void); // Launch the loop for interactive mode error return 1
int batch(FILE * fp); // Launch batch mode. error return 1
int exec_func(char ** args); //execute the function.

int main(int argc, char * argv[])
{
	if(argc == 1) 
	{
		//Execute interactive mode
		if(loop())
		{
			fprintf(stderr, "Error!\n");
			return 1;
		}
	}

	//Execute batch mode
	FILE * fp;
	int i;
	for(i = 1; i < argc; i++)
	{
		fp = fopen(argv[i], "r");
		if(batch(fp)){
			fprintf(stderr, "Failed to implementing batch\n");
		}
	}
}

int batch(FILE * fp)
{
	char input[512];
	int len, i, count;
	char ** argvs;
	char ** args;
	
	while(fgets(input, 512, fp) != NULL)
	{
		count = 0;
		len = strlen(input);

		for(i = 0; i < len; i++)
		{
			if(input[i] == ';' && input[i+1] != '\0' && input[i+1] != ';')
				count++;		
		}

		fprintf(stderr, "%s", input);	
		argvs = split_semi(input, count);
		
		for(i = 0; i <= count; i++)
		{
			if(argvs[i][0] == '\n')
				break;

			args = split_string(argvs[i]);
			if(args[0] != NULL)
			{
				if(exec_func(args))
				{
					fprintf(stderr, "Failed to exec_func\n");
					return 1;
				}
			}
			free(args);
		}

		//Wait all processes.
		while(r_wait(NULL) > 0){}
		free(argvs);
	}
	return 0;
}

int loop(void)
{
	char ** argvs;
	char ** args;
	int len, i, count;
	char input[BUF_SIZE];
	char cwd[BUF_SIZE];

	for(;;)
	{
		getcwd(cwd, BUF_SIZE);
		printf("prompt : %s > ", cwd);
		if(fgets(input, BUF_SIZE, stdin)==NULL)
		{
			printf("\n");
			exit(0);
		}
		len = strlen(input);
		count = 0;

		for(i = 0; i < len; i++)
		{
			if(input[i] == ';' && input[i+1] != '\0' && input[i+1] != ';')
				count++;
		}
		
		argvs = split_semi(input, count);
		
		for(i = 0; i <= count; i++)
		{
			if(argvs[i][0] == '\n')
				break;

			args = split_string(argvs[i]);
			if(args[0] != NULL)
			{
				if(exec_func(args))
				{	
					fprintf(stderr, "Failed to exec_func\n");
					return 1;
				}
				free(args);
			}
		}
		
		//Wait all processes.		
		while(r_wait(NULL) > 0){}
		free(argvs);
	}
}

char ** split_string(char * string)
{
	char * ptr;
	int len = strlen(string);
	char ** result;
	int i = 0;

	result = (char**)malloc(sizeof(char*)*len);

	result[i++] = strtok_r(string, " \n", &ptr);
	while((result[i++] = strtok_r(NULL, " \n", &ptr)) != NULL){}
	
	return result;
}

char ** split_semi(char * string, int count)
{
	char * ptr;
	int i;

	char ** result = (char**)malloc(sizeof(char *)*(count+1));
	
	result[0] = strtok_r(string, ";", &ptr);
	
	for(i = 1; i <= count; i++){
		result[i] = strtok_r(NULL, ";", &ptr);
	}
	return result;
}

int func_cd(char ** args)
{
	if(strcmp(args[0], "cd"))
	{
		fprintf(stderr, "Failed to cd : args[0] is not cd\n");
		return 1;
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

int func_shell(char ** args)
{
	int childpid;
	childpid = fork();
	
	if(childpid == -1)
	{
		perror("Failed to fork");
		return 1;
	}

	if(childpid == 0)
	{
		execvp(args[0], args);
		perror("Failed to exec");
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

int exec_func(char ** args)
{
	if(!strcmp(args[0], "cd"))
		return func_cd(args);
	else if(!strcmp(args[0], "exit") || !strcmp(args[0], "quit"))
		exit(0);
	else
		return func_shell(args);
}
