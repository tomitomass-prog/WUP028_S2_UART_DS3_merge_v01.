#ifndef PAD_GC_H
#define PAD_GC_H
#include <stdbool.h>
#include <stdint.h>
#include "pad_packet.h"
/* WUP-028 port: status, low buttons, high buttons, LX, LY, CX, CY, LT, RT. */
static inline void gc_neutral(uint8_t p[9]) {
    for (int i=0;i<9;i++) p[i]=0;
    p[0]=0x10; /* keep port 1 present, as in proven USB diagnostic */
    p[3]=p[4]=p[5]=p[6]=128;
}
static inline uint8_t gc_axis(int8_t x) { return (uint8_t)((int)x+128); }
static inline void gc_from_pad_packet(const uint8_t f[PAD_PACKET_SIZE], uint8_t p[9]) {
    gc_neutral(p);
    if (!(f[12] & PAD_CONNECTED)) return;
    uint16_t b=(uint16_t)f[3] | ((uint16_t)f[4]<<8);
    uint8_t lo=0, hi=0;
    if (b & PAD_B) lo|=0x01; /* swap Cross/Circle: PAD_B -> GC A */
    if (b & PAD_A) lo|=0x02; /* PAD_A -> GC B */
    if (b & PAD_Y) lo|=0x04; /* swap Square/Triangle: PAD_Y -> GC X */
    if (b & PAD_X) lo|=0x08; /* PAD_X -> GC Y */
    uint8_t hat=f[5];
    if (hat==6 || hat==7 || hat==8) lo|=0x10; /* left */
    if (hat==2 || hat==3 || hat==4) lo|=0x20; /* right */
    if (hat==4 || hat==5 || hat==6) lo|=0x40; /* down */
    if (hat==1 || hat==2 || hat==8) lo|=0x80; /* up */
    if (b & PAD_START) hi|=0x01;
    /* v0.4: swap L1 with L2 and R1 with R2 in digital GC buttons. */
    if (b & (PAD_L | PAD_R)) hi|=0x02; /* L1/R1 -> GC Z */
    if (b & PAD_ZL) hi|=0x08;         /* L2 -> GC L */
    if (b & PAD_ZR) hi|=0x04;         /* R2 -> GC R */
    p[1]=lo; p[2]=hi;
    p[3]=gc_axis((int8_t)f[6]); /* left stick horizontal unchanged */
    /* Invert left Y only; clamp -128 to 255 instead of wrapping to zero. */
    int ly_inv = 128 - (int8_t)f[7];
    p[4]=(uint8_t)(ly_inv > 255 ? 255 : ly_inv);
    p[5]=gc_axis((int8_t)f[8]); p[6]=gc_axis((int8_t)f[9]);
    p[7]=f[10]; p[8]=f[11]; /* analog triggers unchanged */
}
#endif
