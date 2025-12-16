#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// Системный вызов: узнать свой UID
int
sys_getuid(void)
{
  return myproc()->uid;
}

// Системный вызов: узнать свой GID
int
sys_getgid(void)
{
  return myproc()->gid;
}

// Системный вызов: сменить UID (только root!)
int
sys_setuid(void)
{
  int uid;

  if(argint(0, &uid) < 0)
    return -1;

  struct proc *curproc = myproc();

  // Только root может менять UID
  if(curproc->uid != 0)
    return -1;

  curproc->uid = uid;
  return 0;
}

// Системный вызов: login (вход под пользователем)
int
sys_login(void)
{
  char *username, *password;

  if(argstr(0, &username) < 0 || argstr(1, &password) < 0)
    return -1;

  int uid = users_checkpassword(username, password);

  if(uid < 0)
    return -1;  // Неверный логин/пароль

  // Меняем UID текущего процесса
  myproc()->uid = uid;
  return 0;
}

// Системный вызов: whoami (узнать своё имя)
int
sys_whoami(void)
{
  char *buf;
  int size;

  if(argint(1, &size) < 0)
    return -1;

  if(argptr(0, &buf, size) < 0)
    return -1;

  users_getname(myproc()->uid, buf, size);
  return 0;
}
