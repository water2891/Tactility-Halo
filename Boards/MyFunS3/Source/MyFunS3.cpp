#include "MyFunS3Constants.h"
#include "MyFunS3.h"
#include "InitBoot.h"
#include "Tactility/lvgl/LvglSync.h"
#include "hal/MyFunS3Display.h"
#include "hal/MyFunS3Power.h"
#include "hal/MyFunS3SdCard.h"
#include "hal/MyFunS3VKeyboard.h"

#include <Tactility/hal/Configuration.h>



const tt::hal::Configuration myfun_s3 = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    //.createKeyboard = createKeyboard,
    //.sdcard = createSdCard(),
    //.power = createPower,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Main",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = I2C_SDA_IO_NUM,
                .scl_io_num = I2C_SCL_IO_NUM,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    },
    .spi {
        tt::hal::spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = SPI_MOSI_IO_NUM,
                .miso_io_num = SPI_MISO_IO_NUM,
                .sclk_io_num = SPI_SCLK_IO_NUM,
                .data2_io_num = GPIO_NUM_NC,
                .data3_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = SPI_TRANSACTION_SIZE,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    }
};
