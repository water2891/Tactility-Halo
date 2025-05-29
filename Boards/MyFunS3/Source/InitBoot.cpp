#include <Max17048.h>
#include <Tca6408a.h>
#include <Qmi8658a.h>
#include <Tactility/Log.h>
#include <Tactility/kernel/Kernel.h>
#include "PwmBacklight.h"
#include <driver/spi_master.h>
#include "iot_button.h"
#include "button_gpio.h"
#include "MyFunS3Constants.h"

extern "C" {

#include "LEDRing/LEDRing.h"

}

#define TAG "myfuns3"

std::shared_ptr<Max17048> max17048;
std::shared_ptr<Tca6408a> tca6408a;
std::shared_ptr<Qmi8658a> qmi8658a;
std::shared_ptr<button_dev_t> bootBtn;
std::shared_ptr<led_strip_handle_t> ledStrip;

led_strip_handle_t led_strip;

static bool initLedRing() {
    TT_LOG_I(TAG, "initLedRing()");

    //led_strip_handle_t led_strip;
    led_ring_init(&led_strip); // 初始化LED灯带

    led_ring_on_config_t led_on_conf = {
        .led_strip = led_strip,
        .led_index = 0,
        .angle = 0,
        .brightness = 100,
        .mode = LED_RING_SNAKE_GRADIENT,
        .colors = {
            {255, 0, 0, 0},
            {0, 255, 0, 1},
        }
    };
    led_ring_on(&led_on_conf); 

    return true;
}

bool initButton(){
    TT_LOG_I(TAG, "initButton()");

    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = GPIO_NUM_0,
        .active_level = 0,
        .enable_power_save = true,
    };
    button_handle_t raw_handle = nullptr;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &raw_handle);
    if(raw_handle == nullptr) {
        TT_LOG_E(TAG, "Button create failed");
    }

    // int short_press_time = 500;
    // iot_button_set_param(raw_handle, BUTTON_SHORT_PRESS_TIME_MS, (void*)&short_press_time);
    // int long_press_time = 2000;
    // iot_button_set_param(raw_handle, BUTTON_LONG_PRESS_TIME_MS, (void*)&long_press_time);

    // 将原始句柄封装为共享指针，并绑定删除函数
    bootBtn = std::shared_ptr<button_dev_t>(raw_handle, [](button_dev_t* btn) {
        iot_button_delete(btn);
    });

    return true;
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot()");

    //initLedRing();

    // max17048 = std::make_shared<Max17048>(I2C_NUM_0);
    // tt::hal::registerDevice(max17048);

    // 6D传感器初始化
    // qmi8658a = std::make_shared<Qmi8658a>(I2C_NUM_0);
    // tt::hal::registerDevice(qmi8658a);
    // if (!qmi8658a->start()) {
    //     TT_LOG_E(TAG, "QMI8658A init failed");
    // }

    // 扩展IO初始化
    tca6408a = std::make_shared<Tca6408a>(LCD_TOUCH_I2C_HOST);
    tt::hal::registerDevice(tca6408a);
    // 写入控制引脚默认值 LCD_CS = 1
    if (!tca6408a->setOutputConfig(0x01))
    {
        TT_LOG_E(TAG, "TCA6408A setOutputConfig failed");
    }
    // 把TCA6408A芯片的IO0设置为输出 其它引脚保持默认的输入
    if(!tca6408a->setIOConfig(0xfE))
    {
        TT_LOG_E(TAG, "TCA6408A setIOConfig failed");
    }

    //vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延时1秒

    //, 5000, LEDC_TIMER_1, LEDC_CHANNEL_0
    if (!driver::pwmbacklight::init(LCD_BACKLIGHT_IO_NUM)) { //GPIO_NUM_21
        TT_LOG_E(TAG, "Backlight init failed");
        return false;
    }

    initButton();



    return true;
}