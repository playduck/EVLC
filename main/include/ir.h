/* ir.h by robin prillwitz 2020 */

#ifndef IR_H
#define IR_H
#pragma once


#include "definitions.h"

#include "driver/rmt.h"
#include "ir_tools.h"

#define TAG_i "IR"

#define KEY_VOL_UP 0x0010
#define KEY_VOL_DOWN 0x0011
#define KEY_MUTE 0x000d

#define KEY_RED 0x0037
#define KEY_GREEN 0x0036
#define KEY_YELLOW 0x0032
#define KEY_BLUE 0x0034

void ir_rx_task(void*);
void decode_ir(uint32_t*, bool);

#endif
