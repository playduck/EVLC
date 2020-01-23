/* audio.c by robin prillwitz 2020 */

#include "audio.h"

void set_attenuator_a(uint8_t vol, bool mute)   {
    // vol = (uint8_t)((float)(100.0F - vol) * 0.79F);
    vol = (uint8_t)(MAX(0, MIN(79, roundf(79.0F - 39.6F * log10f(vol)))));

    uint8_t cmd[3];
    cmd[0] = MUTE_ST | (mute | mute << 1);
    cmd[1] = ATT_T_ST | (uint8_t)(vol / 10);
    cmd[2] = ATT_S_ST | (uint8_t)(vol % 10);

    write_i2c(ADDR_a, cmd, 3*sizeof(uint8_t)); // wrap in ESP_ERROR_CHECK
}

void initilize_a()  {
    ESP_LOGI(TAG_a, "Initilizing Audio");

    uint8_t clear_reg = CLEAR_REG;
    ESP_ERROR_CHECK_WITHOUT_ABORT(write_i2c(ADDR_a, &clear_reg, sizeof(clear_reg)));
    set_attenuator_a(0, true);

    ESP_LOGI(TAG_a, "Audio initilized");
}
