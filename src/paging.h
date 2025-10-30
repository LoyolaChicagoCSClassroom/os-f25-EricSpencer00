#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "page.h"

struct page_directory_entry {
   uint32_t present       : 1;
   uint32_t rw            : 1;
   uint32_t user          : 1;
   uint32_t writethru     : 1;
   uint32_t cachedisabled : 1;
   uint32_t accessed      : 1;
   uint32_t pagesize      : 1;
   uint32_t ignored       : 2;
   uint32_t os_specific   : 3;
   uint32_t frame         : 20;
};

struct page {
   uint32_t present    : 1;
   uint32_t rw         : 1;
   uint32_t user       : 1;
   uint32_t accessed   : 1;
   uint32_t dirty      : 1;
   uint32_t unused     : 7;
   uint32_t frame      : 20;
};

/* Map a linked list of physical pages (ppage) to the virtual address vaddr using page directory pd.
   Returns the starting virtual address mapped. Assumes 4KB pages and that pd and any page tables
   referenced are aligned to 4KB. */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd);

/* Load PD into CR3 */
void loadPageDirectory(struct page_directory_entry *pd);

/* Enable paging by setting CR0 bits */
void enable_paging(void);

#endif
