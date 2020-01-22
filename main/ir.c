// #include "definitions.h"
#include "ir.h"

static rmt_channel_t rx_channel = RMT_CHANNEL_1;

void ir_rx_task(void *arg)  {
// while(1)    {
    uint32_t addr = 0;
    uint32_t cmd = 0;
    uint32_t length = 0;
    bool repeat = false;
    RingbufHandle_t rb = NULL;
    rmt_item32_t *items = NULL;

    rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(CONFIG_RMT_RX_GPIO, rx_channel);
    rmt_config(&rmt_rx_config);
    rmt_driver_install(rx_channel, 1000, 0);
    ir_parser_config_t ir_parser_config = IR_PARSER_DEFAULT_CONFIG((ir_dev_t)rx_channel);
    ir_parser_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)
    ir_parser_t *ir_parser = NULL;
    // ir_parser = ir_parser_rmt_new_nec(&ir_parser_config);
    ir_parser = ir_parser_rmt_new_rc5(&ir_parser_config);

    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(rx_channel, &rb);
    // Start receive
    rmt_rx_start(rx_channel, true);
    while(1)    {
    // while (rb) {
        items = (rmt_item32_t *) xRingbufferReceive(rb, &length, 1000);
        if (items) {
            length /= 4; // one RMT = 4 Bytes
            if (ir_parser->input(ir_parser, items, length) == ESP_OK) {
                if (ir_parser->get_scan_code(ir_parser, &addr, &cmd, &repeat) == ESP_OK) {
                    // ESP_LOGI(TAG_i, "Scan Code %s --- addr: 0x%04x cmd: 0x%04x", repeat ? "(repeat)" : "", addr, cmd);
                    decode_ir(&cmd, repeat);
                }
            }
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void *) items);
        } else {
            // break;
            vTaskDelay(1200 / portTICK_PERIOD_MS);
        }
    }
    ir_parser->del(ir_parser);
    rmt_driver_uninstall(rx_channel);
    // }
    // vTaskDelete(NULL);
}

int8_t quantize(int8_t val)   {
    // return (val - (val % 5) );
    return  MIN(MAX(( (val + 2) / 5) * 5, 0), 100);
}

void decode_ir(uint32_t *cmd, bool repeat)    {
    switch(*cmd) {
        case 0x0010:    // vol up
            if(repeat == false)    {
                target = quantize(target+5);
            }   else    {
                target = quantize(target+10);
            }
            muted = false;
            ESP_LOGI(TAG_i, "Vol up (%d)", target);
            break;
        case 0x0011:    // vol down
            if(repeat == false)    {
                target = quantize(target-5);
            }   else    {
                target = quantize(target-10);
            }
            muted = false;
            ESP_LOGI(TAG_i, "Vol down (%d)", target);
            break;
        case 0x000d:    // mute
            if(repeat == false) {
                ESP_LOGI(TAG_i, "Mute");
                if(muted)   {
                    target = pre_mute;
                }   else    {
                    pre_mute = target;
                    target = 0;
                }
                muted = !muted;
            }
            break;
        case 0x0037:    // red
            ESP_LOGW("DEBUG", "Restarting!\n");
            esp_restart();
            // longjmp(buf, 0);
            return;
        case 0x0036:    // green
            ESP_LOGW("DEBUG", "Forcing shutdown task");
            current = target;
            display_off = false;
            shutdown_timer = 15;
            return;
        case 0x0032:    // yellow
            ESP_LOGW("DEBUG", "Intro Animation");
            if(display_off) {
                fade_in_l();
            }
            purge_buffer_l();
            intro_animation_l();
            vTaskDelay(600 / portTICK_PERIOD_MS);
            purge_buffer_l();
            break;
        case 0x0034:    // blue
            ; // empty statement, 'cause C
            char pcWriteBuffer[1024] = "";
            vTaskGetRunTimeStats(( char *)pcWriteBuffer);

            ESP_LOGW("DEBUG", "\nIDF-Version: %s\nHeap: %u\nMin Heap: %u\nCPU: %u\n",
                esp_get_idf_version(), esp_get_free_heap_size(),
                esp_get_minimum_free_heap_size(), esp_cpu_get_ccount() );
            ESP_LOGW("DEBUG", "%s\n", pcWriteBuffer);
            return;
        default:
            return;
    }
    resume_update();
}
