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

  case T_PGFLT:
    //Lazy allocation!!!
    /////////////////////////////////////////////////////
    //  cprintf("pid %d %s: trap %d err %d on cpu %d "
    //        "eip 0x%x addr 0x%x--kill proc\n",
    //        myproc()->pid, myproc()->name, tf->trapno,
    //        tf->err, cpuid(), tf->eip, rcr2());
    /////////////////////////////////////////////////////
    if(myproc()->killed)
      break;
    if(rcr2() >= myproc()->sz) {
      cprintf("Invalid memory access\n");
      cprintf("%d %d 0x%x\n", myproc()->pid, myproc()->sz, rcr2());
      myproc()->killed = 1;
      break;
    }    
    if(myproc()->sz >= KERNBASE) {
      cprintf("Failed to memory allocation\n");
      myproc()->killed = 1;
      break;
    }
    /*
    if(myproc()->sz >= rcr2())
    {
      cprintf("Lazy allocation is needed\n");
      if((mem = kalloc()) == 0) {
        cprintf("Memory allocation is failed\n");
        myproc()->killed = 1;
        break;
      }

      memset(mem, 0, PGSIZE);
      if(mappages(myproc()->pgdir, (char *)PGROUNDDOWN(rcr2()), PGSIZE, V2P(mem), PTE_W|PTE_U) < 0) {
        cprintf("PTE recording is failed\n");
        myproc()->killed = 1;
        break;
      }
      lcr3(V2P(myproc()->pgdir));
      break;
    }
    */
    if(cow_handler(myproc()->pgdir, myproc()->sz, rcr2())) {
      //cprintf("pid %d %s: trap %d err %d on cpu %d "
      //      "eip 0x%x addr 0x%x--kill proc\n",
      //      myproc()->pid, myproc()->name, tf->trapno,
      //      tf->err, cpuid(), tf->eip, rcr2());
      cprintf("Page fault handler !\n");
      myproc()->killed = 1;
      return;
    }
    break;
    /* 
    tmp = ((pte_t*)P2V(PTE_ADDR(*(myproc()->pgdir))));//[PTX(a)];
    //cprintf("ASDF : 0x%x\n", tmp[PTX(rcr2())]); 
    if(rcr2() >= myproc()->sz) {
      cprintf("Invalid access 0x%x\n", V2P(rcr2()));
      cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
      return;
    }
    if(myproc()->sz >= KERNBASE) {
      cprintf("Lack of memory\n"); 
      cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
      
      myproc()->killed = 1;
      return;
    }
    
    a = PGROUNDDOWN(rcr2());
    //cprintf("0x%x\n", V2P(rcr2()));
    
    //cprintf("0x%x\n", ((pte_t*)P2V(PTE_ADDR(*(myproc()->pgdir))))[PDX(a)]);
    //cprintf("0x%x\n", tmp[PTX(a)]);
    if((tmp[PTX(a)] & 5) == 5) {
      //cprintf("CoW\n");
      //if(get_pg_owner(PTE_ADDR(tmp[PTX(a)])) == 0)
        tmp[PTX(a)] |= PTE_W;
      //else {
      //tmp[PTX(a)] |= PTE_W;
      //  down_pg_owner(PTE_ADDR(tmp[PTX(a)]));
        uint flags = PTE_FLAGS(tmp[PTX(a)]);
        mem = kalloc();
        memmove(mem, (char *)P2V(PTE_ADDR(tmp[PTX(a)])), PGSIZE);
        tmp[PTX(a)] = V2P(mem) | flags;
        //cprintf("0x%x\n", tmp[PTX(a)]);
      //}
      lcr3(V2P(myproc()->pgdir));
      return;
    }
    else if(!(tmp[PTX(a)] & 1)) {
    mem = kalloc();
    if(mem == 0) {
      cprintf("allocuvm out of memory\n");
      cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
      return;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(myproc()->pgdir, (char *)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0) {
      cprintf("allocuvm out of memory(2)\n");
      cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
      return;
    }
    lcr3(V2P(myproc()->pgdir)); 
   // myproc()->sz = newsz;
    switchuvm(myproc());
    
    return;
    }
    */
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
