#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Initialize paging expansion and allocators (Week 2) */
void memory_init(uint64_t mb_magic, uint64_t mb_info);

/* Very simple 2 MiB frame allocator (identity-mapped region) */
uint64_t pmm_alloc_2m(void);   /* returns physical base or 0 on OOM */
void     pmm_free_2m(uint64_t phys);

/* Tiny bump allocator for kernel heap */
void* kmalloc(size_t size);
void  kfree(void* ptr); /* no-op for bump allocator */

/* Stats */
size_t pmm_total_frames(void);
size_t pmm_free_frames(void);

/* Optional: 4 KiB mapping helpers (stubs for now) */
int map_page_4k(uint64_t phys, uint64_t virt, uint64_t flags);
int unmap_page_4k(uint64_t virt);

/* Week 5 helper: mark identity-mapped 2MiB pages in [start,end) as user-accessible */
void memory_mark_user_range(uint64_t start, uint64_t end);

#endif // MEMORY_H
