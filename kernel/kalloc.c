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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct ref_stru {
  struct spinlock lock;
  int cnt[PHYSTOP / PGSIZE];  // number of references to each page.
} ref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // set it to one, kfree will do the minus one.
    ref.cnt[(uint64)p / PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  acquire(&ref.lock);
  if(--ref.cnt[(uint64)pa / PGSIZE] != 0){
    release(&ref.lock);
    return;
  }

  release(&ref.lock);
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&ref.lock);
    // init the counter to one
    ref.cnt[(uint64)r / PGSIZE] = 1;
    release(&ref.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// check whether the given page table is using for COW case
// return 1 if it is, 0 otherwise
int
cowpage(pagetable_t pagetable, uint64 va) {
  // invalid virtual address
  if(va >= MAXVA) return 0;
  // get the page table entry
  pte_t* pte = walk(pagetable, va, 0);
  // invalid page table entry
  if(pte == 0 || (*pte & PTE_V) == 0)
    return 0;
  return (*pte & PTE_COW); // check the COW bit
}

// alloc a page table for a new writing operation
// assume the page table is valid and only using for COW case
// return 0 for failure, otherwise for success
void*
cowalloc(pagetable_t pagetable, uint64 va) {
  va = PGROUNDDOWN(va);

  if(va % PGSIZE != 0) return 0;

  uint64 pa = walkaddr(pagetable, va);
  if(pa == 0) return 0;

  pte_t* pte = walk(pagetable, va, 0);

  if(krefcnt((char*)pa) == 1) {
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
    return (void*)pa;
  } else {
    char *ka = kalloc();
    if(ka == 0) return 0;
    
    // copy the page to the new page
    memmove(ka, (char*)pa, PGSIZE);
    // clear the old page
    *pte &= ~PTE_V;

    // set the new mapping in the page table
    uint flags = PTE_FLAGS(*pte);
    flags = (flags | PTE_W) & ~PTE_COW;

    if(mappages(pagetable, va, PGSIZE, (uint64)ka, flags) != 0) {
      kfree(ka);
      *pte |= PTE_V;
      return 0;
    }

    // remove the old memory usage
    // this will decrease the reference count of address
    kfree((char*)PGROUNDDOWN(pa));
    return (void*)ka;
  }
}

// get the current ref count of the given address
int
krefcnt(void* pa) {
  return ref.cnt[(uint64)pa / PGSIZE];
}

// increase the ref count of the given address
int
kaddrefcnt(void* pa) {
  // invalid address
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&ref.lock);
  ++ref.cnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  return 0;
}