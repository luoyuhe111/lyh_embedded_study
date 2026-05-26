#include <stdint.h>
#include "console.h"

#define APP_BASE_ADDRESS  0x08010000
//0x08010000->sp
//0x08010004->pc

extern void board_lowlevel_init(void);
extern void bootloader_main(void);

int main(void)
{
    board_lowlevel_init();
    console_init();
    bootloader_main();
    
//    extern void JumpApp(uint32_t base);
//    JumpApp(APP_BASE_ADDRESS);

    return 0;
}

