#include "Tca6408a.h"

bool Tca6408a::setIOConfig(uint8_t config) const{
    return writeRegister8(TCA6408A_CONFIGURATION_PORT, config);
}

bool Tca6408a::setOutputConfig(uint8_t config) const{
    return writeRegister8(TCA6408A_OUTPUT_PORT, config);
}

bool Tca6408a::setIOLevel(int pos , uint8_t level) const{
    if(level == 0){
        return bitOffByIndex(TCA6408A_OUTPUT_PORT, pos);
    }else{
        return bitOnByIndex(TCA6408A_OUTPUT_PORT, pos);
    }
}

uint8_t Tca6408a::getIOLevel(int pos) const{
    uint8_t result;
    if(readRegister8(TCA6408A_INPUT_PORT, result)){
        return (result >> pos) & 0x01;
    }
    return 0;
}