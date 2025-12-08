#include "paging.h"
#include <stddef.h>
#include <stdint.h>

/* Global page directory (4KB-aligned) */
struct page_directory_entry pd[1024] __attribute__((aligned(4096)));

/* Pool of page tables (one per PD entry) to support mapping across multiple page-directory ranges */
static struct page pt_pool[1024][1024] __attribute__((aligned(4096)));

void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd_in) {
    uintptr_t va = (uintptr_t)vaddr;
    struct page_directory_entry *pd_local = pd_in ? pd_in : pd;

    // Compute page directory index
    uint32_t pd_index = (va >> 22) & 0x3FF;

    // If the directory entry isn't present, install a page table from the pool
    if (!pd_local[pd_index].present) {
        // Use the page table corresponding to this PD index
        struct page *pt_for_index = pt_pool[pd_index];
        uintptr_t pt_phys = (uintptr_t)pt_for_index;
        pd_local[pd_index].present = 1;
        pd_local[pd_index].rw = 1;
        pd_local[pd_index].user = 0;
        pd_local[pd_index].writethru = 0;
        pd_local[pd_index].cachedisabled = 0;
        pd_local[pd_index].accessed = 0;
        pd_local[pd_index].pagesize = 0; // using 4KB pages
        pd_local[pd_index].ignored = 0;
        pd_local[pd_index].os_specific = 0;
        pd_local[pd_index].frame = (uint32_t)(pt_phys >> 12);
        // Clear the chosen page table
        for (int i = 0; i < 1024; i++) {
            pt_for_index[i].present = 0;
            pt_for_index[i].rw = 0;
            pt_for_index[i].user = 0;
            pt_for_index[i].accessed = 0;
            pt_for_index[i].dirty = 0;
            pt_for_index[i].unused = 0;
            pt_for_index[i].frame = 0;
        }
    }

    struct page *ptable = pt_pool[pd_index]; // Use the page table for this PD index
    uintptr_t start_va = va;
    struct ppage *p = pglist;
    while (p) {
        uint32_t pt_index = (va >> 12) & 0x3FF;
        // set ptable[pt_index]
        uintptr_t phys = (uintptr_t)p->physical_addr;
        ptable[pt_index].present = 1;
        ptable[pt_index].rw = 1;
        ptable[pt_index].user = 0;
        ptable[pt_index].accessed = 0;
        ptable[pt_index].dirty = 0;
        ptable[pt_index].unused = 0;
        ptable[pt_index].frame = (uint32_t)(phys >> 12);

        // advance
        va += 0x1000;
        p = p->next;
    }

    return (void*)start_va;
}

void loadPageDirectory(struct page_directory_entry *pd) {
    asm volatile ("mov %0,%%cr3" : : "r" (pd) : );
}

void enable_paging(void) {
    asm volatile (
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        :::"eax"
    );
}
