#pragma once

#include <Tactility/hal/i2c/I2cDevice.h>

#define MAX17048_ADDRESS 0x36

#define MAX17048_VCELL_REG 0x02    /*!< ADC measurement of VCELL. */
#define MAX17048_SOC_REG 0x04      /*!< Battery state of charge. */
#define MAX17048_MODE_REG 0x06     /*!< Initiates quick-start, reports hibernate mode, and enables sleep mode. */
#define MAX17048_VERSION_REG 0x08  /*!< IC production version. */
#define MAX17048_HIBRT_REG 0x0A    /*!< Controls thresholds for entering and exiting hibernate mode. */
#define MAX17048_CONFIG_REG 0x0C   /*!< Compensation to optimize performance, sleep mode, alert indicators, and configuration. */
#define MAX17048_VALERT_REG 0x14   /*!< Configures the VCELL range outside of which alerts are generated. */
#define MAX17048_CRATE_REG 0x16    /*!< Approximate charge or discharge rate of the battery. */
#define MAX17048_VRESET_REG 0x18   /*!< Configures VCELL threshold below which the IC resets itself, ID is a one-time factory-programmable identifier. */
#define MAX17048_CHIPID_REG 0x19   /*!< Register that holds semi-unique chip ID. */
#define MAX17048_STATUS_REG 0x1A   /*!< Indicates overvoltage, undervoltage, SOC change, SOC low, and reset alerts. */
#define MAX17048_CMD_REG 0xFE      /*!< Sends POR command. */

class Max17048 final : public tt::hal::i2c::I2cDevice {

public:

    enum ChargeStatus {
        CHARGE_STATUS_CHARGING = 0b01,
        CHARGE_STATUS_DISCHARGING = 0b10,
        CHARGE_STATUS_STANDBY = 0b00
    };

    explicit Max17048(i2c_port_t port) : I2cDevice(port, MAX17048_ADDRESS) {}

    std::string getName() const final { return "MAX17048"; }
    std::string getDescription() const final { return "Power management with I2C interface."; }

    bool setRegisters(uint8_t* bytePairs, size_t bytePairsSize) const;

    bool getBatteryVoltage(float& vbatMillis) const;
    bool getChargeStatus(ChargeStatus& status) const;
};
