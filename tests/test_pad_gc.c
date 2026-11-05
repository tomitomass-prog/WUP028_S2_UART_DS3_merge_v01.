#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "../diagnostics/s2_uart_wup028/pad_gc.h"
int main(void) {
    uint8_t f[PAD_PACKET_SIZE],o[9];
    pad_encode(f,1,PAD_A|PAD_START|PAD_R,1,0,0,0,0,0,0,true);
    assert(pad_valid(f));
    gc_from_pad_packet(f,o);
    assert(o[0]==0x10 && o[1]==0x81 && o[2]==0x05);
    assert(o[3]==128 && o[4]==128);
    f[3]^=1; assert(!pad_valid(f));
    pad_encode(f,2,0,0,0,0,0,0,0,0,false);
    gc_from_pad_packet(f,o);
    assert(o[1]==0 && o[2]==0);
    puts("PASS: UART packet -> GC/WUP-028");
}
