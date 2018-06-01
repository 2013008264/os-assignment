#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers. All same.
struct spinlock tickslock;
uint ticks;
uint curticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++) //SETGATE is macro.
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  SETGATE(idt[T_USER_SYSCALL], 1, SEG_KCODE<<3, vectors[T_USER_SYSCALL], DPL_USER);
  

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void//여기가 진짜로, interrupt 의 분기.
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){ // Sleep 이 일어났을 때.
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD://Keyboard interrupt.
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  // USER interrupt
  case T_USER_SYSCALL:
    cprintf("user interrupt 128 called!\n");
    break;
  /*  IF PAGE FAULT...  */
  case T_PGFLT:
    /* If actual inavlid access. */
    if(rcr2() >= myproc()->sz || myproc()->sz >= KERNBASE)
      goto bad; 

    /* Jumping to page fault handler  */
    if(pfault_handler(myproc()->pgdir, myproc()->sz, rcr2())) 
      goto bad;
    break;

  bad:
    cprintf("Page fault in pid %d, process size : %d, address : 0x%x\n",
        myproc()->pid, myproc()->sz, rcr2());
    myproc()->killed = 1;
    break;
  //PAGEBREAK: 13
  break;
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake. Kernel 이 잘못되면, panic 으로 멈춰버림.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved. 
  // Example 이거는 user process. 가 잘못했을 경우. exit()으로 안 끝내고, return 으로 끝내면 처리가 안댐.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
#ifdef MLFQ_SCHED
  curticks++;
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER){
    int tmp = (myproc()->ticks)++;
    if(pick_now())
      yield();
    switch(myproc()->level) {
    case 0:
      if(tmp >= 1) {
        levelup(myproc());
        yield();
      }
      break;
    case 1:
      if(tmp >= 3) {
        levelup(myproc());
        yield();
      }
      break;
    case 2:
      if(tmp >= 7)
        yield();
      break;
    }
  }
  if(curticks >= 100) {
    curticks = 0;
    boost();
    if(myproc())
      yield();
  }
#else
  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  // For interrupt timer interrupt.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();
#endif

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
