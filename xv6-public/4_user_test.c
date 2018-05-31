#include "types.h"
#include "stat.h"
#include "user.h"

int 
main(int argc, char * argv[])
{
  int size = 1000000;
  int first = get_n_free_pages();
  printf(1, "%d this size will be sbrk right now!\n", size);
  printf(1, "Init . Number of free frames :\t\t %d\n", get_n_free_pages());
  char * abc = sbrk(size);
  printf(1, "sbrk finished, number of free frames :\t %d\n", get_n_free_pages());
  printf(1, "Now access this process' sbrked data\n");
  for(int i = 0; i < 1000000; i++)
    abc[i] = 100;
  printf(1, "Finished. Number of free frames :\t %d\n", get_n_free_pages());
  printf(1, "Now process' returns its sbrked frame\n");
  printf(1, "Before that, I make child process\n");
  int origin  = get_n_free_pages();
  int pid = fork();
  int after_fork = get_n_free_pages();
  if(pid == 0) {
    printf(1, "###################################\n");
    printf(1, "I am Child process\n");
    printf(1, "My own page number is    %d\n", origin - after_fork);
    printf(1, "My shared page number is %d\n", first - origin);
    printf(1, "Child returns its heap resource\n");
    abc = sbrk(-1000000);
    printf(1, "Finished\n");
    printf(1, "Actual returned frame :  %d\n", get_n_free_pages() - after_fork);
    printf(1, "Child exit\n");
    //printf(1, "###################################\n");
    exit();
  }
  wait();
  printf(1, "Actual returned frame :  %d\n", get_n_free_pages() - after_fork);
  printf(1, "Child deallocated\n");    
  printf(1, "###################################\n");

  printf(1, "###################################\n");
  printf(1, "Parent will return heap resoruces\n");
  origin = get_n_free_pages();
  abc = sbrk(-1000000);
  printf(1, "Actual returned frame : %d\n", get_n_free_pages() - origin);
  printf(1, "Finished. Number of free frames :\t %d\n", get_n_free_pages());

  exit();
}
