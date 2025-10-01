
#include <stdint.h>

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

void main() {
    // unsigned short *vram = (unsigned short*)0xb8000; // Base address of video mem
    // const unsigned char color = 7; // gray text on black background

    // while(1) {
    //     uint8_t status = inb(0x64);

    //     if(status & 1) {
    //         uint8_t scancode = inb(0x60);
    //         // echo scancode as hex nibble pairs for visibility
    //     }
    // }
    print_char('E');
    print_char('P');
    print_char('I');
    print_char('C');

    print_string("WE NEED THIS STRING TO BE REALLY REALLY LONG TO TEST OUR NEW LINE\n");

    print_string("TEST");
    print_string("TEST\n");
    print_string("TEST");
    print_string("TEST\n");
    // while(1);
    // while(1);
}
