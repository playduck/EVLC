#ifndef LED_BAR_H
#define LED_BAR_H
#pragma once

#include "definitions.h"

#define ACK_CHECK_EN 0x01
#define TAG_l "I2C_LED"
#define ADDR_l 0x70
#define DISP_ADDR_l 0x00 // Display Data Address pointer
#define SYS_SET_l 0x20 // System Setup
#define KEY_DAT_l 0x40 // Key Data
#define INT_ADDR_l 0x60 // Int flags
#define DISP_SET_l 0x80 // Display Setup
#define ROW_INT_l 0xA0 // ROW / INT Selector
#define DIM_l 0xE0 // Dimming table
#define TEST_l 0xD9 // internal test

// sets led brightness (0-15)
void set_brightness_l(uint8_t);

// sets display on/off and  blink frequency (0-3)
void set_display_l(bool, uint8_t);

// sets led with color
uint16_t* set_led_l(uint16_t[8], uint8_t, color_t);

// draws a bar to first arg with colors and updates leds
void set_bar_l(uint8_t, color_t, color_t, bool);

// overwrites the entire led driver memory
void set_all_led_l(uint16_t[8]);

// fancy startup animation after debug blink
void intro_animation_l();

// lerp brightness from 15 to 0
void fade_out_l();

// lerp brightness from 0 to 15
void fade_in_l();

// resets the internal display buffer
void purge_buffer_l();

// initilizes the led driver, takes callback fn
void initilize_l(void (*)());

#endif
