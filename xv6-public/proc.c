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
  //To check mlfq processes
  int mproc;    
  //To check ordinary processes
  int procnum;
  //This is whole ticket number
  int ticket;
  //This variable check called run_MLFQ()
  int turn_m;
  //This means sum of cpu_share percentage
  int sumofcs;
  //This is to check number of scheduling
  int temp;
  //This is number of cpu_share processes
  int cpunum;
} ptable;

struct spinlock pgdirlock;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

//This function makes mlfq priority high
int boost(void){
    
    struct proc *p;
    int min = 2147483647;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == RUNNABLE && p->mlfq == 0) {
            if(p->stride_pass < min) min = p->stride_pass;
        }
        if(ptable.turn_m == 1 && p->mlfq == 1) {
            p->m_prior = 0;
            p->m_ticks = 1;
            p->m_check = 0;
        }
    }
    //Periodically reduce stride_pass with minimum stride_pass value
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == RUNNABLE && p->mlfq == 0) {
            p->stride_pass -= min;
        }
    }
    release(&ptable.lock);
    return 0;

}
int run_MLFQ(void){
    acquire(&ptable.lock);
    //This is initialization for running mlfq scheduling
    if(ptable.turn_m == 0){
        ptable.turn_m = 1;
        ptable.mproc = 0;
    }
    struct proc *p = myproc();
    //If current process have cpu percentage, exclude that state, and change to mlfq scheduling process
    if(p->cpu_share != 0){
        ptable.ticket+=p->cpu_share*100;
        p->cpu_share = 0;
        ptable.cpunum--;
    }
    //This is set up for mlfq process
    ptable.procnum--;
    p->mlfq = 1;
    p->m_ticks = 1;
    p->m_prior = 0;
    p->m_check = 0;
    ptable.mproc++;
    //Restride other processes
       for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
             if(p->state == RUNNABLE && ptable.procnum>0 &&p->cpu_share==0 &&p->mlfq==0) {
                 p->stride_value = ptable.ticket/((ptable.procnum)*100);
             }
       }
    
    release(&ptable.lock);

    return 0;
}
//This function return mlfq process priority
int getlev(void){
    acquire(&ptable.lock);
    int n = myproc()->m_prior;
    release(&ptable.lock);
    return n;
}

int cpu_share(int a){
    struct proc *p;
    int min = 2147483647;
    acquire(&ptable.lock);
    //If cpu_share percentage over 20%, just return
    if( a + ptable.sumofcs > 20) {
        cprintf("cpu_share proportion will be over 20%, you can't get more cpu\n");
        release(&ptable.lock);
        return 0;
    }
    //This is set up for applying cpu percentage
    else{
    ptable.procnum--;
    ptable.cpunum++;
    ptable.sumofcs += a;
    ptable.ticket -= a*100;
        //Restride other process
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
             if(p->stride_pass < min) min = p->stride_pass;
                if(p->state == RUNNABLE && ptable.procnum >0 && p->cpu_share==0 &&p->mlfq==0 ) {
                 p->stride_value = ptable.ticket/((ptable.procnum)*100);
             }   
        }
    p = myproc();
    //If current process is mlfq process, change to cpu_share process
    if(p->mlfq == 1) {
        p->mlfq = 0;
        p->m_prior = -1;
        ptable.mproc--;
    }
    p->cpu_share += a;
    p->stride_value = 10000/(a*100);
    p = myproc();
    p->stride_pass = min;
    }

  release(&ptable.lock);
    
    return 0;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
    //Set up for scheduling
    ptable.procnum = 0;
    ptable.ticket = 10000;
    ptable.sumofcs = ptable.turn_m = 0;
    ptable.temp = 0;
    ptable.mproc = ptable.cpunum = 0; 
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
  struct proc *f;
  char *sp;
  int tmp = 2147483647;

  acquire(&ptable.lock);
  //Count number of ordinary processes
  ptable.procnum++;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == UNUSED)
             goto found;

  release(&ptable.lock);
  return 0;

