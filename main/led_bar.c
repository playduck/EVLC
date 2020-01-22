// #include "definitions.h"
#include "led_bar.h"

uint16_t buffer[8];

// sets led brightness (0-15)
void set_brightness_l(uint8_t brightness)   {
    ESP_LOGI(TAG_l, "Setting Brightness to %0X (%0d)", brightness, brightness);
    uint8_t br_command = DIM_l | (brightness & 0x0F);
    ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_l, &br_command, sizeof(br_command)));
}

// sets display on/off and  blink frequency (0-3)
void set_display_l(bool on, uint8_t frequency)  {
    ESP_LOGI(TAG_l, "Setting Display %s with Blinking %0X", on ? "on":"off", frequency);
    uint8_t disp_command = DISP_SET_l | ((frequency & 0x03) << 1) | (on ? 0x01 : 0x00);
    ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_l, &disp_command, sizeof(disp_command)));
}

uint16_t* set_led_l(uint16_t data[8], uint8_t val, color_t color)   {
    uint16_t a = 0, c = 0;

    if (val < 12)   {
        c = val / 4;
    }   else    {
        c = (val - 12) / 4;
    }

    a = val % 4;
    if (val >= 12)  {
        a += 4;
    }

    if (color == red)   {
        data[c] |= _BV(a + 8);      // red on
        data[c] &= ~_BV(a);         // green off
    }   else if (color == green)    {
        data[c] |= _BV(a);          // green on
        data[c] &= ~_BV(a + 8);     // red off
    }   else if (color == yellow)   {
        data[c] |= _BV(a) | _BV(a + 8); // red + green
    }   else    {
        data[c] &= ~_BV(a) & ~_BV(a + 8);
    }

    return data;
}

void set_bar_l(uint8_t val, color_t bar, color_t head, bool update)   {
    set_led_l(buffer, val, head);

    for(int j = 0; j < val; j++)    {
        set_led_l(buffer, j, bar);
    }

    if(update)  {
        set_all_led_l(buffer);
    }
}

// overwrites the entire led driver memory
void set_all_led_l(uint16_t data[8])    {
    uint8_t disp_command = 0x00;
    uint8_t data_cmd[17];

    *data_cmd = disp_command;
    for(int i = 0; i < 8; i++)  {
        *(1+data_cmd + 2*i) = (data[i] & 0xFF00) >> 8;
        *(1+data_cmd + 2*i+1) = data[i] & 0x00FF;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_l, data_cmd, 17));
}

void intro_animation_l()  {
    for(uint8_t i = 0; i < 24; i++)  {
        set_bar_l(i, off, off, true);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    for(uint8_t i = 0; i < 24; i++)  {
        set_bar_l(i, red, red, true);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    for(uint8_t i = 0; i < 23; i++)  {
        set_bar_l(i, off, off, false);
        set_bar_l(i, green, green, true);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    for(uint8_t i = 0; i < 22; i++)  {
        set_bar_l(i, yellow, yellow, true);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    for(uint8_t i = 0; i < 24; i++)  {
        set_bar_l(i, off, off, true);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    // vTaskDelay(40/portTICK_PERIOD_MS);

    purge_buffer_l();
}

void fade_out_l()   {
    for(int8_t i = 15; i >= 0; i--)    {
        set_brightness_l(i);
        vTaskDelay(60 / portTICK_PERIOD_MS);
    }
    vTaskDelay(400 / portTICK_PERIOD_MS);
}

void fade_in_l()    {
    for(int8_t i = 0; i <= 15; i++)    {
        set_brightness_l(i);
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    vTaskDelay(400 / portTICK_PERIOD_MS);
}

void purge_buffer_l()  {
    for(uint8_t i = 0; i < 8; i++)  {
        buffer[i] = 0;
    }
}

// initilizes the led driver
void initilize_l(void (*cb)())  {
    // Enable internal Sys Clock
    ESP_LOGI(TAG_l, "Enabeling internal LED System Clock");
    uint8_t setup_cmd = 0x21;
    ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_l, &setup_cmd, sizeof(setup_cmd)));

    // ESP_LOGI(TAG_l, "Configure ROW/INT");
    // uint8_t row_int_setup = ROW_INT_l | (0b10100000);
    // ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_l, I2C_NUM_0, &row_int_setup, sizeof(row_int_setup)));

    purge_buffer_l();

    set_brightness_l(0);
    set_display_l(true, 0x02);

    set_bar_l(24, red, red, true);

    fade_in_l();

    set_display_l(true, 0x00);

    intro_animation_l();
    purge_buffer_l();

    (*cb)();
}
