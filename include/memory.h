#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Initialize paging expansion and allocators (Week 2) */
void memory_init(void);

/* Very simple 2 MiB frame allocator (identity-mapped region) */
uint64_t pmm_alloc_2m(void);   /* returns physical base or 0 on OOM */
void     pmm_free_2m(uint64_t phys);

/* Tiny bump allocator for kernel heap */
void* kmalloc(size_t size);
void  kfree(void* ptr); /* no-op for bump allocator */

#endif // MEMORY_H
