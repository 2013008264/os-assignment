#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char * argv[])
{
  char * arg[3] = { "ls", "Hi!", 0 };
  int pid = fork();
  if(pid < 0) {
    printf(1, "fork failed...\n");
    exit();
  }
  
  if(pid > 0) {
    printf(1, "fork successed...\n");
    printf(1, "But if dragoon....\n");
    pid = fork();
    if(pid == 0) {
      exec("echo", arg);
      printf(1, "exec failed...\n");
    }
    char * arg1[2] = { "ls", 0 };
    if(pid > 0) {
      exec("ls", arg1);
      printf(1, "exec failed...\n");
    }
    wait();
  }
  char * arg2[3] = { "cat", "README", 0 };
  if(pid == 0) {
    exec("cat", arg2);
    printf(1, "exec failed...\n");
  }
  exit();
}
