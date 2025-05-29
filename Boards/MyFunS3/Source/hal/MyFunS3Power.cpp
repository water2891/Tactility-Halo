#include "MyFunS3Power.h"

#include <Tactility/Log.h>

#define TAG "myfuns3_power"

bool MyFunS3Power::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
        case IsCharging:
        case ChargeLevel:
            return true;
        default:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool MyFunS3Power::getMetric(MetricType type, MetricData& data) {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage: {
            float milliVolt;
            if (maxDevice->getBatteryVoltage(milliVolt)) {
                data.valueAsUint32 = (uint32_t)milliVolt;
                return true;
            } else {
                return false;
            }
        }
        case ChargeLevel: {
            float vbatMillis;
            if (maxDevice->getBatteryVoltage(vbatMillis)) {
                float vbat = vbatMillis / 1000.f;
                float max_voltage = 4.20f;
                float min_voltage = 2.69f;
                if (vbat > 2.69f) {
                    float charge_factor = (vbat - min_voltage) / (max_voltage - min_voltage);
                    data.valueAsUint8 = (uint8_t)(charge_factor * 100.f);
                } else {
                    data.valueAsUint8 = 0;
                }
                return true;
            } else {
                return false;
            }
        }
        case IsCharging: {
            Max17048::ChargeStatus status;
            if (maxDevice->getChargeStatus(status)) {
                data.valueAsBool = (status == Max17048::CHARGE_STATUS_CHARGING);
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

static std::shared_ptr<PowerDevice> power;
extern std::shared_ptr<Max17048> max17048;

std::shared_ptr<PowerDevice> createPower() {
    if (power == nullptr) {
        power = std::make_shared<MyFunS3Power>(max17048);
    }

    return power;
}