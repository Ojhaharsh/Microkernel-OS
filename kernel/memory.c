// Week 2: very simple memory subsystem
#include <stdint.h>
#include <stddef.h>
#include "memory.h"

/* Page table structures (we will extend the identity map to 64 MiB) */
typedef uint64_t pte_t;

extern pte_t pml4[]; /* from boot.S */
extern pte_t pdpt[];
extern pte_t pd[];

static inline void invlpg(void* addr) {
	__asm__ __volatile__("invlpg (%0)" :: "r"(addr) : "memory");
}

/* 4 KiB page mapping (very basic; assumes identity PD/PT not yet built). For Week 2, stub. */
int map_page_4k(uint64_t phys, uint64_t virt, uint64_t flags) {
	(void)phys; (void)virt; (void)flags;
	/* Full 4 KiB paging builder will be added in Week 2+; keeping identity mapping for now. */
	return 0;
}

int unmap_page_4k(uint64_t virt) {
	(void)virt; return 0;
}

static void map_identity_2m_range(uint64_t start_phys, uint64_t end_phys) {
	/* We assume pml4[0] -> pdpt, pdpt[0] -> pd already in place.
	   We will fill PD entries as 2 MiB pages up to end_phys. */
	const uint64_t PAGE_2M = 2ULL * 1024 * 1024;
	uint64_t phys = start_phys;
	size_t index = (start_phys / PAGE_2M); /* PD[0] already maps 0..2MiB */
	if (index == 0) {
		phys += PAGE_2M; /* skip first, already present */
		index = 1;
	}
	while (phys < end_phys && index < 512) {
		pd[index] = (phys & ~((uint64_t)0x1FFFFF)) | 0x083ULL; /* present|rw|PS (supervisor) */
		invlpg((void*)phys);
		phys += PAGE_2M;
		index++;
	}
}

/*
 * Physical 2 MiB frame allocator over a fixed identity-mapped window.
 * For Week 2, keep it tiny and deterministic:
 * - Manage frames from 2 MiB up to 64 MiB (i.e., 31 frames of 2 MiB after the first).
 */

#define FRAME_SIZE_2M   (2ULL * 1024 * 1024)
#define PMM_LIMIT       (512ULL * 1024 * 1024)  /* consider memory up to 512 MiB for Week 2 */
#define FRAME_COUNT     (PMM_LIMIT / FRAME_SIZE_2M) /* 256 frames */

static uint64_t frame_bitmap[(FRAME_COUNT + 63) / 64];

static inline void fb_set(size_t i)   { frame_bitmap[i >> 6] |=  (1ULL << (i & 63)); }
static inline void fb_clear(size_t i) { frame_bitmap[i >> 6] &= ~(1ULL << (i & 63)); }
static inline int  fb_test(size_t i)  { return (frame_bitmap[i >> 6] >> (i & 63)) & 1U; }

/* Stats */
static size_t total_frames = 0;
static size_t free_frames  = 0;

size_t pmm_total_frames(void) { return total_frames; }
size_t pmm_free_frames(void)  { return free_frames; }

static void pmm_reserve_range(uint64_t start, uint64_t end) {
	if (end > PMM_LIMIT) end = PMM_LIMIT;
	if (start >= end) return;
	size_t first = (size_t)(start / FRAME_SIZE_2M);
	size_t last  = (size_t)((end   + FRAME_SIZE_2M - 1) / FRAME_SIZE_2M);
	if (last > FRAME_COUNT) last = FRAME_COUNT;
	for (size_t i = first; i < last; ++i) {
		if (!fb_test(i)) { fb_set(i); if (free_frames) free_frames--; }
	}
}

static void pmm_mark_usable_range(uint64_t start, uint64_t end) {
	if (start >= PMM_LIMIT) return;
	if (end > PMM_LIMIT) end = PMM_LIMIT;
	uint64_t a = (start + FRAME_SIZE_2M - 1) & ~(FRAME_SIZE_2M - 1); /* align up */
	uint64_t b = (end) & ~(FRAME_SIZE_2M - 1);                        /* align down */
	if (a >= b) return;
	size_t first = (size_t)(a / FRAME_SIZE_2M);
	size_t last  = (size_t)(b / FRAME_SIZE_2M);
	if (last > FRAME_COUNT) last = FRAME_COUNT;
	for (size_t i = first; i < last; ++i) {
		if (fb_test(i)) { fb_clear(i); free_frames++; }
	}
}

