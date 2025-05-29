#include "Gc9a01Display.h"

#include <Tactility/Log.h>

#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_dev.h>

#define TAG "gc9a01"


bool Gc9a01Display::start() {
    TT_LOG_I(TAG, "Starting");

    // gpio_config_t cs_gpio_config = {
    //     .pin_bit_mask = 1ULL << (GPIO_NUM_43),
    //     .mode = GPIO_MODE_OUTPUT
    // };
    // ESP_ERROR_CHECK(gpio_config(&cs_gpio_config));
    // vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms
    // ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_43, 1));
    // vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms
    // TT_LOG_I(TAG, "GPIO[43]: %d", gpio_get_level(GPIO_NUM_43));
    // vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms

    const esp_lcd_panel_io_spi_config_t panel_io_config = {
        .cs_gpio_num = configuration->csPin, // GC9A01设置CS引脚会导致显示异常，所以设置为NC
        .dc_gpio_num = configuration->dcPin,
        .spi_mode = 0,
        .pclk_hz = configuration->pixelClockFrequency,
        .trans_queue_depth = configuration->transactionQueueDepth,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,  
    };

    if (esp_lcd_new_panel_io_spi(configuration->spiBusHandle, &panel_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->resetPin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    if (esp_lcd_new_panel_gc9a01(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    TT_LOG_I(TAG, "Reset");
    if (esp_lcd_panel_reset(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return false;
    }

    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_43, 0));

    // TT_LOG_I(TAG, "GPIO[43]: %d", gpio_get_level(GPIO_NUM_43));
    if (configuration->beforeInitFunction != nullptr) {
        configuration->beforeInitFunction();
    }

    if (esp_lcd_panel_init(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return false;
    }

    //TT_LOG_I(TAG, "invertColor = %s", configuration->invertColor ? "true" : "false");

    if (esp_lcd_panel_invert_color(panelHandle, configuration->invertColor) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to invert");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panelHandle, configuration->swapXY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to swap XY ");
        return false;
    }

    if (esp_lcd_panel_mirror(panelHandle, configuration->mirrorX, configuration->mirrorY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }


    // 分配内存 这里分配了液晶屏一行数据需要的大小
    // uint16_t *buffer = (uint16_t *)heap_caps_malloc(configuration->horizontalResolution * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

    // if (NULL == buffer)
    // {
    //     ESP_LOGE(TAG, "Memory for bitmap is not enough");
    // }
    // else
    // {
    //     for (size_t i = 0; i < configuration->horizontalResolution; i++) // 给缓存中放入颜色数据
    //     {
    //         buffer[i] = 0xffff;
    //     }
    //     for (int y = 0; y < 240; y++) // 显示整屏颜色
    //     {
    //         esp_lcd_panel_draw_bitmap(panelHandle, 0, y, 240, y+1, buffer);
    //     }
    //     free(buffer); // 释放内存
    // }

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    uint32_t buffer_size;
    if (configuration->bufferSize == 0) {
        buffer_size = configuration->horizontalResolution * configuration->verticalResolution / 10;
    } else {
        buffer_size = configuration->bufferSize;
    }

    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        //.control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = false,
        //.trans_size = 0,
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = false,
        .rotation = {
            .swap_xy = configuration->swapXY,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY,
        },
        //.color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            //.sw_rotate = false,
            .swap_bytes = true, // Swap bytes for GC9A01
            //.full_refresh = false,
            //.direct_mode = false
        }
    };

    if(configuration->buffSPIRAM){
        TT_LOG_I(TAG, "Using SPIRAM for buffer");
        disp_cfg.buffer_size = configuration->horizontalResolution * configuration->verticalResolution;
        disp_cfg.trans_size = buffer_size;
        disp_cfg.flags.buff_dma = false;
        disp_cfg.flags.buff_spiram = true;
    }

    displayHandle = lvgl_port_add_disp(&disp_cfg);

    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool Gc9a01Display::stop() {
    assert(displayHandle != nullptr);

    lvgl_port_remove_disp(displayHandle);

    if (esp_lcd_panel_del(panelHandle) != ESP_OK) {
        return false;
    }

    if (esp_lcd_panel_io_del(ioHandle) != ESP_OK) {
        return false;
    }

    displayHandle = nullptr;
    return true;
}

/**
 * Note:
 * The datasheet implies this should work, but it doesn't:
 * https://www.digikey.com/htmldatasheets/production/1640716/0/0/1/ILI9341-Datasheet.pdf
 *
 * This repo claims it only has 1 curve:
 * https://github.com/brucemack/hello-ili9341
 *
 * I'm leaving it in as I'm not sure if it's just my hardware that's problematic.
 */
void Gc9a01Display::setGammaCurve(uint8_t index) {
    uint8_t gamma_curve;
    switch (index) {
        case 0:
            gamma_curve = 0x01;
            break;
        case 1:
            gamma_curve = 0x04;
            break;
        case 2:
            gamma_curve = 0x02;
            break;
        case 3:
            gamma_curve = 0x08;
            break;
        default:
            return;
    }
    const uint8_t param[] = {
        gamma_curve
    };

    if (esp_lcd_panel_io_tx_param(ioHandle , LCD_CMD_GAMSET, param, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma");
    }
}
