#include "Qmi8658a.h"
#include <Tactility/Log.h>
#include "math.h"
#include <cstdint>

#define TAG "Qmi8658a"

// 初始化qmi8658
bool Qmi8658a::start() const {
    TT_LOG_I(TAG, "QMI8658 Init!");

    uint8_t id = 0; // 芯片的ID号

    readRegister8(QMI8658_WHO_AM_I, id); // 读芯片的ID号
    while (id != 0x05)  // 判断读到的ID号是否是0x05
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延时1秒
        readRegister8(QMI8658_WHO_AM_I, id); // 读取ID号
    }

    //writeRegister8(QMI8658_RESET, 0xb0);  // 复位
    //vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms

    writeRegister8(QMI8658_CTRL1, 0x40); // CTRL1 设置地址自动增加
    writeRegister8(QMI8658_CTRL7, 0x03); // CTRL7 允许加速度和陀螺仪 0x03
    writeRegister8(QMI8658_CTRL5, 0xFF); // CTRL5 开启内部低通滤波器（0x00 关闭,0x99 2.66%, 0xBB 3.63%, 0xBB 5.39%, 0xFF 13.37%)
    writeRegister8(QMI8658_CTRL2, 0x95); // CTRL2 设置ACC 4g 1000Hz
    writeRegister8(QMI8658_CTRL3, 0xd5); // CTRL3 设置GRY 512dps 1000Hz 

    TT_LOG_I(TAG, "QMI8658 OK!");  // 打印信息

    return true;
}

bool Qmi8658a::adjust() const {
    writeRegister8(QMI8658_RESET, 0xb0);  // 复位
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms

    // 按需校正
    writeRegister8(QMI8658_CTRL7, 0x00); // CTRL7 关闭加速度和陀螺仪
    writeRegister8(QMI8658_CTRL9, 0xA2);  // CTRL9 发送CTRL_CMD_ON_DEMAND_CALIBRATION命令
    vTaskDelay(1600 / portTICK_PERIOD_MS); // 延迟1.6秒等待校正完成
    uint8_t status = 0;
    readRegister8(QMI8658_STATUS0, status); // 读状态寄存器
    TT_LOG_I(TAG, "QMI8658按需校正结果：%d", status);  // 打印信息

    return true;
}

// 读取加速度和陀螺仪寄存器值
bool Qmi8658a::getAccAndGry(t_sQMI8658 *p) const {
    uint8_t status = 0;
    uint8_t data_ready = 0;
    int16_t buf[6] = {0};

    readRegister8(QMI8658_STATUSINT, status); // 读状态寄存器
    if (status & 0x01) // 判断加速度和陀螺仪数据是否可读
        data_ready = 1;

    if (data_ready == 1){  // 如果数据可读
        data_ready = 0;
        if (tt::hal::i2c::masterReadRegister(port, address, QMI8658_AX_L, reinterpret_cast<uint8_t*>(buf), 12, DEFAULT_TIMEOUT)) {
            p->accX = buf[0];
            p->accY = buf[1];
            p->accZ = buf[2];
            p->gyrX = buf[3];
            p->gyrY = buf[4];
            p->gyrZ = buf[5];
            return true;
        }
    }
    return false;
}

// 获取XYZ轴的倾角值
bool Qmi8658a::getAll(t_sQMI8658 *p) const {
    float temp;

    if(getAccAndGry(p)){
        // 根据寄存器值 计算倾角值 并把弧度转换成角度
        temp = (float)p->accX / sqrt( ((float)p->accY * (float)p->accY + (float)p->accZ * (float)p->accZ) );
        p->angleX = atan(temp)*57.29578f; // 180/π=57.29578
        temp = (float)p->accY / sqrt( ((float)p->accX * (float)p->accX + (float)p->accZ * (float)p->accZ) );
        p->angleY = atan(temp)*57.29578f; // 180/π=57.29578
        temp = sqrt( ((float)p->accX * (float)p->accX + (float)p->accY * (float)p->accY) ) / (float)p->accZ;
        p->angleZ = atan(temp)*57.29578f; // 180/π=57.29578

        return true;        
    }else{
        return false;
    }
}