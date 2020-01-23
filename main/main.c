/* main.c by robin prillwitz 2020 */

#include "definitions.h"

/* --------------------------------- Headers -------------------------------- */

esp_err_t initilize_i2c();
esp_err_t write_i2c(uint8_t, uint8_t *, size_t);
void update_state(void*);
void update_supervisor_task(void*);
void identify(homekit_value_t);
homekit_value_t get(homekit_characteristic_t*);
void set(homekit_characteristic_t*, homekit_value_t);
nvs_handle_t nvs_user_open();
void nvs_read(void*);
void nvs_write(void*);
void on_wifi_ready();
void on_led_ready();
void app_main(void);

/* -------------------------------- Variables ------------------------------- */

homekit_characteristic_t target_position;
homekit_characteristic_t current_position;
homekit_characteristic_t position_state;

/* ----------------------------------- I2C ---------------------------------- */

esp_err_t initilize_i2c()   {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .master.clk_speed = CONFIG_CLK_SPEED,
        .sda_io_num = CONFIG_SDA,
        .scl_io_num = CONFIG_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false
    };

    ESP_LOGI("I2C", "Initilizing Interface");

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &config));
    return i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0); // last arguments for slave only
}

esp_err_t write_i2c(uint8_t addr, uint8_t *data_wr, size_t size)    {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 500 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ------------------------------ Update Loops ------------------------------ */

void resume_update()    {
    shutdown_timer = 0;
    vTaskResume(updateTask);

    if(display_off) {
        fade_in_l();
        display_off = false;
    }
}

void update_state(void*_args)  {
    while(1)    {
        if(current < target)    {
            current++;
        }   else if(current > target)   {
            current--;
        }

        purge_buffer_l();

        if(current >= 96)   {
            set_bar_l( (uint8_t)(current / 4.166F), red, red, true);
            set_attenuator_a(current, false);
        }   else if(current == 0)   {
            set_bar_l(1, green, off, true);
            set_attenuator_a(1, true);
        }   else    {
            set_bar_l( (uint8_t)(current / 4.166F), green, yellow, true);
            set_attenuator_a(current, false);
        }

        if(current == target)   {
            // if(current == 0)    {
            //     muted = true;
            // }
            homekit_characteristic_notify(&position_state, HOMEKIT_UINT8(POSITION_STATE_STOPPED));
            ESP_LOGI(TAG_a, "Attenuation at -%ddB",
                (uint8_t) MAX(0, MIN(79, roundf(79.0F - 39.6F * log10f(current) ) ) ) );
            vTaskSuspend(updateTask);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void update_supervisor_task(void*_args) {
    while(1)    {
        shutdown_timer = 0;
        for(; shutdown_timer <= 15; shutdown_timer++) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            homekit_characteristic_notify(&current_position, HOMEKIT_UINT8(current));
            homekit_characteristic_notify(&target_position, HOMEKIT_UINT8(target));
            // ESP_LOGI("TIMER", "%d", shutdown_timer);
        }

        if(display_off == false)    {
            ESP_LOGI("UDS", "Suspending Update Task");
            vTaskSuspend(updateTask);
            xTaskCreate(nvs_write, "NVS write", 4096, NULL, 2, NULL);
            fade_out_l();
            // vTaskDelay(400 / portTICK_PERIOD_MS);
            // set_display_l(false, 0x00);
            display_off = true;
        }
    }
}

/* --------------------------------- Homekit -------------------------------- */

void identify(homekit_value_t _value)   {
    ESP_LOGI("HK", "Identify");
    set_display_l(true, 1);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    set_display_l(true, 0);
}

homekit_value_t get(homekit_characteristic_t* ctx)  {
    ESP_LOGI("HK", "Get");
       if(ctx->type == HOMEKIT_CHARACTERISTIC_CURRENT_POSITION) {
                return HOMEKIT_UINT8(current);
       } else if(ctx->type == HOMEKIT_CHARACTERISTIC_TARGET_POSITION)    {
                return HOMEKIT_UINT8(target);
       } else if(ctx->type == HOMEKIT_CHARACTERISTIC_POSITION_STATE)    {
                return HOMEKIT_UINT8(state);
        }
    return HOMEKIT_NULL(NULL);
}

void set(homekit_characteristic_t* ctx, homekit_value_t value)  {
    if (value.format != homekit_format_uint8)   {
        ESP_LOGE("HK", "Invalid Value format: %i for %s", value.format, ctx->description);
    }   else    {
        if(ctx->type == HOMEKIT_CHARACTERISTIC_CURRENT_POSITION) {
            // current = value.uint8_value;
            ESP_LOGE("HK", "(tried to) set Current (?): %d to %d", current, value.uint8_value);
        } else if(ctx->type == HOMEKIT_CHARACTERISTIC_TARGET_POSITION)    {
            target = MAX(MIN(value.uint8_value, 100), 0);
            ESP_LOGI("HK", "Set Target: %d", target);
            if(target == 0) {
                homekit_characteristic_notify(&position_state, HOMEKIT_UINT8(POSITION_STATE_CLOSING));
            }   else if(target == 100)  {
                homekit_characteristic_notify(&position_state, HOMEKIT_UINT8(POSITION_STATE_OPENING));
            }
        } else if(ctx->type == HOMEKIT_CHARACTERISTIC_POSITION_STATE)    {
            state = value.uint8_value;
            ESP_LOGI("HK", "Set State: %d", state);
        }

        shutdown_timer = 0;
        vTaskResume(updateTask);
        if(display_off) {
            // set_display_l(true, 0x00);
            fade_in_l();
            display_off = false;
        }
    }
}

homekit_characteristic_t target_position = {
    HOMEKIT_DECLARE_CHARACTERISTIC_TARGET_POSITION(0, .getter_ex = get, .setter_ex = set)
};
homekit_characteristic_t current_position = {
    HOMEKIT_DECLARE_CHARACTERISTIC_CURRENT_POSITION(0, .getter_ex = get, .setter_ex = set)
};
homekit_characteristic_t position_state = {
    HOMEKIT_DECLARE_CHARACTERISTIC_POSITION_STATE(POSITION_STATE_STOPPED, .getter_ex = get, .setter_ex = set)
};

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_speaker, .services = (homekit_service_t *[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t *[]){
            HOMEKIT_CHARACTERISTIC(NAME, CONFIG_DEVICE_NAME),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "robin"),
            HOMEKIT_CHARACTERISTIC(MODEL, "none"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        HOMEKIT_SERVICE(DOOR, .primary = true, .characteristics = (homekit_characteristic_t *[]){
            HOMEKIT_CHARACTERISTIC(NAME, CONFIG_DEVICE_NAME),
            &target_position,
            &current_position,
            &position_state,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = CONFIG_PASSWD,
    .setupId = "1QJ8"
};

/* ----------------------------------- NVS ---------------------------------- */

nvs_handle_t nvs_user_open()    {
    ESP_LOGI("NVS", "Opening NVS");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK)  {
        ESP_LOGE("NVS", "Error with opening NVS handle: %s", esp_err_to_name(err));
        return (nvs_handle_t)NULL;
    }   else    {
        return nvs_handle;
    }
}

void nvs_read(void*_args)   {
    nvs_handle_t nvs_handle = nvs_user_open();
    if(nvs_handle != (nvs_handle_t)NULL)  {
        ESP_LOGI("NVS", "Reading from NVS");

        esp_err_t err = nvs_get_u8(nvs_handle, "target", &target);

        switch (err) {
            case ESP_OK:
                ESP_LOGI("NVS", "Sucessfully loaded target Value");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE("NVS", "Value is not initilized");
                target = 0;
                break;
            default :
                ESP_LOGE("NVS", "Errror reading: %s",  esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    }

    vTaskDelete(NULL);
}

void nvs_write(void*_args)    {
    esp_err_t err;
    nvs_handle_t nvs_handle = nvs_user_open();
    if(nvs_handle != (nvs_handle_t)NULL)  {

        ESP_LOGI("NVS", "Writing to NVS");
        err = nvs_set_u8(nvs_handle, "target", target);
        if(err != ESP_OK)   {
            ESP_LOGE("NVS", "Failed writing to NVS: %s", esp_err_to_name(err));
        }   else    {
            ESP_LOGI("NVS", "Done writing to NVS");
        }

        ESP_LOGI("NVS", "Commiting changes");
        err = nvs_commit(nvs_handle);
        if(err != ESP_OK)   {
            ESP_LOGE("NVS", "Failed commiting to NVS: %s", esp_err_to_name(err));
        }   else    {
            ESP_LOGI("NVS", "Done commiting to NVS");
        }
        nvs_close(nvs_handle);
    }

    vTaskDelete(NULL);
}

/* -------------------------------- Callbacks ------------------------------- */

void on_wifi_ready()    {
    ESP_LOGI(TAG_w, "Connection Online, starting homekit");
    homekit_server_init(&config);
}

void on_led_ready() {
    ESP_LOGI(TAG_l, "LED Connection Ready, starting update tasks");
    xTaskCreate(update_state, "Update", 4096, NULL, 3 , &updateTask);
    xTaskCreate(update_supervisor_task, "Update_Shutdown", 4096, NULL, 2 , NULL);
}

/* ---------------------------------- Main ---------------------------------- */

void app_main(void) {

    shutdown_timer = 0;
    current = 0;
    target = 0;
    pre_mute = 0;

    state = 2;
    muted = false;
    display_off = false;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(initilize_i2c());
    xTaskCreate(nvs_read, "NVS read", 2048, NULL, 2, NULL);

    initilize_a();
    void (*callback)() = &on_led_ready;
    initilize_l(callback);

    wifi_init();

    ESP_LOGI(TAG_i, "Starting IR Task");
    xTaskCreate(ir_rx_task, "ir_rx_task", 4096, NULL, 10, NULL);
}
