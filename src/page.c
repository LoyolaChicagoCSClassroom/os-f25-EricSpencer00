#include "page.h"
#include <stddef.h>

/* Statically allocate 128 physical page descriptors */
static struct ppage physical_page_array[128];

/* Head of free physical pages list */
static struct ppage *free_physical_head = NULL;

void init_pfa_list(void) {
    free_physical_head = NULL;
    for (int i = 0; i < 128; i++) {
        physical_page_array[i].next = free_physical_head;
        physical_page_array[i].prev = NULL;
        physical_page_array[i].physical_addr = (void*)((uintptr_t)i * 0x200000); // 2MB increments
        if (free_physical_head) {
            free_physical_head->prev = &physical_page_array[i];
        }
        free_physical_head = &physical_page_array[i];
    }
}

/* Helper: unlink a single page from the free list */
static struct ppage *unlink_head(void) {
    if (!free_physical_head) return NULL;
    struct ppage *p = free_physical_head;
    free_physical_head = p->next;
    if (free_physical_head) free_physical_head->prev = NULL;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0) return NULL;
    struct ppage *head = NULL;
    struct ppage *tail = NULL;
    for (unsigned int i = 0; i < npages; i++) {
        struct ppage *p = unlink_head();
        if (!p) {
            // Out of free pages: simplistic rollback - put the chunk we grabbed back at the front
            if (head) {
                // append the taken chunk back to head of free list
                struct ppage *t = head;
                while (t->next) t = t->next;
                t->next = free_physical_head;
                if (free_physical_head) free_physical_head->prev = t;
                free_physical_head = head;
            }
            return NULL;
        }
        if (!head) {
            head = tail = p;
        } else {
            tail->next = p;
            p->prev = tail;
            tail = p;
        }
    }
    return head;
}

void free_physical_pages(struct ppage *ppage_list) {
    if (!ppage_list) return;
    // Find tail of the list to be appended
    struct ppage *tail = ppage_list;
    while (tail->next) tail = tail->next;

    // Prepend this list onto free_physical_head
    if (free_physical_head) {
        free_physical_head->prev = tail;
    }
    tail->next = free_physical_head;
    ppage_list->prev = NULL;
    free_physical_head = ppage_list;
}
