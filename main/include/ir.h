#ifndef IR_H
#define IR_H
#pragma once

#define TAG_i "IR"

#include "definitions.h"

#include "driver/rmt.h"
#include "ir_tools.h"

void ir_rx_task(void*);
void decode_ir(uint32_t*, bool);

#endif
