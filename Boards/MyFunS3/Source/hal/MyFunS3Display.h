#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <esp_lcd_types.h>
#include "driver/gpio.h"
#include <lvgl.h>

class MyFunS3Display : public tt::hal::display::DisplayDevice {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;
    bool poweredOn = true;

public:

    std::string getName() const final { return "GC9A01"; }
    std::string getDescription() const final { return "SPI display"; }

    bool start() override;

    bool stop() override;

    void setPowerOn(bool turnOn) override;
    bool isPoweredOn() const override { return poweredOn; };
    bool supportsPowerControl() const override { return false; }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override { return true; }

    void setGammaCurve(uint8_t index) override;
    uint8_t getGammaCurveCount() const override { return 4; };

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
