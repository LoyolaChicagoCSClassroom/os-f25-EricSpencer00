
#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "fat.h"

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

/* String utility functions for bare-metal use */
int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n-- && *s1 && *s2) {
        if (*s1 != *s2) return 1;
        s1++;
        s2++;
    }
    return 0;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 1;
        s1++;
        s2++;
    }
    return (*s1 == *s2) ? 0 : 1;
}

void memcpy(void *dest, void *src, int n) {
    char *d = (char *)dest;
    char *s = (char *)src;
    while (n--) *d++ = *s++;
}

void memset(void *s, int c, int n) {
    char *p = (char *)s;
    while (n--) *p++ = c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
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

    // Identity map low memory (0x0 to 0x100000) for VGA buffer, GRUB structures, etc.
    for (uintptr_t a = 0; a < 0x100000; a += 0x1000) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void*)a;
        map_pages((void*)a, &tmp, pd);
    }

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
    
    print_string("\n=== FAT Filesystem Driver Test ===\n\n");
    check_pagination();
    
    // Initialize FAT filesystem
    if (fatInit() == 0) {
        print_string("FAT: Filesystem initialized\n\n");
        check_pagination();
        
        // Try to open a test file
        fat_file_t testfile;
        if (fatOpen("testfile.txt", &testfile) == 0) {
            print_string("FAT: File opened, reading...\n");
            check_pagination();
            
            // Read file into a buffer
            char file_buffer[512];
            int bytes_read = fatRead(&testfile, file_buffer, sizeof(file_buffer));
            
            if (bytes_read > 0) {
                print_string("\nFAT: File contents:\n");
                print_string("---\n");
                check_pagination();
                
                // Print file contents
                for (int i = 0; i < bytes_read; i++) {
                    if (file_buffer[i] == '\n') {
                        print_char('\n');
                    } else if (file_buffer[i] >= 32 && file_buffer[i] < 127) {
                        print_char(file_buffer[i]);
                    }
                }
                print_string("\n---\n");
                check_pagination();
            } else {
                print_string("FAT: Failed to read file\n");
            }
        } else {
            print_string("FAT: Could not open testfile.txt\n");
        }
    } else {
        print_string("FAT: Failed to initialize filesystem\n");
    }
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