uint64_t pmm_alloc_2m(void) {
	for (size_t i = 1; i < FRAME_COUNT; ++i) { /* skip frame 0 (first 2 MiB) */
		if (!fb_test(i)) {
			fb_set(i);
			if (free_frames) free_frames--;
			return (uint64_t)i * FRAME_SIZE_2M;
		}
	}
	return 0;
}

void pmm_free_2m(uint64_t phys) {
	size_t i = (size_t)(phys / FRAME_SIZE_2M);
	if (i == 0 || i >= FRAME_COUNT) return;
	if (fb_test(i)) { fb_clear(i); free_frames++; }
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

/* Multiboot2 parsing */
typedef struct {
	uint32_t type;
	uint32_t size;
} __attribute__((packed)) mb2_tag_t;

typedef struct {
	uint32_t type;       /* = 6 */
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	/* entries follow */
} __attribute__((packed)) mb2_tag_mmap_t;

typedef struct {
	uint64_t addr;
	uint64_t len;
	uint32_t type;   /* 1 = usable */
	uint32_t zero;
} __attribute__((packed)) mb2_mmap_entry_t;

extern uint8_t _kernel_start; /* from linker */
extern uint8_t _kernel_end;

void memory_init(uint64_t mb_magic, uint64_t mb_info) {
	/* Identity-map up to 64 MiB using 2 MiB pages for early stability */
	map_identity_2m_range(0, 64ULL * 1024 * 1024);

	/* Initialize bitmap: mark everything reserved */
	for (size_t i = 0; i < (FRAME_COUNT + 63) / 64; ++i) frame_bitmap[i] = ~0ULL;
	total_frames = FRAME_COUNT - 1; /* exclude frame 0 */
	free_frames = 0;

	/* If Multiboot2 info present, parse memory map and mark usable frames */
	if (mb_magic == 0x36D76289 && mb_info) {
		uint8_t* base = (uint8_t*)(uintptr_t)mb_info;
		uint32_t total_size = *(uint32_t*)base;
		uint32_t off = 8; /* skip total + reserved */
		while (off + sizeof(mb2_tag_t) <= total_size) {
			mb2_tag_t* tag = (mb2_tag_t*)(base + off);
			if (tag->type == 0 && tag->size == 8) break; /* end */
			if (tag->type == 6) {
				mb2_tag_mmap_t* mmt = (mb2_tag_mmap_t*)tag;
				uint32_t eoff = off + sizeof(mb2_tag_mmap_t);
				while (eoff + mmt->entry_size <= off + mmt->size) {
					mb2_mmap_entry_t* e = (mb2_mmap_entry_t*)(base + eoff);
					if (e->type == 1 && e->len) {
						pmm_mark_usable_range(e->addr, e->addr + e->len);
					}
					eoff += mmt->entry_size;
				}
			}
			/* advance to next 8-byte aligned tag */
			uint32_t next = off + ((tag->size + 7) & ~7U);
			if (next <= off) break; /* overflow guard */
			off = next;
		}
	} else {
		/* Fallback: assume 2..64 MiB usable */
		pmm_mark_usable_range(2ULL * 1024 * 1024, 64ULL * 1024 * 1024);
	}

	/* Reserve frame 0 (first 2 MiB) */
	pmm_reserve_range(0, 2ULL * 1024 * 1024);
	/* Reserve kernel range */
	uint64_t kstart = (uint64_t)(uintptr_t)&_kernel_start;
	uint64_t kend   = (uint64_t)(uintptr_t)&_kernel_end;
	pmm_reserve_range(kstart, kend);
}

/* Mark identity-mapped 2MiB pages covering [start,end) as user-accessible (set U/S bit) */
void memory_mark_user_range(uint64_t start, uint64_t end) {
	if (end > 64ULL * 1024 * 1024) end = 64ULL * 1024 * 1024;
	if (start >= end) return;
	const uint64_t PAGE_2M = 2ULL * 1024 * 1024;
	uint64_t phys = start & ~(PAGE_2M - 1);
	size_t index = (size_t)(phys / PAGE_2M);
	while (phys < end && index < 512) {
		uint64_t entry = pd[index];
		if (entry & 0x80ULL) { /* PS */
			entry |= 0x4ULL;   /* set U/S=1 (user) */
			pd[index] = entry;
			invlpg((void*)phys);
		}
		phys += PAGE_2M;
		index++;
	}
	/* Also set U bit on upper levels used for this mapping */
	pml4[0] |= 0x4ULL;
	pdpt[0] |= 0x4ULL;
}
