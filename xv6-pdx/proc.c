#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "uproc.h"

#ifdef CS333_P3P4
struct StateLists
{
	struct proc* ready[MAX+1];
	struct proc* free;
	struct proc* sleep;
	struct proc* zombie;
	struct proc* running;
	struct proc* embryo;
};
#endif

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  #ifdef CS333_P3P4
  struct StateLists pLists;
  uint PromoteAtTime;
  #endif
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
#ifdef CS333_P3P4
static void addhead(struct proc** head, struct proc* toadd)
{
	toadd->next = *head;
	*head = toadd;
} 

static void addtail(struct proc** head, struct proc* toadd)
{
	if(!(*head))
	{
		*head = toadd;
		(*head)->next = 0;
		return;
	}
	struct proc* current = (*head);
	while(current->next)
	{
		current = current->next;
	}
	current->next = toadd;
	current->next->next = 0;
	return;
}

static int remove(struct proc** head, struct proc* remove)
{
	if(!(*head))
	{
		return -1;
	}
	if((*head) == remove)
	{
		struct proc* temp = *head;
		*head = (*head)->next;
		temp->next = 0;
		return 0;
	}
	struct proc* current = *head;
	struct proc* previous = 0;
	while(current && (current != remove))
	{
		previous = current;
		current = current->next;
	}
	if(!current)
	{
		return -1;
	}
	previous->next = current->next;
	current->next = 0;
	return 0;
}

static void assert(struct proc* p, enum procstate state)
{
	if (p->state != state)
	{
		panic("Error, invalid state removal");
	}
}
#endif
void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

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
  #ifndef CS333_P3P4
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  #else
  if(ptable.pLists.free)
  {
	p = ptable.pLists.free;
	goto found;
  }
  #endif
  release(&ptable.lock);
  return 0;

