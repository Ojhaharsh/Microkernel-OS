// Week 2: very simple memory subsystem
#include <stdint.h>
#include <stddef.h>
#include "memory.h"

/*
 * Physical 2 MiB frame allocator over a fixed identity-mapped window.
 * For Week 2, keep it tiny and deterministic:
 * - Manage frames from 2 MiB up to 64 MiB (i.e., 31 frames of 2 MiB after the first).
 */

#define FRAME_SIZE_2M   (2ULL * 1024 * 1024)
#define PMM_START       (2ULL * 1024 * 1024)   /* skip the first 2 MiB we used for boot */
#define PMM_END         (64ULL * 1024 * 1024)  /* manage up to 64 MiB */

static uint64_t pmm_next = PMM_START;

uint64_t pmm_alloc_2m(void) {
	if (pmm_next + FRAME_SIZE_2M > PMM_END) return 0;
	uint64_t ret = pmm_next;
	pmm_next += FRAME_SIZE_2M;
	return ret;
}

void pmm_free_2m(uint64_t phys) {
	(void)phys; /* no free list in this trivial allocator */
}

/* Tiny bump allocator for the kernel (carved from a simple static buffer) */
static uint8_t kheap[64 * 1024];
static size_t  kheap_off = 0;

void* kmalloc(size_t size) {
	if (size == 0) return NULL;
	/* 16-byte align */
	size = (size + 15) & ~((size_t)15);
	if (kheap_off + size > sizeof(kheap)) return NULL;
	void* ptr = &kheap[kheap_off];
	kheap_off += size;
	return ptr;
}

void kfree(void* ptr) { (void)ptr; }

/* For Week 2 we’ll expand mapping later; keep init as a placeholder now */
void memory_init(void) {
	/* In a later step, we’ll add code to build more page tables and map the range we allocate. */
}
