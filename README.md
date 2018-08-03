# xv6
xv6 with MLFQ, FCFS, Priority scheduler &amp; Copy on write, Lazy allocation

### Questions
  * game9777@hanyang.ac.kr
  * game9777@naver.com

### MLFQ & Priority queue
 * Data structure
    * I use multi level list implemented queue.
    * O(1) Select with head pointer.
    * O(n) Find.
    
### FCFS
  * Data structure
    * Linked list. (Very simple to implement)
    * O(1) Select with head pointer.
    * O(1) Insert with tail pointer.
    * O(1) Delete.
    * O(n) Find.
    
### Copy on write & Lazy allocation
  * Just implemented... nothing to explain..
  * Changed code
    * vm.c
    * trap.c 
    * etc..

You can see full changes in pull requests
 * [Scheduler](https://github.com/2013008264/xv6/pull/3)
 * [Virtual memory](https://github.com/2013008264/xv6/pull/4)
