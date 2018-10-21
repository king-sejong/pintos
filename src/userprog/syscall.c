#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "list.h"
#include "process.h"


static void syscall_handler (struct intr_frame *);

static void exit_proc(int);
static void* valid_vaddr(const void *);
static void* exec_proc(char *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{

  int * esp = f->esp;
  valid_vaddr(esp);
  //printf ("system call!\n");
  //printf ("system call no : %d \n", *esp);
  switch(*esp){
    case SYS_HALT :
      power_off();
      break;
    case SYS_EXIT :
      valid_vaddr(esp+1);
      exit_proc(*(esp+1));
      break;
    case SYS_EXEC :
      //valid_vaddr(esp+1);
      //valid_vaddr(*(esp+1));
      //f -> eax = exec_proc(*(esp+1));
      break;
    
    case SYS_WAIT :
      valid_vaddr(esp + 1);
      f -> eax = process_wait(*(esp+1));
      break;
    case SYS_CREATE :
      break;
    case SYS_REMOVE :
      break;
    case SYS_OPEN :
      break;
    case SYS_FILESIZE :
      break;
    case SYS_READ :
      break;
    case SYS_WRITE :
      write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_TELL :
      break;
    case SYS_CLOSE :
      break;
  }
  //thread_exit ();
}


static void
exit_proc(int status){


  struct thread *t = thread_current();
  
  t -> exit_status = -1;
  if(t -> parent -> wc_tid == t -> tid)
    sema_up(&t -> parent -> child_lock);
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

static void*
valid_vaddr(const void * va){

  // Assert va != NULL                                                                            
  // Assert va in user_memory                                                                     
  // Assert thread_current() page_dir + vaddr is valid 

  if (!is_user_vaddr(va)){
    exit_proc(-1);
    return 0;
  }
  
  void * kva = pagedir_get_page(thread_current()->pagedir, va);
  
  if(!kva){
    exit_proc(-1);
    return 0;
  }
  return kva;

}


static void*
exec_proc(char * fn){
  
  return (void*)0;
}

int write (int fd, const void *buffer, unsigned size) {


  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}
