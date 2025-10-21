#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr;
};

/* Initialize the page frame allocator list */
void init_pfa_list(void);

/* Allocate npages contiguous ppage structures from the free list and return the head of the alloc'd list */
struct ppage *allocate_physical_pages(unsigned int npages);

/* Free a list of pages (adds them back to the free list) */
void free_physical_pages(struct ppage *ppage_list);

#endif // PAGE_H
