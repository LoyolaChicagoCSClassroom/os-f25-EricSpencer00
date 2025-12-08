/* Minimal kernel shell (runs inside kernel) */
#include <stdint.h>
#include <stddef.h>
#include "fat.h"

/* I/O helpers from kernel */
extern uint8_t inb(uint16_t _port);
extern void print_string(char *s);
extern void print_char(char c);
/* string helpers provided by kernel_main.c */
extern int strcasecmp(const char *s1, const char *s2);

/* simple scancode->ASCII table (set 1) for common keys */
static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-', '=', '\b', /* backspace */
    '\t', /* tab */
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', /* enter */
    0, /* ctrl */
    'a','s','d','f','g','h','j','k','l',';','\'', '`',
    0, /* left shift */
    '\\','z','x','c','v','b','n','m',',','.','/', 0, /* right shift */
    '*', 0, /* alt */
    ' ', /* space */
};

/* Read a line from keyboard using port 0x60 polling. Returns length. */
int kernel_readline(char *buf, int maxlen) {
    int len = 0;
    while (1) {
        uint8_t status = inb(0x64);
        if (status & 1) {
            uint8_t sc = inb(0x60);
            if (sc == 0) continue;
            /* key release events have high bit set (>=0x80) */
            if (sc & 0x80) continue;
            if (sc == 0x1C) { /* Enter */
                print_char('\n');
                buf[len] = '\0';
                return len;
            } else if (sc == 0x0E) { /* Backspace */
                if (len > 0) {
                    len--;
                    print_string("\b \b");
                }
            } else {
                char c = (sc < 128) ? scancode_map[sc] : 0;
                if (c) {
                    if (len + 1 < maxlen) {
                        buf[len++] = c;
                        print_char(c);
                    }
                }
            }
        }
    }
}

/* Simple tokenizer: split on spaces, return argc and fill argv pointers */
int kernel_tokenize(char *line, char **argv, int maxargs) {
    int argc = 0;
    char *p = line;
    while (*p && argc < maxargs) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') { *p = '\0'; p++; }
    }
    argv[argc] = NULL;
    return argc;
}

/* Kernel shell main loop */
void nsh_kernel_run(void) {
    char line[256];
    char *argv[16];
    print_string("nsh> ");
    while (1) {
        int len = kernel_readline(line, sizeof(line));
        if (len <= 0) { print_string("nsh> "); continue; }
        int argc = kernel_tokenize(line, argv, 16);
        if (argc == 0) { print_string("nsh> "); continue; }
        if (argv[0][0] == '\0') { print_string("nsh> "); continue; }

        if (/* builtins */ 0) {}

        if (strcasecmp(argv[0], "help") == 0) {
            print_string("Commands: help, cat <file>, exit\n");
        } else if (strcasecmp(argv[0], "exit") == 0) {
            print_string("Exiting shell\n");
            return;
        } else if (strcasecmp(argv[0], "cat") == 0) {
            if (argc < 2) { print_string("Usage: cat <filename>\n"); }
            else {
                fat_file_t f;
                if (fatOpen(argv[1], &f) == 0) {
                    char buf[512];
                    int n = fatRead(&f, buf, sizeof(buf));
                    if (n > 0) {
                        for (int i = 0; i < n; i++) {
                            char c = buf[i];
                            if (c >= 32 || c == '\n' || c == '\r' || c == '\t') print_char(c);
                        }
                        print_char('\n');
                    } else {
                        print_string("(empty)\n");
                    }
                } else {
                    print_string("File not found\n");
                }
            }
        } else {
            print_string("Unknown command. Type 'help'.\n");
        }

        print_string("nsh> ");
    }
}
