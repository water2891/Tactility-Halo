#pragma once

#include <Tactility/hal/i2c/I2cDevice.h>

#define TCA6408A_ADDRESS 0x20

#define TCA6408A_INPUT_PORT              0x00
#define TCA6408A_OUTPUT_PORT             0x01
#define TCA6408A_POLARITY_INVERSION_PORT 0x02
#define TCA6408A_CONFIGURATION_PORT      0x03

class Tca6408a : public tt::hal::i2c::I2cDevice {

public:

    explicit Tca6408a(i2c_port_t port) : I2cDevice(port, TCA6408A_ADDRESS) {}

    std::string getName() const final { return "TCA6408A"; }
    std::string getDescription() const final { return "I/O Expander with I2C interface."; }

    bool setIOConfig(uint8_t config) const;
    bool setOutputConfig(uint8_t config) const;
    bool setIOLevel(int pos , uint8_t level) const;
    uint8_t getIOLevel(int pos) const;
};
