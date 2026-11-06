#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pad_gc.h"
int main(void) {
  uint8_t f[PAD_PACKET_SIZE], gc[9];
  pad_encode(f, 9, PAD_A | PAD_START | PAD_L, 1, 0, 0, 0, 0, 0, 0, true);
  assert(pad_valid(f));
  gc_from_pad_packet(f,gc);
  assert(gc[0] == 0x10);
  assert(gc[1] == 0x81); /* A and D-pad UP */
  assert(gc[2] == 0x09); /* START and L */
  assert(gc[3] == 128 && gc[4] == 128);
  f[7] ^= 1;
  assert(!pad_valid(f));
  pad_encode(f, 10, PAD_A, 0, 0, 0, 0, 0, 0, 0, false);
  gc_from_pad_packet(f, gc);
  assert(gc[1] == 0 && gc[2] == 0);
  puts("UART frame CRC + GC A/START/L/D-pad mapping: OK");
  return 0;
}
