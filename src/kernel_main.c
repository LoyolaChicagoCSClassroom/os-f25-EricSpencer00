
#include <stdint.h>
#include <stddef.h>
#include "paging.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

struct termbuf {
    char ascii;
    char color;
};

void scroll_up(void) {
    //tba
}

int x = 0;
int y = 0;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// int x = 0, y = 0;

void print_char(char c) {
    struct termbuf *vram = (struct termbuf *)0xB8000;

    if (c == '\n') {
        x = 0;
        y++;
        if (y >= VGA_HEIGHT) {
            // scroll up
            for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
                vram[i] = vram[i + VGA_WIDTH];
            }
            // clear last line
            for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
                vram[i].ascii = ' ';
                vram[i].color = 7;
            }
            y = VGA_HEIGHT - 1;
        }
        return;
    }

    vram[y * VGA_WIDTH + x].ascii = c;
    vram[y * VGA_WIDTH + x].color = 7;
    x++;
    // if (x >= VGA_WIDTH) {
    //     x = 0;
    //     y++;
    //     if (y >= VGA_HEIGHT) {
    //         // scroll up
    //         for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
    //             vram[i] = vram[i + VGA_WIDTH];
    //         }
    //         // clear last line
    //         for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
    //             vram[i].ascii = ' ';
    //             vram[i].color = 7;
    //         }
    //         y = VGA_HEIGHT - 1;
    //     }
    // }
}
void print_string(char *s) {
    while (*s != 0) {
        print_char(*s);
        s++;
    }
}

void print_hex8(uint8_t b) {
    const char *hex = "0123456789ABCDEF";
    print_char(hex[(b >> 4) & 0xF]);
    print_char(hex[b & 0xF]);
}

void print_hex32(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        print_char(hex[(val >> (i * 4)) & 0xF]);
    }
}

void print_pointer(void *ptr) {
    print_string("0x");
    print_hex32((uint32_t)ptr);
}

void print_decimal(int num) {
    if (num == 0) {
        print_char('0');
        return;
    }
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    char buf[16];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        print_char(buf[--i]);
    }
}

// Simple bump allocator for kernel heap simulation
static uint8_t heap[4096];
static uint32_t heap_offset = 0;

void *malloc(uint32_t size) {
    if (heap_offset + size > sizeof(heap)) {
        return (void*)0;
    }
    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

// Linked list structure
struct list_element {
    struct list_element *next;
};

// Pagination: wait for a key press and clear screen
void wait_for_key_and_clear(void) {
    print_string("\n--- Press any key to continue ---");
    
    // Wait for key press
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 1) {
            inb(0x60); // consume scancode
            break;
        }
    }
    
    // Clear screen
    struct termbuf *vram = (struct termbuf *)0xB8000;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vram[i].ascii = ' ';
        vram[i].color = 7;
    }
    x = 0;
    y = 0;
}

void check_pagination(void) {
    if (y >= VGA_HEIGHT - 1) {
        wait_for_key_and_clear();
    }
}

void main() {
    extern char _end_kernel;
    extern char _start_stack;
    extern char _end_stack;

    // Identity map kernel range 0x100000 -> &_end_kernel
    uintptr_t kstart = 0x100000;
    uintptr_t kend = (uintptr_t)&_end_kernel;
    for (uintptr_t a = kstart; a < kend; a += 0x1000) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void*)a;
        map_pages((void*)a, &tmp, pd);
    }

    // Identity map stack region
    uintptr_t sstart = (uintptr_t)&_start_stack;
    uintptr_t send = (uintptr_t)&_end_stack;
    for (uintptr_t a = sstart; a < send; a += 0x1000) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void*)a;
        map_pages((void*)a, &tmp, pd);
    }

    // Identity map VGA buffer at 0xB8000 (map one page)
    struct ppage vtmp;
    vtmp.next = NULL;
    vtmp.prev = NULL;
    vtmp.physical_addr = (void*)0xB8000;
    map_pages((void*)0xB8000, &vtmp, pd);

    // Load page directory and enable paging
    loadPageDirectory(pd);
    enable_paging();

    print_string("=== Linked List Demonstration ===\n\n");
    check_pagination();
    
    // Initialize array of list elements
    struct list_element arr[10] = { {.next = &arr[1]} };
    
    // Allocate a new list element using malloc
    struct list_element *n = malloc(sizeof(struct list_element));
    print_string("malloc() returned ");
    print_pointer(n);
    print_char('\n');
    check_pagination();
    
    // Link the malloc'd element to arr[0]
    n->next = &arr[0];
    print_pointer(n);
    print_string(".next = ");
    print_pointer(n->next);
    print_char('\n');
    check_pagination();
    
    // Build the linked list chain in the array
    for (int i = 1; i < 10-1; i++) {
        arr[i].next = &arr[i+1];
    }
    arr[9].next = (struct list_element*)0; // NULL terminator
    
    print_string("\nArray elements and their next pointers:\n");
    check_pagination();
    for (int i = 0; i < 10; i++) {
        print_pointer(&arr[i]);
        print_string(".next = ");
        print_pointer(arr[i].next);
        print_char('\n');
        check_pagination();
    }
    
    print_string("\nTraversing list with for loop:\n");
    check_pagination();
    for (struct list_element *i = &arr[0]; i != (struct list_element*)0; i = i->next) {
        print_pointer(i);
        print_char('\n');
        check_pagination();
    }
    
    print_string("\nTraversing list with while loop:\n");
    check_pagination();
    struct list_element *p = &arr[0];
    while (p != (struct list_element*)0) {
        print_string("p = ");
        print_pointer(p);
        print_char('\n');
        check_pagination();
        p = p->next;
    }
    
    print_string("\nExtra element linked to arr[0]:\n");
    check_pagination();
    struct list_element extra = {.next = &arr[0]};
    print_pointer(&extra);
    print_string(".next = ");
    print_pointer(extra.next);
    print_char('\n');
    check_pagination();
    
    print_string("\n=== Starting Keyboard Scanner ===\n");
    print_string("Press keys to see scancodes...\n\n");
    check_pagination();
    
    // Keyboard polling loop
    while (1) {
        uint8_t status = inb(0x64);

        if (status & 1) {
            uint8_t scancode = inb(0x60);
            print_string("SC:");
            print_hex8(scancode);
            print_char('\n');
            check_pagination();
        }
    }
}
