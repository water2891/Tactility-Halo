#include "MyFunS3SdCard.h"

#include "SDMMCDevice/SDMMCDevice.h"

#include <esp_vfs_fat.h>


#define MYFUNS3_SD_CLK GPIO_NUM_2
#define MYFUNS3_SD_CMD GPIO_NUM_43
#define MYFUNS3_SD_D0 GPIO_NUM_1

using tt::hal::sdcard::SDMMCDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto* configuration = new SDMMCDevice::Config(
        MYFUNS3_SD_CLK,
        MYFUNS3_SD_CMD,
        MYFUNS3_SD_D0,
        SdCardDevice::MountBehaviour::AtBoot,
        SDMMC_HOST_DEFAULT()
    );

    auto* sdcard = (SdCardDevice*) new SDMMCDevice(
        std::unique_ptr<SDMMCDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
