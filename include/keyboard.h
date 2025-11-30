#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

// Keyboard driver API
void keyboard_init(void);

// Get next character from keyboard buffer (blocking)
char keyboard_getchar(void);

// Check if keyboard has pending input
int keyboard_has_input(void);

// Keyboard buffer management
#define KB_BUFFER_SIZE 64

// Special key codes
#define KEY_BACKSPACE 0x08
#define KEY_TAB       0x09
#define KEY_ENTER     0x0A
#define KEY_ESC       0x1B
#define KEY_DELETE    0x7F

// Function keys (returned as extended codes)
#define KEY_F1        0x80
#define KEY_F2        0x81
#define KEY_F3        0x82
#define KEY_F4        0x83

// Arrow keys
#define KEY_UP        0x90
#define KEY_DOWN      0x91
#define KEY_LEFT      0x92
#define KEY_RIGHT     0x93

// Serial input functions (COM1)
void serial_input_init(void);
int serial_has_input(void);
char serial_getchar(void);
void serial_irq_handler(void);

// Unified input interface
char getchar(void);  // Gets char from any available input source
int has_input(void); // Checks if any input is available

// Internal keyboard interrupt handler
void keyboard_irq_handler(void);

#endif // KEYBOARD_H