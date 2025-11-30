// PS/2 Keyboard + Serial Input Driver
#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"

// PS/2 Controller ports
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

// Serial port (COM1) constants
#define COM1_BASE        0x3F8
#define COM1_DATA        (COM1_BASE + 0)  // Data register
#define COM1_IER         (COM1_BASE + 1)  // Interrupt Enable Register
#define COM1_IIR         (COM1_BASE + 2)  // Interrupt ID Register
#define COM1_LCR         (COM1_BASE + 3)  // Line Control Register
#define COM1_MCR         (COM1_BASE + 4)  // Modem Control Register
#define COM1_LSR         (COM1_BASE + 5)  // Line Status Register

// I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Keyboard scan code to ASCII translation table
static const char scancode_to_ascii[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',  // 0x00-0x07
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t', // 0x08-0x0F
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10-0x17
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',  // 0x18-0x1F
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20-0x27
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',  // 0x28-0x2F
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,    0,    // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    '7',  // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',  // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0     // 0x58-0x5F
};

// Shift + key translations for common keys
static const char shift_scancode_to_ascii[128] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  // 0x00-0x07
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t', // 0x08-0x0F
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10-0x17
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',  // 0x18-0x1F
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20-0x27
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',  // 0x28-0x2F
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,    0,    // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    '7',  // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',  // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0     // 0x58-0x5F
};

// PS/2 Keyboard buffer (circular)
static char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;
static volatile int kb_count = 0;

// Serial input buffer (circular)
static char serial_buffer[KB_BUFFER_SIZE];
static volatile int serial_head = 0;
static volatile int serial_tail = 0;
static volatile int serial_count = 0;

// Modifier key states (PS/2)
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

// Serial debug output
static int serial_can_tx(void) { return (inb(0x3F8 + 5) & 0x20) != 0; }
static void serial_putc(char c) { while (!serial_can_tx()) {} outb(0x3F8, (uint8_t)c); }
static void serial_write(const char* s) { for (; *s; ++s) serial_putc(*s); }

// PS/2 buffer management
void kb_buffer_put(char c) {
    if (kb_count >= KB_BUFFER_SIZE) {
        // Buffer full, drop oldest character
        kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
        kb_count--;
    }
    kb_buffer[kb_head] = c;
    kb_head = (kb_head + 1) % KB_BUFFER_SIZE;
    kb_count++;
}

char kb_buffer_get(void) {
    if (kb_count == 0) return 0;
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    kb_count--;
    return c;
}

// Serial buffer management
void serial_buffer_put(char c) {
    if (serial_count >= KB_BUFFER_SIZE) {
        // Buffer full, drop oldest character
        serial_tail = (serial_tail + 1) % KB_BUFFER_SIZE;
        serial_count--;
    }
    serial_buffer[serial_head] = c;
    serial_head = (serial_head + 1) % KB_BUFFER_SIZE;
    serial_count++;
}

char serial_buffer_get(void) {
    if (serial_count == 0) return 0;
    char c = serial_buffer[serial_tail];
    serial_tail = (serial_tail + 1) % KB_BUFFER_SIZE;
    serial_count--;
    return c;
}

