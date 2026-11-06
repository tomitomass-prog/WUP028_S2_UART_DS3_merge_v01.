
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pad_gc.h"

static void check_btn(uint16_t button, uint8_t lo, uint8_t hi) {
    uint8_t frame[PAD_PACKET_SIZE], gc[9];
    pad_encode(frame, 3, button, 0, 0, 0, 0, 0, 0, 0, true);
    assert(pad_valid(frame));
    gc_from_pad_packet(frame, gc);
    assert(gc[0] == 0x10);
    assert(gc[1] == lo);
    assert(gc[2] == hi);
}

int main(void) {
    uint8_t f[PAD_PACKET_SIZE], gc[9];

    /* Zamiana przyciskow X/O i kwadrat/trojkat. */
    check_btn(PAD_A, 0x02, 0);
    check_btn(PAD_B, 0x01, 0);
    check_btn(PAD_X, 0x08, 0);
    check_btn(PAD_Y, 0x04, 0);

    /* L1, L2, R1, R2 bez zmian. */
    check_btn(PAD_L, 0, 0x08);
    check_btn(PAD_R, 0, 0x04);
    check_btn(PAD_ZL, 0, 0x02);
    check_btn(PAD_ZR, 0, 0x02);
    check_btn(PAD_START, 0, 0x01);

    /* Odwrocona os Y lewego analoga. */
    pad_encode(f, 9, PAD_B | PAD_START | PAD_L,
               1, 20, -80, -30, 40, 70, 90, true);
    gc_from_pad_packet(f, gc);

    assert(gc[1] == 0x81);
    assert(gc[2] == 0x09);
    assert(gc[3] == 148);
    assert(gc[4] == 208);
    assert(gc[5] == 98 && gc[6] == 168);
    assert(gc[7] == 70 && gc[8] == 90);

    pad_encode(f, 10, 0, 0, 0, -128,
               0, 0, 0, 0, true);
    gc_from_pad_packet(f, gc);
    assert(gc[4] == 255);

    f[7] ^= 1;
    assert(!pad_valid(f));

    pad_encode(f, 11, PAD_B, 0, 0, 0, 0,
               0, 0, 0, false);
    gc_from_pad_packet(f, gc);
    assert(gc[1] == 0 && gc[2] == 0);

    puts("v0.3 mapping tests PASS: button swaps, LY invert, triggers unchanged, CRC");
    return 0;
}