found:
  #ifdef CS333_P3P4
  assert(p, UNUSED);
  if(remove(&ptable.pLists.free, p))
  {
	panic("Error removing from free list");
  }
  p->state = EMBRYO;
  addhead(&ptable.pLists.embryo, p);
  #else
  p->state = EMBRYO;
  #endif
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    #ifdef CS333_P3P4
    acquire(&ptable.lock);
    assert(p, EMBRYO); 
    if(remove(&ptable.pLists.embryo, p))
    {
	panic("Error removing from embryo list");
    }
    p->state = UNUSED;
    addhead(&ptable.pLists.free, p);
    release(&ptable.lock);
    #else
    p->state = UNUSED;
    #endif
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
  #ifdef CS333_P1
  p->start_ticks = ticks;
  #endif
  #ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
  #endif
  #ifdef CS333_P3P4
  p->budget = BUDGET;
  p->prio = 0;
  #endif
  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  #ifdef CS333_P3P4
  struct proc* current;
  acquire(&ptable.lock);
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
  for (int i = 0; i < (MAX + 1); ++i)
  {
	ptable.pLists.ready[i] = 0;
  }
  ptable.pLists.free = 0;
  for(current = ptable.proc; current < &ptable.proc[NPROC]; ++current)
  {
	addtail(&ptable.pLists.free, current);
  }
  ptable.pLists.sleep = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.running = 0;
  ptable.pLists.embryo = 0;
  release(&ptable.lock);
  #endif 
  p = allocproc();
  initproc = p;
  #ifdef CS333_P2
  initproc->uid = DEFUID;
  initproc->gid = DEFGID;
  #endif
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
  #ifdef CS333_P3P4
  acquire(&ptable.lock);
  assert(p, EMBRYO);
  if(remove(&ptable.pLists.embryo, p))
  {
	panic("Error removing from embryo list");
  }
  p->state = RUNNABLE;
  ptable.pLists.ready[0] = p;
  ptable.pLists.ready[0]->next = 0;
  release(&ptable.lock);
  #else
  p->state = RUNNABLE;
  #endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
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

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    #ifdef CS333_P3P4
    acquire(&ptable.lock);
    assert(np, EMBRYO);
    if(remove(&ptable.pLists.embryo, np))
    {
	panic("Error removing from embryo list");
    }
    np->state = UNUSED;
    addhead(&ptable.pLists.free, np);
    release(&ptable.lock);
    #else
    np->state = UNUSED;
    #endif
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  #ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
  #endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  #ifdef CS333_P3P4
  assert(np, EMBRYO);
  if(remove(&ptable.pLists.embryo, np))
  {
	panic("Error removing from embryo list");
  }
  np->state = RUNNABLE;
  addtail(&ptable.pLists.ready[0], np);
  #else
  np->state = RUNNABLE;
  #endif
  release(&ptable.lock);
  
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.

  for (int i = 0; i < MAX+1; ++i)
  {
	  p = ptable.pLists.ready[i];
	  while(p)
	  {
		if(p->parent == proc)
		{
			p->parent = initproc;
		}
		p = p->next;
	  }	
  }
  p = ptable.pLists.sleep;
  while(p)
  {
	if(p->parent == proc)
	{
		p->parent = initproc;
	}
	p = p->next;
  }	
  p = ptable.pLists.zombie;
  while(p)
  {
	if(p->parent == proc)
	{
		p->parent = initproc;
		wakeup1(initproc);
	}
	p = p->next;
  }	
  p = ptable.pLists.running;
  while(p)
  {
	if(p->parent == proc)
	{
		p->parent = initproc;
	}
	p = p->next;
  }	
  p = ptable.pLists.embryo;
  while(p)
  {
	if(p->parent == proc)
	{
		p->parent = initproc;
	}
	p = p->next;
  }	

  // Jump into the scheduler, never to return.
  assert(proc, RUNNING);
  if(remove(&ptable.pLists.running, proc))
  {
	panic("Error removing from running list");
  }
  proc->state = ZOMBIE;
  addhead(&ptable.pLists.zombie, proc);
  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;

    p = ptable.pLists.zombie;
    while(p)
    {
	if(p->parent == proc)
	{
	      	havekids = 1;
		// Found one.
		pid = p->pid;
		kfree(p->kstack);
		p->kstack = 0;
		freevm(p->pgdir);
		assert(p, ZOMBIE);
		if(remove(&ptable.pLists.zombie, p))
		{
			panic("Error removing from zombie list");
		}
		p->state = UNUSED;
		addhead(&ptable.pLists.free, p);
		p->pid = 0;
		p->parent = 0;
		p->name[0] = 0;
		p->killed = 0;
		release(&ptable.lock);
		return pid;
      		
	}
	p = p->next;
    }

    int flag = 0;
    p = ptable.pLists.sleep;
    while(p && !flag)
    {
	if(p->parent == proc)
	{
		havekids = 1;
		flag = 1;
	}
	p = p->next;
    }
    for(int i = 0; i < MAX + 1 && !flag; ++i)
    {
	    p = ptable.pLists.ready[i];
	    while(p && !flag)
	    {
		if(p->parent == proc)
		{
			havekids = 1;
			flag = 1;
		}
		p = p->next;
	    }
    }
    p = ptable.pLists.running;
    while(p && !flag)
    {
	if(p->parent == proc)
	{
		havekids = 1;
		flag = 1;
	}
	p = p->next;
    }
    p = ptable.pLists.embryo;
    while(p && !flag)
    {
	if(p->parent == proc)
	{
		havekids = 1;
		flag = 1;
	}
	p = p->next;
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
 } 
}
#endif

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      #ifdef CS333_P2
      p->cpu_ticks_in = ticks;
      #endif
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    acquire(&ptable.lock);

    if (ptable.PromoteAtTime <= ticks)
    {
	struct proc* current;
	current = ptable.pLists.sleep;
	while(current)
	{
		if(current->prio > 0)
		{
			--(current->prio);
		}
		current = current->next;
	}
	current = ptable.pLists.running;
	while(current)
	{
		if(current->prio > 0)
		{
			--(current->prio);
		}
		current = current->next;
	}
	for(int i = 1; i < MAX+1; ++i)
	{
		current = ptable.pLists.ready[i];
		while(current)
		{
			struct proc* connect = current->next;
			assert(current, RUNNABLE);
			if(remove(&ptable.pLists.ready[i], current))
			{
				panic("ERROR removing");
			}
			--(current->prio);
			addtail(&ptable.pLists.ready[current->prio], current);
			current = connect;
		}
	}	

	ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
    }
    for (int i = 0; i < MAX+1; ++i)
    {
	    if(ptable.pLists.ready[i])
	    {
	      p = ptable.pLists.ready[i];
	      // Switch to chosen process.  It is the process's job
	      // to release ptable.lock and then reacquire it
	      // before jumping back to us.
	      idle = 0;  // not idle this timeslice
	      proc = p;
	      switchuvm(p);
	      assert(p, RUNNABLE);
	      if(remove(&ptable.pLists.ready[i], p))
	      {
		panic("Error removing from ready list");
	      }
	      p->state = RUNNING;
	      addhead(&ptable.pLists.running, p);
	      #ifdef CS333_P2
	      p->cpu_ticks_in = ticks;
	      #endif
	      swtch(&cpu->scheduler, proc->context);
	      switchkvm();

	      // Process is done running for now.
	      // It should have changed its p->state before coming back.
	      proc = 0;
	      i = MAX+1;
	    }
    } 
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
#ifndef CS333_P3P4
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  #ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
  #endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
#else
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  #ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
  #endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
