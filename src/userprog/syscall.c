#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "list.h"
#include "process.h"
#include "filesys/filesys.h"


static void syscall_handler (struct intr_frame *);

static void exit_proc(int);
static void valid_vaddr(const void *);
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
  //printf("syscall no : %d\n", *esp);
  switch(*esp){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      valid_vaddr(esp+1);
      exit(*(esp+1));
      break;
    case SYS_EXEC:
      valid_vaddr(esp+1);
      f->eax = exec((const char*) *(esp+1));
      break;
    case SYS_WAIT:
      valid_vaddr(esp+1);
      f->eax = wait((pid_t) *(esp+1));
      break;
    case SYS_CREATE:
      valid_vaddr(esp+1);
      valid_vaddr(esp+2);
      valid_vaddr(*(esp+1));
      //valid_vaddr(*(esp+2));
      f->eax = create((const char*) *(esp+1), (unsigned) *(esp+2));
      // eax == false , then what happen?
      break;
    case SYS_REMOVE:
      valid_vaddr(esp+1);
      f->eax = remove((const char*) *(esp+1));
      break;
    case SYS_OPEN:
      valid_vaddr(esp+1);
      valid_vaddr(*(esp+1));
      f->eax = open((const char*) *(esp+1));
      break;
    case SYS_FILESIZE:
      valid_vaddr(esp+1);
      f->eax = filesize((const char*) *(esp+1));
      break;
    case SYS_READ:
      valid_vaddr(esp+1);
      valid_vaddr(esp+2);
      valid_vaddr(esp+3);
      valid_vaddr(*(esp+2));
      f->eax = read((int) *(esp+1), (void*) *(esp+2), (unsigned) *(esp+3));
      break;
    case SYS_WRITE:
      valid_vaddr(esp+1);
      valid_vaddr(esp+2);
      valid_vaddr(esp+3);
      valid_vaddr(*(esp+2));
      f->eax = write((int) *(esp+1), (const void*) *(esp+2), (unsigned) *(esp+3));
      break;
    case SYS_SEEK:
      valid_vaddr(esp+1);
      valid_vaddr(esp+2);
      valid_vaddr(*(esp+1));
      seek((int) *(esp+1), (unsigned) *(esp+2));
      break;
    case SYS_TELL:
      valid_vaddr(esp+1);
      valid_vaddr(*(esp+1));
      f->eax = tell((int) *(esp+1));
      break;
    case SYS_CLOSE:
      valid_vaddr(esp+1);
      valid_vaddr(*(esp+1));
      close((int) *(esp+1));
      break;
  }
  //thread_exit ();
}

static void
valid_vaddr(const void * va){

  // Assert va != NULL                                                                            
  // Assert va in user_memory                                                                     
  // Assert thread_current() page_dir + vaddr is valid 
  // printf("debugging_valid_vaddr : %c\n", (char *) va);

  if ( !va || !is_user_vaddr(va)){
    exit(-1);
    //return 0;
  }
  
  void * kva = pagedir_get_page(thread_current()->pagedir, va);
  
  if(!kva){
    exit(-1);
    //return 0;
  }
  //return kva;
}

void
halt (void){
  power_off();
}

void
exit (int status){
  struct thread *t = thread_current();
  
  t -> exit_status = status;
  t -> used = true;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

pid_t
exec (const char *file_name){


  char * fn = malloc(strlen(file_name)+1);
  char * next;
  strlcpy(fn, file_name, strlen(file_name)+1);
  fn = strtok_r(fn, " ", &next);
  struct file* f = filesys_open (fn);
  if (f == NULL) return -1; 
  return process_execute(file_name);

}

int
wait(pid_t pid){
  return process_wait(pid);
}

bool
create(const char *file, unsigned initial_size){
  if(file == NULL){
    exit(-1);
    //printf("filename NULL\n");
    return false;
  }
  return filesys_create(file, initial_size);
}

bool
remove(const char *file){
  if(file==NULL)
    return false;
  return filesys_remove(file);
}

int
open(const char *file){
  struct file *f = filesys_open(file);
  
  if(f == NULL){
    //exit(-1);
    return -1;
  }

  struct file_elem *felem= (struct file_elem *) malloc(sizeof(struct file_elem));

  if(felem==NULL){
    file_close (f);
    return -1;
  }

  felem->file=f;
  felem->fd =thread_current()->fd_count;
  thread_current()->fd_count++;
  list_push_back(&thread_current()->file_list, &felem->f_elem);
  return felem->fd;
}

int
filesize(int fd){
  struct file_elem *felem= find_file(fd);

  if(felem==NULL)
    return -1;

  return file_length(felem->file);
}

int
read(int fd, void *buffer, unsigned size){
  int i;
  if(fd == 0){
    for( i=0;i<size;i++){
      *(char *)(buffer+i)=input_getc();
      return size;
    }
  }

  struct file_elem *felem= find_file(fd);
  if(felem==NULL)
      return -1;
  return file_read(felem->file, buffer, size);
}

int
write(int fd, const void *buffer, unsigned size){
  if(fd==0)
    return -1;

  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }

  struct file_elem *felem= find_file(fd);
  if(felem==NULL)
    return -1;
  return file_write(felem->file, buffer, size); 
}

void
seek(int fd, unsigned position){
  struct file_elem *felem= find_file(fd);
  if(felem==NULL)
    return -1;
  return file_seek(felem->file);
}

unsigned
tell(int fd){
  struct file_elem *felem= find_file(fd);
  if(felem==NULL)
    return -1;
  return file_tell(felem->file);
}

void
close(int fd){
  struct file_elem *felem= find_file(fd);
  if(felem==NULL)
    return -1;
  return file_close(felem->file);
}
