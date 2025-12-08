#include <stdint.h>

// Forward declare print functions from kernel_main
extern void print_string(char *s);
extern void print_char(char c);
extern void print_hex8(uint8_t b);
extern uint8_t inb(uint16_t _port);

int eric_main(void) {
    char input[100];
    int input_idx = 0;

    print_string("Enter something: ");

    // Simple keyboard input loop - read characters until Enter is pressed
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 1) {
            uint8_t scancode = inb(0x60);
            
            // Scancode 0x1C = Enter key
            if (scancode == 0x1C) {
                input[input_idx] = '\0';
                print_char('\n');
                break;
            }
            // Scancode 0x0E = Backspace
            else if (scancode == 0x0E && input_idx > 0) {
                input_idx--;
                print_string("\b \b");  // backspace, space, backspace
            }
            // Regular character keys (simplified - just storing scancodes as visual feedback)
            else if (input_idx < sizeof(input) - 1 && scancode < 0x3A) {
                input[input_idx++] = '?';  // Placeholder for visualization
                print_char('*');
            }
        }
    }

    // Convert to uppercase
    for (int i = 0; input[i]; i++) {
        if (input[i] >= 'a' && input[i] <= 'z') {
            input[i] = input[i] - ('a' - 'A');
        }
    }

    print_string("You typed: ");
    print_string(input);
    print_char('\n');
    
    return 0;
}