#ifndef AUDIO_H
#define AUDIO_H
#pragma once

#include "definitions.h"

#define TAG_a "AUDIO"
#define ADDR_a 0x88
#define CLEAR_REG 0xF0
#define MUTE_ST 0x74
#define ATT_T_ST 0xE0
#define ATT_S_ST 0xD0

void set_attenuator_a(uint8_t, bool);
void initilize_a();

#endif
