#ifndef DEFINITIONS_H
#define DEFINITIONS_H
#pragma once

#include <stdio.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_spi_flash.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#define POSITION_STATE_CLOSING 0
#define POSITION_STATE_OPENING 1
#define POSITION_STATE_STOPPED 2

#define WRITE_BIT I2C_MASTER_WRITE

#define ABS(a) (a < 0 ? -a : a)
#define MAX(a,b) (a > b ? a : b )
#define MIN(a,b) (a < b ? a : b )
#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif

typedef enum {
    off,
    red,
    green,
    yellow
} color_t;

extern void vTaskGetRunTimeStats( char *pcWriteBuffer );
esp_err_t write_i2c(uint8_t, uint8_t*, size_t);
void resume_update();

TaskHandle_t updateTask;

uint8_t shutdown_timer;
bool display_off;

uint8_t current;
uint8_t state;
uint8_t target;

uint8_t pre_mute;
bool muted;

#include "wifi.h"
#include "led_bar.h"
#include "audio.h"
#include "ir.h"

#endif
