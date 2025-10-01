#include <stdint.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BOARD_X 35  // Center the board on 80-char screen
#define BOARD_Y 2

// VGA colors
#define COLOR_BLACK   0x00
#define COLOR_RED     0x04
#define COLOR_GREEN   0x02
#define COLOR_YELLOW  0x06
#define COLOR_BLUE    0x01
#define COLOR_MAGENTA 0x05
#define COLOR_CYAN    0x03
#define COLOR_WHITE   0x07

// Tetris piece types
typedef enum {
    PIECE_I = 0, PIECE_O, PIECE_T, PIECE_S, PIECE_Z, PIECE_J, PIECE_L
} piece_type_t;

// Game state
static uint8_t board[BOARD_HEIGHT][BOARD_WIDTH];
static int current_piece_type;
static int current_piece_x, current_piece_y;
static int current_rotation;
static int score;
static int lines_cleared;
static int level;
static uint32_t tick_counter;
static int game_over;

// Piece definitions (4x4 grid for each rotation)
static const uint8_t pieces[7][4][4][4] = {
    // I piece
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}},
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}}
    },
    // O piece
    {
        {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}}
    },
    // T piece
    {
        {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {0,1,1,0}, {0,1,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {1,1,0,0}, {0,1,0,0}}
    },
    // S piece
    {
        {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,1,0}},
        {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,0,0,0}, {1,1,0,0}, {0,1,0,0}}
    },
    // Z piece
    {
        {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,0,1,0}, {0,1,1,0}, {0,1,0,0}},
        {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {1,1,0,0}, {1,0,0,0}}
    },
    // J piece
    {
        {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {0,1,0,0}, {0,1,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {0,1,0,0}, {1,1,0,0}}
    },
    // L piece
    {
        {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,1,0}},
        {{0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,0,0}}
    }
};

static const uint8_t piece_colors[7] = {
    COLOR_CYAN, COLOR_YELLOW, COLOR_MAGENTA, COLOR_GREEN, 
    COLOR_RED, COLOR_BLUE, COLOR_WHITE
};

// Simple random number generator
static uint32_t rng_state = 1;

uint32_t simple_rand(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return rng_state;
}

// VGA text mode functions
void put_char_at(int x, int y, char c, uint8_t color) {
    uint16_t *vga = (uint16_t*)0xB8000;
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        vga[y * 80 + x] = (color << 8) | c;
    }
}

void clear_screen(void) {
    uint16_t *vga = (uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (COLOR_BLACK << 8) | ' ';
    }
}

void print_string_at(int x, int y, const char *str, uint8_t color) {
    while (*str && x < 80) {
        put_char_at(x++, y, *str++, color);
    }
}

void print_number_at(int x, int y, int num, uint8_t color) {
    char buffer[12];
    int i = 0;
    
    if (num == 0) {
        buffer[i++] = '0';
    } else {
        int temp = num;
        while (temp > 0) {
            buffer[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        // Reverse the string
        for (int j = 0; j < i / 2; j++) {
            char swap = buffer[j];
            buffer[j] = buffer[i - 1 - j];
            buffer[i - 1 - j] = swap;
        }
    }
    buffer[i] = '\0';
    print_string_at(x, y, buffer, color);
}

// Game logic functions
int can_place_piece(int piece_type, int x, int y, int rotation) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (pieces[piece_type][rotation][py][px]) {
                int board_x = x + px;
                int board_y = y + py;
                
                if (board_x < 0 || board_x >= BOARD_WIDTH || 
                    board_y >= BOARD_HEIGHT || 
                    (board_y >= 0 && board[board_y][board_x])) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

void place_piece(int piece_type, int x, int y, int rotation) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (pieces[piece_type][rotation][py][px]) {
                int board_x = x + px;
                int board_y = y + py;
                
                if (board_y >= 0 && board_x >= 0 && board_x < BOARD_WIDTH && board_y < BOARD_HEIGHT) {
                    board[board_y][board_x] = piece_type + 1;
                }
            }
        }
    }
}

int clear_lines(void) {
    int lines_cleared_now = 0;
    
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!board[y][x]) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            // Move all lines above down
            for (int move_y = y; move_y > 0; move_y--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[move_y][x] = board[move_y - 1][x];
                }
            }
            // Clear top line
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[0][x] = 0;
            }
            
            lines_cleared_now++;
            y++; // Check the same line again
        }
    }
    
    return lines_cleared_now;
}

void spawn_new_piece(void) {
    current_piece_type = simple_rand() % 7;
    current_piece_x = BOARD_WIDTH / 2 - 2;
    current_piece_y = 0;
    current_rotation = 0;
    
    if (!can_place_piece(current_piece_type, current_piece_x, current_piece_y, current_rotation)) {
        game_over = 1;
    }
}

