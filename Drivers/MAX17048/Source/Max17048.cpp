#include "Max17048.h"
#include <Tactility/Log.h>

bool Max17048::getBatteryVoltage(float& vbatMillis) const {
    uint16_t value;
    if (readRegister16(MAX17048_CRATE_REG, value)) {
        vbatMillis = 1.0f * value * 78.125f / 1000.0f;
        return true;
    }else{
        return false;
    }
}

bool Max17048::getChargeStatus(ChargeStatus& status) const {
    uint16_t value;
    if (readRegister16(MAX17048_CRATE_REG, value)) {
        float percent = (float)(value) * 0.208f;
        status = (percent > 0) ? CHARGE_STATUS_CHARGING : ((percent < 0) ? CHARGE_STATUS_DISCHARGING : CHARGE_STATUS_STANDBY);
        return true;
    } else {
        return false;
    }
}

bool Max17048::setRegisters(uint8_t* bytePairs, size_t bytePairsSize) const {
    return tt::hal::i2c::masterWriteRegisterArray(port, address, bytePairs, bytePairsSize, DEFAULT_TIMEOUT);
}
