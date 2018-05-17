#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct{
#ifdef PRIORITY_SCHED
	struct queue_proc queue_arr[NPROC];
#else
	struct queue_proc queue_arr[3];
#endif
	struct queue_proc * head_q;
	struct queue_proc * now_q;
	struct queue_proc * tail;
	int num_q;
} pqueue;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);
static void wakeup1(void *chan);

struct queue_proc * 
q_init(int priority) {
	struct queue_proc * q;
#ifdef PRIORITY_SCHED
	for(q = pqueue.queue_arr; q < &pqueue.queue_arr[NPROC]; q++)
#else
	for(q = pqueue.queue_arr; q < &pqueue.queue_arr[3]; q++)
#endif
		if(q->state == UNUSED_Q) {
			q->state = DELETABLE;
			q->next = q->prev = 0; q->head = q->tail = 0; q->num_proc = q->num_runnable = 0;
			q->priority = priority;
			pqueue.num_q++;
			return q;
		}
	return 0;
}

struct queue_proc * 
find_q(int priority) {
	struct queue_proc * tmp = pqueue.now_q;
	int i = 0;
	if(!pqueue.num_q)
		return 0;
	while(tmp->priority != priority) {
		tmp = tmp->prev;
		if(i++ == pqueue.num_q)
			return 0;
	}if(tmp->state != UNUSED_Q)
		return tmp;
	
	return 0;
}

void 
insert_queue(struct queue_proc * queue) {
	
	int priority = queue->priority;
	struct queue_proc * left = pqueue.head_q;
	struct queue_proc * right = left->next;
	int l_priority, r_priority;
	if(pqueue.num_q == 1) {
		pqueue.now_q = pqueue.tail = pqueue.head_q = queue;
		queue->next = queue->prev = queue;
		return ;
	}
	
	for(int i = 0; i < pqueue.num_q; i++) {
		l_priority = left->priority;	r_priority = right->priority;
		if((r_priority == pqueue.head_q->priority)) {
			left->next = queue;
			right->prev = queue;
			queue->prev = left;
			queue->next = right;
			pqueue.num_q++;
				
			pqueue.tail = queue;
		}
		else if ((l_priority < priority) &&(priority < r_priority)) {
			left->next= queue;
			right->prev = queue;

			queue->prev = left;
			queue->next = right;
			pqueue.num_q++;
			return;
		}
		left = left->next;
		right = right->next;
	} // Queue state will be added
}

void 
delete_queue(struct queue_proc * queue) {
	int priority = queue->priority;
	struct queue_proc *tmp = pqueue.now_q;
	for(int i = 0; i < pqueue.num_q; i++) {
		if(tmp->priority == priority)
			break;
		tmp = tmp->prev;
	}
	if(tmp->num_proc > 0) {
		return;
	}
	if(pqueue.num_q > 1) {
		tmp->prev->next = tmp->next;
		tmp->next->prev = tmp->prev;
		if(tmp->priority == pqueue.tail->priority) {
			pqueue.tail = tmp->prev;
		}
		if(tmp->priority == pqueue.head_q->priority)
			pqueue.head_q = tmp->next;
	}else {
		pqueue.tail = pqueue.head_q = pqueue.now_q = 0;
	}
	tmp->state = UNUSED_Q;
	pqueue.num_q--;
	// Queue state.
}
/* Fin */
void 
insert_proc(struct queue_proc * queue, struct proc * p) {
	if(queue == 0 || p == 0)
		return;
	if(!queue->num_proc) {
		queue->head = queue->tail = p;
		p->next = p;
		queue->num_proc++;
		
	}else {
		p->next = queue->head;
		queue->tail->next = p;
		queue->tail = p;
		queue->num_proc++;
	}if(p->state == RUNNABLE || p->state == RUNNING) {
		queue->num_runnable++;
	}
	if(queue->num_runnable)
		queue->state = RUNNABLE_Q;
}
/* Fin */
void 
delete_proc(struct queue_proc * queue, struct proc * p) {
	if(queue == 0 || p == 0)
		return;
	struct proc * tmp = queue->head;
	struct proc * ttmp;
	int i;
	if(tmp->pid == p->pid) {
		ttmp = tmp;
		queue->head = queue->head->next;
		queue->tail->next = queue->head;
	}else {
		for(i = 0; i < queue->num_proc-1; i++) {
			if(tmp->next->pid == p->pid)
				break;
			tmp = tmp->next;
		}
		if(i == queue->num_proc)
			return;
		ttmp = tmp->next;
		tmp->next = tmp->next->next;
		if(ttmp->pid == queue->tail->pid) {
			queue->tail = tmp;
		}
	}
	if(ttmp->state == RUNNABLE || ttmp->state == RUNNING)
		queue->num_runnable--;
	queue->num_proc--;
	if(!queue->num_proc)
		queue->state = DELETABLE;
	else if(!queue->num_runnable)
		queue->state = ALL_SLEEPING;
}

int
insert(struct proc * p) {
#ifdef MLFQ_SCHED
	insert_proc(&pqueue.queue_arr[p->level], p);
	return 0;
#else
	struct queue_proc *q;
	if(!(q = find_q(p->priority))) {
		q = q_init(p->priority);
		insert_queue(q);
	}
	insert_proc(q, p);
	return 0;
#endif
}

int
delete(struct proc * p) {
#ifdef MLFQ_SCHED
	delete_proc(&pqueue.queue_arr[p->level], p);
	return 0;
#else
	struct queue_proc *q;
	if(!(q = find_q(p->priority))) {
		return 1;
	}
	delete_proc(q, p);
	if(q->state == DELETABLE) {
		delete_queue(q);
	}
	return 0;
#endif
}

