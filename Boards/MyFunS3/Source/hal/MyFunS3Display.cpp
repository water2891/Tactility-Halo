#include <Tactility/Log.h>

#include "../MyFunS3Constants.h"

#include "MyFunS3Display.h"

#include <Tca6408a.h>
#include <Cst816Touch.h>
#include <PwmBacklight.h>
#include <Gc9a01Display.h>

#include <driver/spi_master.h>

#define TAG "myfuns3_display"

extern std::shared_ptr<Tca6408a> tca6408a;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        LCD_TOUCH_I2C_HOST,
        240,
        240,
        LCD_SWAPXY,
        !LCD_MIRRORX,
        LCD_MIRRORY,
        GPIO_NUM_NC,
        LCD_TOUCH_INT,
        0,
        1
    );

    return std::make_shared<Cst816sTouch>(std::move(configuration));
    // return nullptr;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    if (tca6408a == nullptr) {
        TT_LOG_E(TAG, "TCA6408A not found");
    }

    // GC9A01 屏幕旋转设置：
    // 0°: false, true, false;
    // 90°: true, false, false;
    // 180°: false, false, true;
    // 270°: true, true, true;
    auto configuration = std::make_unique<Gc9a01Display::Configuration>(
        SPI_MAIN_HOST,
        LCD_CS_IO_NUM,
        LCD_DC_IO_NUM,
        LCD_RESET_IO_NUM,
        LCD_HORIZONTAL_RESOLUTION,
        LCD_VERTICAL_RESOLUTION,
        touch,
        LCD_SWAPXY,
        LCD_MIRRORX,
        LCD_MIRRORY,
        true,   // invertColor
        LCD_DRAW_BUFFER_SIZE,
        true // use spiram
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    configuration->beforeInitFunction = []() {
        TT_LOG_I(TAG, "把CS引脚置低电平");
        tca6408a->setIOLevel(0, 0);
    };

    return std::make_shared<Gc9a01Display>(std::move(configuration));
}
