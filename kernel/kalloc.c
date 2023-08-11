// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  // 每个CPU都有一个lock与freeList
  struct spinlock lock;
  struct run *freelist;
  // 每个CPU拥有的空闲页数
  int freePageCount;
} kmem[NCPU];

void kinit()
{
  for (int i = 0; i < NCPU; i++) // 初始化所有锁
    initlock(&kmem[i].lock, "kmem");

  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  // 检查传入的地址是否合法
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 用特定值填充释放的内存块，以捕捉悬空引用
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  // 获取当前CPU的编号，即核心号
  push_off();
  int nowCpuId = cpuid();
  // 获取当前CPU的内存分配器锁，确保只有一个进程可以访问内存分配器
  acquire(&kmem[nowCpuId].lock);
  // 将释放的内存块添加到当前CPU的空闲列表中
  r->next = kmem[nowCpuId].freelist;
  kmem[nowCpuId].freelist = r;
  // 更新当前CPU的空闲页计数
  kmem[nowCpuId].freePageCount++;
  // 释放当前CPU的内存分配器锁
  release(&kmem[nowCpuId].lock);
  // 恢复中断状态
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
  struct run *r;

  // 关中断，确保在执行内存分配的过程中不会被中断
  push_off();
  // 获取当前CPU的编号，即核心号
  int nowCpuId = cpuid();
  // 获取当前CPU的内存分配器锁，确保只有一个进程可以访问内存分配器
  acquire(&kmem[nowCpuId].lock);

  // 尝试从当前CPU的空闲列表中获取一个内存块
  r = kmem[nowCpuId].freelist;
  if (r)
  {
    // 从当前CPU的空闲列表中移除一个内存块
    kmem[nowCpuId].freelist = r->next;
    // 更新当前CPU的空闲页计数
    kmem[nowCpuId].freePageCount--;
    // 释放当前CPU的内存分配器锁
    release(&kmem[nowCpuId].lock);
  }
  else
  {
    // 释放当前CPU的内存分配器锁
    release(&kmem[nowCpuId].lock);

    // 从其他CPU的空闲列表中“偷取”一个内存块
    int selectedCpuId = 0;
    acquire(&kmem[0].lock);
    for (int i = 1; i < NCPU; i++)
    {
      if (i == nowCpuId)
        continue;
      acquire(&kmem[i].lock);
      // 找到空闲页最多的CPU，以便从其空闲列表中“偷取”内存块
      if (kmem[i].freePageCount > kmem[selectedCpuId].freePageCount)
      {
        release(&kmem[selectedCpuId].lock);
        selectedCpuId = i;
      }
      else
      {
        release(&kmem[i].lock);
      }
    }
    // 从选定的CPU的空闲列表中获取一个内存块
    r = kmem[selectedCpuId].freelist;
    if (r)
    {
      // 从选定的CPU的空闲列表中移除一个内存块
      kmem[selectedCpuId].freelist = r->next;
      // 更新选定的CPU的空闲页计数
      kmem[selectedCpuId].freePageCount--;
    }
    // 释放选定的CPU的内存分配器锁
    release(&kmem[selectedCpuId].lock);
  }

  // 恢复中断状态
  pop_off();
  // 如果成功分配了内存块，则进行初始化操作，将分配的内存块的内容填充为特定值（用于调试）
  if (r)
    memset((char *)r, 5, PGSIZE);
  // 返回分配的内存块的指针
  return (void *)r;
}