int 
getlev(void) {
	return myproc()->level;
}

void
boost(void) {
	struct proc * p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		switch(p->state) {
		case UNUSED: case EMBRYO: case ZOMBIE:
			break;
		case SLEEPING: case RUNNABLE: case RUNNING:
			delete(p);
			p->level = 0;
			p->ticks = 0;
			insert(p);
			break;
		default:
			break;
		}
	}
}

int 
setpriority(int pid, int n) {
#ifdef PRIORITY_SCHED
	struct proc * p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->pid == pid)
			break;

	delete(p);
	p->priority = n;
	insert(p);
	return p->priority;
#endif
	//cprintf("setpriority system call is only used by priority scheduling policy\n");
	return 0;
}

int
pick_now(void) {
	struct queue_proc *q = pqueue.tail;
	if(!pqueue.now_q) {
		pqueue.now_q = q;
		return 1;
	}
	for(int i = 0; i < pqueue.num_q; i++) {
		if(!q->num_runnable) {
			q = q->prev;
			continue;
		}
		else if(q->priority == pqueue.now_q->priority) {
			return 0;
		}
		else {
			pqueue.now_q = q;
			return 1;
		}
	}
	return 1;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
	pqueue.num_q = 0;
#ifdef MLFQ_SCHED
	int i;
	for(i = 0; i < 3; i++) {
		q_init(3 - i);
		insert_queue(&pqueue.queue_arr[i]);
	}
#endif
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
	p->priority = 0;
	p->level = 0;
	p->ticks = 0;
	
  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
	insert(p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);
	np->priority = 0;
	np->level = 0;
	np->ticks = 0;

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
	insert(np);

  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
	delete(curproc);
	curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

#ifdef MLFQ_SCHED
	struct proc* tmp;
	int ticks = 0;
#endif

  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PRIORITY_SCHED
	pick_now();
#elif MLFQ_SCHED
	pick_now();
#elif FCFS_SCHED
	pqueue.now_q = pqueue.head_q;
//#elif MLFQ_SCHED
#else
#endif
  
  for(;;){
    sti();

    acquire(&ptable.lock);
		
		p = pqueue.now_q->head;
#ifdef PRIORITY_SCHED
	  for(;;){
			if(pick_now()) {
				break;
			}
			if(p->priority != pqueue.now_q->priority)
				break;
      if(p->state != RUNNABLE){
				p = p->next;
				continue;
			}
#elif MLFQ_SCHED
		for(;;) {
			if(pick_now()) {
				break;
			}

			if(3 - p->level != pqueue.now_q->priority) {
				break;
				cprintf("ASDf\n");
			}
			if(p->state != RUNNABLE) {
				p = p->next;
				continue;
			}
#elif FCFS_SCHED
		for(;;) {
			if(!pqueue.now_q->num_runnable)
				break;
			if(p->state != RUNNABLE) {
				p = p->next;
				continue;
			}
//#elif MLFQ_SCHED
#else
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if(p->state != RUNNABLE) {
				continue;
			}
#endif
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();
				
      c->proc = 0;
#ifdef PRIORITY_SCHED
			p = p->next;
#elif FCFS_SCHED
			p = pqueue.now_q->head;
#elif MLFQ_SCHED
			int change = 0;
			p->ticks++;
			switch(p->level) {
			case 0:
				if(p->ticks == 2) {
					change = 1;
					p->ticks = 0;
				}
				break;
			case 1:
				if(p->ticks == 4) {
					change = 1;
					p->ticks = 0;
				}
				break;
			case 2:
				if(p->ticks == 8) {
					p = p->next;
					p->ticks = 0;
				}
			default:
				break;
			}
			if(change) {
				tmp = p;
				p = p->next;
				cprintf("%d\n", p->pid);
				delete(tmp);
				tmp->level++;
				insert(tmp);
			}
			ticks++;
			if(ticks == 100) {
				ticks = 0;
				boost();
			}
#endif
    }
    release(&ptable.lock);

  }
}

// Enter cprintf("%d\n", q->num_runnable);
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE; // process 상태를 runnable 이 된다고 함.
  sched(); // Schedule 이 여기서 일어남.
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

void
sleep(void * chan, struct spinlock *lk)
{
	struct proc *p = myproc();
	struct queue_proc *q;

	if(p == 0)
		panic("sleep");
	if(lk == 0)
		panic("sleep without lk");
	
	if(lk != &ptable.lock) {
		acquire(&ptable.lock);
		release(lk);
	}

	p->chan = chan;
	p->state = SLEEPING;
#ifdef MLFQ_SCHED
	q = find_q(3 - p->level);
#else
	q = find_q(p->priority);
#endif
	q->num_runnable--;
	if(!q->num_runnable)
		q->state = ALL_SLEEPING;
  
	sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
	struct queue_proc *q = pqueue.now_q;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){	
		if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
#ifdef MLFQ_SCHED
			q = find_q(3 - p->level);
#else
			q = find_q(p->priority);
#endif
			q->num_runnable++;
			q->state = RUNNABLE_Q;
		}
	}
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{ 
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
#ifdef MLFQ_SCHED
				struct queue_proc * q = find_q(3 - p->level);
#else
				struct queue_proc * q = find_q(p->priority);
#endif
				q->num_runnable++;
				q->state = RUNNABLE_Q;
			}
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
