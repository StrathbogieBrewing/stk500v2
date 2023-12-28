#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "stk500v2.h"

int main(void) {
    board_init();
    stdio_init_all();

    printf("Starting STK500V2\n");
    stk500v2_init();
    while (1) {
        stk500v2_update();
    }
    
    return 0;
}
