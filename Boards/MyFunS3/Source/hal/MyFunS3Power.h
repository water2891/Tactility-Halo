#pragma once

#include "Tactility/hal/power/PowerDevice.h"
#include <Max17048.h>
#include <memory>

using tt::hal::power::PowerDevice;

class MyFunS3Power : public PowerDevice {

    std::shared_ptr<Max17048> maxDevice;

public:

    explicit MyFunS3Power(std::shared_ptr<Max17048> ma) : maxDevice(std::move(ma)) {}
    ~MyFunS3Power() override = default;

    std::string getName() const final { return "Max17048 Power"; }
    std::string getDescription() const final { return "Power management via I2C"; }

    bool supportsMetric(MetricType type) const override; // 支持哪些指标
    bool getMetric(MetricType type, MetricData& data) override;
};

std::shared_ptr<PowerDevice> createPower();
