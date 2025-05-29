#pragma once

#include <Tactility/hal/keyboard/KeyboardDevice.h>
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_touch.h>
#include <Qmi8658a.h>

class MyFunS3VKeyboard : public tt::hal::keyboard::KeyboardDevice {

private:

    lv_indev_t* _Nullable deviceHandle = nullptr;
    lv_obj_t* _Nullable cursor_obj = nullptr;
    t_sQMI8658 QMI8658;

public:

    std::string getName() const final { return "IMU VKeyboard"; }
    std::string getDescription() const final { return "用6D姿态传感器模拟的虚拟键盘"; }

    bool start(lv_display_t* display) override;
    bool stop() override;
    bool isAttached() const override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};

std::shared_ptr<tt::hal::keyboard::KeyboardDevice> createKeyboard();