#endif

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  #ifdef CS333_P3P4
  assert(proc, RUNNING);
  if(remove(&ptable.pLists.running, proc))
  {
	panic("Error removing from running list");
  }
  proc->state = RUNNABLE;
  proc->budget = proc->budget - (ticks - proc->cpu_ticks_in);

  if(proc->budget <= 0)
  {
	if(proc->prio != MAX)
	{
		++(proc->prio);
	}
	proc->budget = BUDGET;
  }

  addtail(&ptable.pLists.ready[proc->prio], proc);
  #else
  proc->state = RUNNABLE;
  #endif
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
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  #ifdef CS333_P3P4
  assert(proc, RUNNING);
  if(remove(&ptable.pLists.running, proc))
  {
	panic("Error removing from running list");
  }
  proc->state = SLEEPING;
  proc->budget = proc->budget - (ticks - proc->cpu_ticks_in);
  if(proc->budget <= 0)
  {
	if(proc->prio != MAX)
	{
		++(proc->prio);
	}
	proc->budget = BUDGET;
  }

  addhead(&ptable.pLists.sleep, proc);
  #else
  proc->state = SLEEPING;
  #endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){ 
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

#ifndef CS333_P3P4
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
#else
static void
wakeup1(void *chan)
{
  struct proc* current;
  struct proc* temp;

  current = ptable.pLists.sleep;
  while(current)
  {
	temp = current->next;
	if(current->chan == chan)
	{
		assert(current, SLEEPING);
		if(remove(&ptable.pLists.sleep, current))
		{
			panic("Error removing from sleep list");
		}
		current->state = RUNNABLE;
		addtail(&ptable.pLists.ready[current->prio], current);
	}
	current = temp;
  }

}
#endif

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
#ifndef CS333_P3P4
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
#else
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);

  p = ptable.pLists.sleep;
  while(p)
  {
	if(p->pid == pid)
	{
		p->killed = 1;
		assert(p, SLEEPING);
		if(remove(&ptable.pLists.sleep, p))
		{
			panic("Error removing from sleep list");
		}
		p->state = RUNNABLE;
		addtail(&ptable.pLists.ready[p->prio], p);
		release(&ptable.lock);
		return 0;
	}
	p = p->next;
  }

  p = ptable.pLists.running;
  while(p)
  {
	if(p->pid == pid)
	{
		p->killed = 1;
	
		release(&ptable.lock);
		return 0;
	}
	p = p->next;
  }
  p = ptable.pLists.embryo;
  while(p)
  {
	if(p->pid == pid)
	{
		p->killed = 1;
	
		release(&ptable.lock);
		return 0;
	}
	p = p->next;
  }
  for(int i = 0; i < MAX+1; ++i)
  {
	  p = ptable.pLists.ready[i];
	  while(p)
	  {
		if(p->pid == pid)
		{
			p->killed = 1;
			release(&ptable.lock);
			return 0;
		}
		p = p->next;
	  }
  }
  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  #ifdef CS333_P1
	cprintf("\n");
	cprintf("PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\tPCs\n");
  #endif
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    #ifdef CS333_P2
    int ppid;
    if(p->pid == 1)
    {
	ppid = 1;
    }
    else
    {
	ppid = p->parent->pid;
    }
    cprintf("%d\t%s\t%d\t%d\t%d\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, p->prio);
    #else
    cprintf("%d\t%s\t%s\t", p->pid, state, p->name);
    #endif

    #ifdef CS333_P1
    uint diff = (ticks - (p->start_ticks));
    uint whole = diff/1000;
    uint ten = (diff - (whole * 1000))/100;
    uint hundred = (diff - (whole * 1000) - (ten * 100))/10;
    uint thousand = diff - (whole * 1000) - (ten * 100) - (hundred * 10);
    cprintf("%d.%d%d%d\t", whole, ten, hundred, thousand);
    #endif

    #ifdef CS333_P2
    diff = p->cpu_ticks_total;
    whole = diff/1000;
    ten = (diff - (whole * 1000))/100;
    hundred = (diff - (whole * 1000) - (ten * 100))/10;
    thousand = diff - (whole * 1000) - (ten * 100) - (hundred * 10);
    cprintf("%d.%d%d%d\t%s\t%d\t", whole, ten, hundred, thousand, state, p->sz);
    #endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef CS333_P2
int
getprocs(int max, struct uproc* table)
{
	int i;
	struct proc* p;	
        
        acquire(&ptable.lock);
 
	for(i = 0, p = ptable.proc; p < &ptable.proc[NPROC] && i < max; p++)
	{
		if(p->state != UNUSED && p->state != EMBRYO)
		{
			table[i].pid = p->pid;
			table[i].uid = p->uid;
			table[i].gid = p->gid;
			if(p->pid == 1)
			{
				table[i].ppid = 1;
			}
			else
			{
				table[i].ppid = p->parent->pid;
			}
			table[i].elapsed_ticks = (ticks - (p->start_ticks));
			table[i].CPU_total_ticks = p->cpu_ticks_total;
			table[i].size = p->sz;
			table[i].priority = p->prio;
			safestrcpy(table[i].name, p->name, sizeof(table[i].name)); 
			if (p->state == RUNNABLE)
			{
				safestrcpy(table[i].state, "RUNNABLE", sizeof(table[i].state));
			} 
			else if (p->state == RUNNING)
			{	
				safestrcpy(table[i].state, "RUNNING", sizeof(table[i].state));
			} 
			else if (p->state == SLEEPING)
			{ 
				safestrcpy(table[i].state, "SLEEPING", sizeof(table[i].state));
			} 
			else if (p->state == ZOMBIE)
			{
				safestrcpy(table[i].state, "ZOMBIE", sizeof(table[i].state));
			} 

			i++;
		}
	}
 	release(&ptable.lock);

	return i;
} 
#endif

#ifdef CS333_P3P4

void
ctrlr(void)
{

	acquire(&ptable.lock);
	struct proc* current;

	cprintf("Ready List Processes:\n");
	for(int i = 0; i < MAX+1; ++i)
	{
		current = ptable.pLists.ready[i];
		cprintf("%d: ", i);
		if(!current)
		{
			cprintf("Empty\n\n");
		}
		else 
		{
			while(current->next)
			{
				cprintf("(%d, %d) -> ", current->pid, current->budget);
				current = current->next;
			}
			if(current)
			{
				cprintf("(%d, %d)\n\n", current->pid, current->budget);
			}
		}
	}
	release(&ptable.lock);
}

void
ctrlf(void)
{
	acquire(&ptable.lock);
	struct proc* current;
	current = ptable.pLists.free;
	int count = 0;
	
	if(current)
	{
		while(current)
		{
			++count;
			current = current->next;
		}
	}
	cprintf("Free List Size: %d processes\n", count);
	release(&ptable.lock);
}

void 
ctrls(void)
{
	acquire(&ptable.lock);
	struct proc* current;
	current = ptable.pLists.sleep;

	cprintf("Sleep List Processes:\n");
	if(!current)
	{
		cprintf("Empty\n");
		release(&ptable.lock);
		return;
	}
	while(current->next)
	{
		cprintf("%d -> ", current->pid);
		current = current->next;
	}
	if(current)
	{
		cprintf("%d\n", current->pid);
	}
	release(&ptable.lock);
}
void
ctrlz(void)
{
	acquire(&ptable.lock);
	struct proc* current;
	current = ptable.pLists.zombie;
	int ppid;

	cprintf("Zombie List Processes:\n");
	if(!current)
	{
		cprintf("Empty\n");
		release(&ptable.lock);
		return;
	}
	while(current->next)
	{
		if(current->pid == 1)
		{
			ppid = 1;
		}
		else
		{
			ppid = current->parent->pid;
		}
		cprintf("(%d, %d) -> ", current->pid, ppid);
		current = current->next;
	}
	if(current)
	{
		if(current->pid == 1)
		{
			ppid = 1;
		}
		else
		{
			ppid = current->parent->pid;
		}
		cprintf("(%d, %d)\n", current->pid, ppid);
	}
	release(&ptable.lock);
}

int
setpriority(int pid, int priority)
{
	if(priority > MAX || priority < 0 || pid < 0)
	{
		return -1;
	}
	acquire(&ptable.lock);
	struct proc* current;
	int flag = 0;
	current = ptable.pLists.sleep;
	while(current && !flag)
	{
		if(current->pid == pid)
		{
			current->prio = priority;
			current->budget = BUDGET;
			flag = 1;
		}
		current = current->next;
	}
	current = ptable.pLists.running;
	while(current && !flag)
	{
		if(current->pid == pid)
		{
			current->prio = priority;
			current->budget = BUDGET;
			flag = 1;
		}
		current = current->next;
	}
	for(int i = 0; i < MAX+1 && !flag; ++i)
	{
		current = ptable.pLists.ready[i];
		while(current && !flag)
		{
			if(current->pid == pid)
			{
				if(current->prio == priority)
				{
					current->budget = BUDGET;
				}
				else
				{
					assert(current,RUNNABLE);
					remove(&ptable.pLists.ready[current->prio], current);
					current->prio = priority;
					addtail(&ptable.pLists.ready[current->prio], current);
					current->budget = BUDGET;
				}
				flag = 1;
			}
			current = current->next;
		}
	}
	release(&ptable.lock);
	if(!flag)
	{
		return -1;
	}
	return 0;	
}		
#endif
