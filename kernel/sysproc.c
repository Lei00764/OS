#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace(); // step4
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_sigalarm(void) // alarm-step6
{
  int interval;   // 用于存储时间间隔的变量
  uint64 handler; // 用于存储处理程序地址的变量
  struct proc *p = myproc();
  // 接收系统调用第0个参数和第一个参数
  if (argint(0, &interval) < 0)
  {
    return -1;
  }
  if (argaddr(1, &handler) < 0)
  {
    return -1;
  }
  // 设置进程的时间间隔和处理程序
  p->interval = interval;
  p->handler = handler;
  p->ticks = 0;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();        // 获取正在运行的进程的指针
  *p->trapframe = *p->pretrapframe; // 从之前保存的状态中恢复到发生系统调用前的状态
  p->ticks = 0;
  return 0;
}