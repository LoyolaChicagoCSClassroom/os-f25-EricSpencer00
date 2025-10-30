#include "paging.h"
#include <stddef.h>
#include <stdint.h>

/* Global page directory and one page table (both 4KB-aligned) */
struct page_directory_entry pd[1024] __attribute__((aligned(4096)));
struct page pt[1024] __attribute__((aligned(4096)));

void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd_in) {
    uintptr_t va = (uintptr_t)vaddr;
    struct page_directory_entry *pd_local = pd_in ? pd_in : pd;

    // Compute page directory index
    uint32_t pd_index = (va >> 22) & 0x3FF;

    // If the directory entry isn't present, install our single page table
    if (!pd_local[pd_index].present) {
        // pd entry points to our pt
        uintptr_t pt_phys = (uintptr_t)pt;
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
        // clear pt
        for (int i = 0; i < 1024; i++) {
            pt[i].present = 0;
            pt[i].rw = 0;
            pt[i].user = 0;
            pt[i].accessed = 0;
            pt[i].dirty = 0;
            pt[i].unused = 0;
            pt[i].frame = 0;
        }
    }

    struct page *ptable = (struct page *)pt; // use our single pt
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