// PS/2 keyboard interrupt handler (called from IRQ1 ISR)
void keyboard_irq_handler(void) {
    // Debug: show that IRQ1 fired
    serial_write("[kbd] IRQ1 fired\n");
    
    uint8_t scancode = inb(PS2_DATA_PORT);
    
    // Debug: show raw scancode
    serial_write("[kbd] scancode: 0x");
    // Simple hex output
    char hex[] = "0123456789ABCDEF";
    serial_putc(hex[(scancode >> 4) & 0xF]);
    serial_putc(hex[scancode & 0xF]);
    serial_write("\n");
    
    // Handle key release (top bit set)
    if (scancode & 0x80) {
        scancode &= 0x7F; // Remove release bit
        // Handle modifier key releases
        switch (scancode) {
            case 0x2A: case 0x36: shift_pressed = 0; break; // L/R Shift
            case 0x1D: ctrl_pressed = 0; break;             // Ctrl
            case 0x38: alt_pressed = 0; break;              // Alt
        }
        return;
    }
    
    // Handle modifier key presses
    switch (scancode) {
        case 0x2A: case 0x36: shift_pressed = 1; return; // L/R Shift
        case 0x1D: ctrl_pressed = 1; return;             // Ctrl
        case 0x38: alt_pressed = 1; return;              // Alt
    }
    
    // Convert scancode to ASCII
    char ascii = 0;
    if (scancode < 128) {
        if (shift_pressed) {
            ascii = shift_scancode_to_ascii[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
    }
    
    if (ascii != 0) {
        // Apply Ctrl modifier
        if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 1; // Ctrl+letter = ASCII control code
        } else if (ctrl_pressed && ascii >= 'A' && ascii <= 'Z') {
            ascii = ascii - 'A' + 1;
        }
        
        kb_buffer_put(ascii);
        
        // Debug output
        serial_write("[kbd] key: '");
        if (ascii >= 32 && ascii <= 126) {
            serial_putc(ascii);
        } else {
            serial_write("<ctrl>");
        }
        serial_write("'\n");
    }
}

// PS/2 keyboard initialization
void keyboard_init(void) {
    // Initialize buffer
    kb_head = 0;
    kb_tail = 0;
    kb_count = 0;
    
    // Reset modifier states
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    
    serial_write("[kbd] PS/2 keyboard driver initialized\n");
}

// PS/2 keyboard functions
char keyboard_getchar(void) {
    // Block until we have input
    while (kb_count == 0) {
        // Yield to other tasks while waiting
        extern void yield(void);
        yield();
    }
    
    // Disable interrupts while accessing buffer
    __asm__ __volatile__("cli");
    char c = kb_buffer_get();
    __asm__ __volatile__("sti");
    
    return c;
}

int keyboard_has_input(void) {
    return kb_count > 0;
}

// Serial input initialization
void serial_input_init(void) {
    // Initialize serial buffer
    serial_head = 0;
    serial_tail = 0;
    serial_count = 0;
    
    // Enable receive interrupts on COM1
    outb(COM1_IER, 0x01); // Enable "data available" interrupt
    
    serial_write("[serial] COM1 input driver initialized\n");
}

// Serial interrupt handler (called from IRQ4 ISR)
void serial_irq_handler(void) {
    // Check if data is available
    if (inb(COM1_LSR) & 0x01) {
        char c = inb(COM1_DATA);
        
        // Debug: show received character
        serial_write("[serial] received: '");
        if (c >= 32 && c <= 126) {
            serial_putc(c);
        } else {
            serial_write("<ctrl>");
        }
        serial_write("'\n");
        
        // Add to buffer
        serial_buffer_put(c);
    }
}

// Serial input functions
int serial_has_input(void) {
    return serial_count > 0;
}

char serial_getchar(void) {
    while (serial_count == 0) {
        // Yield to other tasks while waiting
        extern void yield(void);
        yield();
    }
    
    // Disable interrupts while accessing buffer
    __asm__ __volatile__("cli");
    char c = serial_buffer_get();
    __asm__ __volatile__("sti");
    
    return c;
}

// Unified input interface - gets char from any available source
char getchar(void) {
    // Check both PS/2 and serial input, prioritize serial for testing
    while (1) {
        // Poll serial port for input (in case interrupts don't work)
        if (inb(COM1_LSR) & 0x01) {
            char c = inb(COM1_DATA);
            serial_write("[serial] polled: '");
            if (c >= 32 && c <= 126) {
                serial_putc(c);
            } else {
                serial_write("<ctrl>");
            }
            serial_write("'\n");
            return c;  // Return immediately without buffering
        }
        
        // Check buffered input
        if (serial_has_input()) {
            return serial_getchar();
        }
        if (keyboard_has_input()) {
            return keyboard_getchar();
        }
        
        // Yield if no input available
        extern void yield(void);
        yield();
    }
}

// Check if any input source has data
int has_input(void) {
    // Poll serial for input (fallback if interrupts don't work)
    if (inb(COM1_LSR) & 0x01) {
        char c = inb(COM1_DATA);
        serial_write("[serial] polled: '");
        if (c >= 32 && c <= 126) {
            serial_putc(c);
        } else {
            serial_write("<ctrl>");
        }
        serial_write("'\n");
        serial_buffer_put(c);
    }
    
    return serial_has_input() || keyboard_has_input();
}