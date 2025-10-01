
#include <stdint.h>

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

// Tetris function declarations
void start_tetris(void);
void tetris_handle_input(char key);
void tetris_game_tick(void);
void tetris_render(void);
int is_tetris_quit(char key);

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
    start_tetris(); // Initialize Tetris game
    
    uint8_t previous_scancode = 0;
    
    for (;;) {
        uint8_t scancode = inb(0x60);
        
        if (scancode != previous_scancode && scancode < 0x80) {
            char key = 0;
            
            // Convert scancodes to keys for Tetris
            switch (scancode) {
                case 0x11: key = 'w'; break; // W key - rotate
                case 0x1E: key = 'a'; break; // A key - left
                case 0x1F: key = 's'; break; // S key - down
                case 0x20: key = 'd'; break; // D key - right
                case 0x39: key = ' '; break; // Space - drop
                case 0x01: key = 27; break;  // ESC - quit
            }
            
            if (key) {
                if (is_tetris_quit(key)) {
                    break; // Exit game
                }
                tetris_handle_input(key);
            }
        }
        
        previous_scancode = scancode;
        
        // Game tick for automatic piece dropping
        tetris_game_tick();
        tetris_render();
        
        // Simple delay
        for (volatile int i = 0; i < 1000000; i++);
    }
}
