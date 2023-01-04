#include <proc_syscall.h>
#include <proc.h>
#include <addrspace.h>
#include <syscall.h>
#include <kern/errno.h>
#include <synch.h>
#include <current.h>
#include <thread.h>
#include <copyinout.h>
#include <kern/wait.h>
#include <vfs.h>
#include <copyinout.h>
#include <kern/fcntl.h>
#include <vnode.h>
#include <vm.h>
#include <lib.h>


int sys_getpid(pid_t *curproc_pid) {
	*curproc_pid = curproc->proc_id;
	return 0;
}

static void op_efp(void *tfv, unsigned long data2){
  (void) data2;
  struct trapframe *tf = (struct trapframe *)tfv;
  enter_forked_process(tf);
}

int sys_fork(pid_t *child_pid, struct trapframe *tf){

  struct trapframe *tf_child; //Stack of the exception handled
  struct proc *child_proc;
  int err;
  char *name;

  if (proc_counter >= PID_MAX){
    return ENPROC; //There are too many process on the system.
  }
  name = curproc->p_name;
  child_proc = proc_create_runprogram(name);

  if(child_proc == NULL){
    return ENOMEM; //Sufficient virtual memory for the new process was not available.
  }



  /* Search the first free space in proc_table */

    if(proc_counter == MAX_PROC -1){
      return ENPROC; //There are already too many process on the system
    }

  proc_table[proc_counter] = child_proc;

  /*Copy the address space of the parent in a child process */
  err = as_copy(curproc->p_addrspace, &child_proc->p_addrspace);

  if(err){
    return err;
  }

  child_proc->parent_id = curproc->proc_id;

  /*Since the parent and child have the same file table: */

  for(int i = 0; i < OPEN_MAX; i++){
    if(curproc->file_table[i] != NULL){
      lock_acquire(curproc->file_table[i]->lock);//Lock the filetablei[i]
      child_proc->file_table[i] = curproc->file_table[i];
      lock_release(curproc->file_table[i]->lock);//Release the lock at filetable[i]
    }
  }

  spinlock_acquire(&curproc->p_lock);
      if (curproc->p_cwd != NULL) {
            VOP_INCREF(curproc->p_cwd);
            child_proc->p_cwd = curproc->p_cwd;
      }
  spinlock_release(&curproc->p_lock);

  /*Creation of memory space of trapframe */
  tf_child = (struct trapframe *)kmalloc(sizeof(struct trapframe));
  if(tf_child == NULL){
    return ENOMEM; //Sufficient virtual memory for the new process was not available.
  }

  /* Copy the trapframe into trapframe_child */
  //*tf_child = *tf;
  memcpy(tf_child, tf, sizeof(struct trapframe));

  err = thread_fork(curthread->t_name, child_proc,op_efp,(void *)tf_child, (unsigned long)0);
  /*If thread_fork fails: destroy the process and free the trapframe memory*/
  if(err){
    proc_destroy(child_proc);
    kfree(tf_child);
    return err;
  }

/*Set the return value as a child_PID */
  *child_pid = child_proc->proc_id;
  return 0;
}

void sys_exit(int exitcode){
	int i = 0;

	for(i = 0; i < MAX_PROC; i++){
		if(proc_table[i] != NULL){
			if(proc_table[i]->proc_id == curproc->proc_id)
			break;
		}
		if(i == MAX_PROC - 1)
		panic("Current process not found in process table");
	}

	lock_acquire(curproc->lock);
	curproc->exit_status = true;
	curproc->exit_code = _MKWAIT_EXIT(exitcode);
	KASSERT(curproc->exit_status == proc_table[i]->exit_status);
	KASSERT(curproc->exit_code == proc_table[i]->exit_code);
	lock_release(curproc->lock);
	thread_exit();
}