found:
  //Initiation for ordinary process
  p->tcnt = p->tid = p->tc = 0;
  p->killed = 0;
  p->tparent = (struct proc*)0;
  p->stride_pass = 0;
  p->cpu_share = 0;
  p->mlfq = 0;
  p->m_prior = p->m_check = p->m_ticks = -1;
  for(int i=0;i<NPROC;i++){
      p->tret[i] = 0;
      p->thread[i] = 0;
      p->tsz[i] = 0;
  }
    //Find minimum stride_value
    for(f = ptable.proc; f < &ptable.proc[NPROC]; f++){
        if(f->state == RUNNABLE && ptable.procnum>0 && f->cpu_share == 0 && f->mlfq == 0) {
             f->stride_value = ptable.ticket/((ptable.procnum)*100);
            }
    if(f->state == RUNNABLE && f->stride_pass < tmp) tmp = f->stride_pass;
    }
  if(tmp != 2147483647) p->stride_pass = tmp;
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

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

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

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  int i;
  struct proc *curproc = myproc();
  struct proc *p;
  struct proc *cap;
  if(myproc()->tparent !=0){
      cap = myproc()->tparent;
  }
  else cap = myproc();
  int fd;

  if(curproc == initproc)
    panic("init exiting");
  // Close all open files.
   
  for(i = 0;i <= NPROC; i++){
      if(i == NPROC){
          curproc = cap;
          goto texit;
      }
      if(cap->thread[i] == 0) continue;
      else { curproc = cap->thread[i];}
 

texit:
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }
  
  if(curproc->cwd){
  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  }
}
  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(cap->parent);
  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc && p->tparent == 0){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
    
  }

  // Jump into the scheduler, never to return.
 
  //This is handling cpu_share processes, mlfq processes, ordinary processes which is exited
    if(curproc->cpu_share != 0) { 
          ptable.sumofcs -= curproc->cpu_share;
          ptable.ticket += curproc->cpu_share*100;
          ptable.cpunum--;
          curproc->cpu_share = 0;
    }
    else if(curproc->mlfq == 0 && curproc->cpu_share == 0){
        ptable.procnum--;
    }
    else if( curproc->mlfq == 1){
        ptable.mproc--;
        curproc->mlfq = 0;
        curproc->m_ticks = curproc->m_prior = curproc ->m_check = -1;
        if(ptable.mproc == 0){ 
            ptable.turn_m = 0;
            ptable.temp = 0;
        }
    }
    //If Comeback shell process, initiation ptable variables
    if(ptable.procnum == 2 && ptable.cpunum == 0 && ptable.mproc == 0 ) {
        ptable.ticket = 10000;
        ptable.sumofcs = ptable.turn_m = ptable.temp = 0;
    }
    curproc->stride_pass = curproc->stride_value = 0;
  //Restride other processes
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE && p->cpu_share == 0 && p->mlfq == 0){
        if(ptable.procnum>0) p->stride_value = ptable.ticket/((ptable.procnum)*100);
    }
  }
  for(i = 0; i < NPROC; i++){
      if(cap->thread[i] != 0){
          cap->thread[i]->state = ZOMBIE;
      }
  }
  cap->state = ZOMBIE;

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  int i;
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc || p->tparent!=0)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        for(i = 0; i<NPROC; i++){
            if(p->thread[i] !=0){
                kfree(p->thread[i]->kstack);
                p->thread[i]->kstack = 0;
                p->thread[i]->killed = 0;
                p->thread[i]->pid =0;
                p->thread[i]->parent = 0;
                p->thread[i]->name[0]= 0;
                p->thread[i]->state = UNUSED;
                p->thread[i] = 0;
            }
        }
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->m_ticks = -1;
        p->m_check = -1;
        p->mlfq = p->stride_pass = p->stride_value = p->cpu_share = 0;
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
  struct proc *m;
  struct cpu *c = mycpu();
  int min = 0;
  c->proc = 0;
//basic stirde scheduling
  for(;;){
    // Enable interrupts on this processor.
    sti();
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    min = 2147483647;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE && p->mlfq == 1)
        continue;

      //find process has minimum stride pass value
   
      if(p->stride_pass < min  && p->state == RUNNABLE){
          min = p->stride_pass;
          m = p;
      }   
      if(p->tc == 1  && p->state == RUNNABLE && p->tparent != 0 ) {
          m = p; 
              
      }
    }
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = m;
      switchuvm(m);
      m->state = RUNNING;
      m->stride_pass += m->stride_value;
      swtch(&(c->scheduler), m->context);
      switchkvm();
      //after calling run_MLFQ(), check ptable.temp to guarantee that cpu 20%
      if(ptable.turn_m == 1) ptable.temp++;
      if(p->tc == 1 && p->tparent == 0 && p->state == RUNNABLE) p->state = ZOMBIE;
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
 
      release(&ptable.lock);
