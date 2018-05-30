#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[])
{
  printf(1, "Hello world!\n");
  printf(1, "Before fork : %d\n", get_n_free_pages());
  int pid = fork();
  if(pid == 0) {
    printf(1, "After fork : %d\n", get_n_free_pages());
    printf(1, "I am child\n");
  }
  if(pid) {
    wait();
    printf(1, "After exit : %d\n", get_n_free_pages());
  }
  
  exit();
}