void draw_board(void) {
    // Draw border
    for (int y = 0; y < BOARD_HEIGHT + 2; y++) {
        put_char_at(BOARD_X - 1, BOARD_Y + y, '|', COLOR_WHITE);
        put_char_at(BOARD_X + BOARD_WIDTH, BOARD_Y + y, '|', COLOR_WHITE);
    }
    for (int x = 0; x < BOARD_WIDTH + 2; x++) {
        put_char_at(BOARD_X - 1 + x, BOARD_Y + BOARD_HEIGHT, '-', COLOR_WHITE);
    }
    
    // Draw board contents
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            char c = ' ';
            uint8_t color = COLOR_BLACK;
            
            if (board[y][x]) {
                c = '#';
                color = piece_colors[board[y][x] - 1];
            }
            
            put_char_at(BOARD_X + x, BOARD_Y + y, c, color);
        }
    }
    
    // Draw current piece
    if (!game_over) {
        for (int py = 0; py < 4; py++) {
            for (int px = 0; px < 4; px++) {
                if (pieces[current_piece_type][current_rotation][py][px]) {
                    int screen_x = BOARD_X + current_piece_x + px;
                    int screen_y = BOARD_Y + current_piece_y + py;
                    
                    if (screen_y >= BOARD_Y && screen_y < BOARD_Y + BOARD_HEIGHT) {
                        put_char_at(screen_x, screen_y, '#', piece_colors[current_piece_type]);
                    }
                }
            }
        }
    }
}

void draw_ui(void) {
    print_string_at(5, 5, "TETRIS", COLOR_YELLOW);
    print_string_at(5, 7, "Score:", COLOR_WHITE);
    print_number_at(12, 7, score, COLOR_WHITE);
    print_string_at(5, 8, "Lines:", COLOR_WHITE);
    print_number_at(12, 8, lines_cleared, COLOR_WHITE);
    print_string_at(5, 9, "Level:", COLOR_WHITE);
    print_number_at(12, 9, level, COLOR_WHITE);
    
    print_string_at(5, 15, "Controls:", COLOR_GREEN);
    print_string_at(5, 16, "A/D - Move", COLOR_WHITE);
    print_string_at(5, 17, "S - Rotate", COLOR_WHITE);
    print_string_at(5, 18, "W - Drop", COLOR_WHITE);
    print_string_at(5, 19, "Q - Quit", COLOR_WHITE);
    
    if (game_over) {
        print_string_at(BOARD_X - 5, BOARD_Y + BOARD_HEIGHT/2, "GAME OVER!", COLOR_RED);
        print_string_at(BOARD_X - 8, BOARD_Y + BOARD_HEIGHT/2 + 1, "Press R to restart", COLOR_WHITE);
    }
}

void tetris_init(void) {
    // Clear board
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }
    
    score = 0;
    lines_cleared = 0;
    level = 1;
    tick_counter = 0;
    game_over = 0;
    
    spawn_new_piece();
}

void tetris_input(char key) {
    if (game_over) {
        if (key == 'r' || key == 'R') {
            tetris_init();
        }
        return;
    }
    
    switch (key) {
        case 'a':
        case 'A':
            if (can_place_piece(current_piece_type, current_piece_x - 1, current_piece_y, current_rotation)) {
                current_piece_x--;
            }
            break;
            
        case 'd':
        case 'D':
            if (can_place_piece(current_piece_type, current_piece_x + 1, current_piece_y, current_rotation)) {
                current_piece_x++;
            }
            break;
            
        case 's':
        case 'S':
            {
                int new_rotation = (current_rotation + 1) % 4;
                if (can_place_piece(current_piece_type, current_piece_x, current_piece_y, new_rotation)) {
                    current_rotation = new_rotation;
                }
            }
            break;
            
        case 'w':
        case 'W':
            while (can_place_piece(current_piece_type, current_piece_x, current_piece_y + 1, current_rotation)) {
                current_piece_y++;
            }
            break;
    }
}

void tetris_tick(void) {
    if (game_over) return;
    
    tick_counter++;
    
    // Drop piece every (60 - level*5) ticks (minimum 5)
    int drop_rate = 60 - level * 5;
    if (drop_rate < 5) drop_rate = 5;
    
    if (tick_counter >= drop_rate) {
        tick_counter = 0;
        
        if (can_place_piece(current_piece_type, current_piece_x, current_piece_y + 1, current_rotation)) {
            current_piece_y++;
        } else {
            // Place piece and spawn new one
            place_piece(current_piece_type, current_piece_x, current_piece_y, current_rotation);
            
            int cleared = clear_lines();
            lines_cleared += cleared;
            score += cleared * 100 * level;
            level = 1 + lines_cleared / 10;
            
            spawn_new_piece();
        }
    }
}

void tetris_update(void) {
    clear_screen();
    draw_board();
    draw_ui();
}

// Export functions for use in kernel_main.c
void start_tetris(void) {
    tetris_init();
}

void tetris_handle_input(char key) {
    tetris_input(key);
}

void tetris_game_tick(void) {
    tetris_tick();
}

void tetris_render(void) {
    tetris_update();
}

int is_tetris_quit(char key) {
    return (key == 'q' || key == 'Q');
}