//mlfq scheduling
    acquire(&ptable.lock);  
    if(ptable.temp == 4){
        ptable.temp = 0;
             if(ptable.mproc !=0){
             //In regular sequence, changing next process with checking priority
             for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                if(p->state != RUNNABLE && p->mlfq == 0)
                    continue;

                if(p->m_prior == 2 && p->mlfq == 1){
                    m = p;
                    if((p->m_ticks > p->m_check) && p->m_check !=0) goto found;
                }
             }
             for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                if(p->state != RUNNABLE && p->mlfq == 0)
                    continue;

                if(p->m_prior == 1 && p->mlfq == 1){
                    m = p;
                    if((p->m_ticks > p->m_check)&& p->m_check !=0) goto found;
                }
             }
             for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                if(p->state != RUNNABLE && p->mlfq == 0)
                    continue;
                if(p->m_prior == 0 && p->mlfq == 1){
                    m = p;
                }
             }

            found:      
             //Context-Switching to mlfq processes
                c->proc = m;
                switchuvm(m);
                m->state = RUNNING;
                
                swtch(&(c->scheduler), m->context);
                switchkvm();
                c->proc = 0;
                m->m_check++;

                if(m->m_prior == 0){
                m->m_prior = 1;
                m->m_ticks = 2;
                m->m_check = 0;
                }
                else if(m->m_prior == 1 && (m->m_ticks == m->m_check)){
                m->m_prior = 2;
                m->m_ticks = 4;
                m->m_check = 0;
                }
           }

        }
    release(&ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
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
  myproc()->state = RUNNABLE;
  sched();
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

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

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

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
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
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
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

//This function works for creating thread 
int thread_create(thread_t * thread, void *(*start_routine)(void *), void *arg){
  int i;
  struct proc *np;
  struct proc *curproc = myproc();
  uint sp , ustack[3];
  struct proc *tp;
  if(curproc->tparent !=0) tp = myproc()->tparent;
  else tp = myproc();
  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  acquire(&ptable.lock);
  // Share page directory
  np->tc = 1;
  np->parent = tp->parent;
  np->tparent = tp;
  np->pgdir = tp->pgdir;
  for(i=0;i<NPROC;i++){
      if(tp->thread[i] == 0)
          break;
  }
  if(i == NPROC) return -1;
  tp->thread[i] = np;
  np->tid = i;
  tp-> tcnt++;
  // Copy process state from proc.
  // Clear %eax so that fork returns 0 in the child.

  release(&ptable.lock);
  acquire(&pgdirlock); 
  for(i =0; i<NPROC; i++){
      if(tp->tsz[i] != 0){
          if((np->sz = allocuvm(tp->pgdir,tp->tsz[i], tp->tsz[i] + 2*PGSIZE))== 0){
              release(&pgdirlock);
              return -1;
          }
         tp -> tsz[i] =0;
          goto done;
      }
  }
  if((np->sz = allocuvm(tp->pgdir, tp->sz, tp->sz + 2*PGSIZE)) == 0){
      release(&pgdirlock);
      return -1;
  }
  tp->sz = np->sz;
done:

  clearpteu(tp->pgdir, (char*)(np->sz - 2*PGSIZE));
  release(&pgdirlock);
 
  
  //This part is similar fork() 
  for(i = 0; i < NOFILE; i++)
    if(tp->ofile[i])
      np->ofile[i] = filedup(tp->ofile[i]);
  np->cwd = idup(tp->cwd);

  safestrcpy(np->name, tp->name, sizeof(tp->name));
  
  *np->tf = *tp->tf; 
  np->tf->eip = (uint)start_routine;
  sp = np->sz; 
  //Modify stack pointer, suppose *arg holds that one argument  
  ustack[0] = 0xffffffff;
  ustack[1] = (uint)arg;
  ustack[2] = 0;
  sp -= sizeof(ustack);

  if(copyout(tp->pgdir, sp, ustack, sizeof(ustack)) < 0)
      return -1;
  // To do start_routine func, modify eip
  np->tf->esp = sp;
  *thread = np;
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  return 0;
}
//This function works for exiting thread
void thread_exit(void *retval){
    struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc->tparent == 0) {
      exit();
  }
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
  wakeup1(curproc->tparent);
  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->tparent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
    
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  curproc->tparent->tret[curproc->tid] = retval;
  curproc->tparent->tcnt--;
  sched();
  cprintf("%d\n%d\n",p->tid,p->tparent->tcnt);
  panic("zombie thread exit");

}
int thread_join(thread_t thread, void **retval){
    int i;
    struct proc *tjoin = thread;
    if(thread->state == UNUSED){ 
        return 0;
    }
    int check = 0;
    acquire(&ptable.lock);
    while(tjoin->state != ZOMBIE)
        sleep(thread->tparent, &ptable.lock);
    acquire(&pgdirlock);
    kfree(tjoin->kstack);
    for(i = 0;i<NPROC;i++){
        if(tjoin->tparent->tsz[i] == 0){
          if((tjoin->tparent->tsz[i] = deallocuvm(tjoin->pgdir, tjoin->sz, tjoin->sz - 2*PGSIZE))==0){
                release(&pgdirlock);
                return -1;
            }
            else{
                check = 1;
                break;
            }
        }
    }
    release(&pgdirlock);       
    if(check == 0) {
        release(&ptable.lock);
        return -1;
    }
    
    if(retval != 0)
      *retval = thread->tparent->tret[tjoin->tid];
    
    thread->tparent->thread[thread->tid] = 0;
    thread->state = UNUSED;
    thread->kstack = 0;
    thread->name[0] = 0;
    thread->killed = 0;
    
    release(&ptable.lock);

 return 0;
}

