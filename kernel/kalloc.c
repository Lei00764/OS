// 物理内存分配器，用于用户进程、内核栈、页表页和管道缓冲区。分配整个 4096 字节的页面。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // 内核后的第一个地址。
                   // 由 kernel.ld 定义。

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;     // 自旋锁，用于保护数据结构
  struct run *freelist;     // 空闲页面链表
  int rc[PHYSTOP / PGSIZE]; // 页面引用计数数组
} kmem;

void kinit()
{
  initlock(&kmem.lock, "kmem");    // 初始化自旋锁
  freerange(end, (void *)PHYSTOP); // 初始化空闲页面范围
}

// 释放指定范围内的页面
void freerange(void *pa_start, void *pa_end)
{
  char *p;
  // 从 pa_start 开始，将 p 初始化为下一个页面的起始地址
  p = (char *)PGROUNDUP((uint64)pa_start);
  // 遍历从 p 到 pa_end 的每一页
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    // 将 kmem.rc 对应元素设置为 1
    kmem.rc[(uint64)p / PGSIZE] = 1;
    // 调用 kfree 释放页面
    kfree(p);
  }
}

// 释放指定的物理内存页面
void kfree(void *pa)
{
  struct run *r;

  // 检查地址是否合法
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 获取 kmem.lock 自旋锁
  acquire(&kmem.lock);
  // 减少引用计数
  kmem.rc[(uint64)pa / PGSIZE]--;
  // 当引用计数减至 0 时
  if (kmem.rc[(uint64)pa / PGSIZE] <= 0)
  {
    // 清空页面内容
    memset(pa, 1, PGSIZE);
    // 添加页面到空闲页面链表
    r = (struct run *)pa;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  // 释放 kmem.lock 自旋锁
  release(&kmem.lock);
}

// 分配一个 4096 字节的物理内存页面
// 返回内核可以使用的指针
// 如果无法分配内存，则返回 0
void *kalloc(void)
{
  struct run *r;

  // 获取 kmem.lock 自旋锁
  acquire(&kmem.lock);
  // 从空闲页面链表获取一个页面
  r = kmem.freelist;
  if (r)
  {
    // 更新链表指针
    kmem.freelist = r->next;
    // 设置页面引用计数为 1
    kmem.rc[(uint64)r / PGSIZE] = 1;
  }
  // 释放 kmem.lock 自旋锁
  release(&kmem.lock);

  // 若分配成功，则将页面填充为垃圾数据
  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

// 增加指定页面的引用计数
void incr_rc(uint64 pa)
{
  // 获取 kmem.lock 自旋锁
  acquire(&kmem.lock);
  // 增加页面引用计数
  kmem.rc[pa / PGSIZE]++;
  // 释放 kmem.lock 自旋锁
  release(&kmem.lock);
}

// Copy-On-Write 分配函数
int cow_alloc(pagetable_t pagetable, uint64 va)
{
  uint64 pa;
  uint64 mem;
  pte_t *pte;
  // 检查地址是否合法
  if (va >= MAXVA)
    return -1;
  // 对虚拟地址进行向下对齐
  va = PGROUNDDOWN(va);
  // 获取页表项
  pte = walk(pagetable, va, 0);
  // 检查页表项是否存在
  if (pte == 0)
  {
    return -1;
  }
  // 检查页是否有效且为 COW 页
  if (!(*pte & PTE_V))
  {
    return -2;
  }
  // 获取物理地址
  pa = PTE2PA(*pte);

  // 获取 kmem.lock 自旋锁
  acquire(&kmem.lock);
  // 当引用计数为 1 时，取消 COW 标志，设置写权限
  if (kmem.rc[pa / PGSIZE] == 1)
  {
    *pte &= ~PTE_COW;
    *pte |= PTE_W;
    release(&kmem.lock);
    return 0;
  }
  release(&kmem.lock);

  // 分配新的内存页面
  if ((mem = (uint64)kalloc()) == 0)
  {
    return -3;
  }

  // 将页面数据复制到新内存
  memmove((void *)mem, (void *)pa, PGSIZE);

  // 更新页表项，指向新的物理页面
  *pte = ((PA2PTE(mem) | PTE_FLAGS(*pte) | PTE_W) & (~PTE_COW));

  // 释放原始物理页面
  kfree((void *)pa);

  return 0;
}
