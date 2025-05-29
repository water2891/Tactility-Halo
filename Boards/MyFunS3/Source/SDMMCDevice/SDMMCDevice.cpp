#include "SDMMCDevice.h"

#include <Tactility/Log.h>

#include <driver/gpio.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define TAG "sdmmc_sdcard"

namespace tt::hal::sdcard {

bool SDMMCDevice::mountInternal(const std::string& newMountPath) {
    TT_LOG_I(TAG, "Mounting %s", newMountPath.c_str());

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = config->formatOnMountFailed,
        .max_files = config->maxOpenFiles,
        .allocation_unit_size = config->allocUnitSize,
        .disk_status_check_enable = config->statusCheckEnabled,
        .use_one_fat = false
    };

    sdmmc_host_t host = config->host; // SDMMC主机接口配置
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT(); // SDMMC插槽配置
    slot_config.width = 1;  // 设置为1线SD模式
    slot_config.clk = config->sdClk; 
    slot_config.cmd = config->sdCmd;
    slot_config.d0 = config->sdD0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP; // 打开内部上拉电阻

    esp_err_t result = esp_vfs_fat_sdmmc_mount(newMountPath.c_str(), &host, &slot_config, &mount_config, &card);

    if (result != ESP_OK) {
        if (result == ESP_FAIL) {
            TT_LOG_E(TAG, "Mounting failed. Ensure the card is formatted with FAT.");
        } else {
            TT_LOG_E(TAG, "Mounting failed (%s)", esp_err_to_name(result));
        }
        return false;
    }

    mountPath = newMountPath;

    return true;
}

bool SDMMCDevice::mount(const std::string& newMountPath) {
    if (mountInternal(newMountPath)) {
        sdmmc_card_print_info(stdout, card);
        return true;
    } else {
        TT_LOG_E(TAG, "Mount failed for %s", newMountPath.c_str());
        return false;
    }
}

bool SDMMCDevice::unmount() {
    if (card == nullptr) {
        TT_LOG_E(TAG, "Can't unmount: not mounted");
        return false;
    }

    if (esp_vfs_fat_sdcard_unmount(mountPath.c_str(), card) == ESP_OK) {
        mountPath = "";
        card = nullptr;
        return true;
    } else {
        TT_LOG_E(TAG, "Unmount failed for %s", mountPath.c_str());
        return false;
    }
}

SdCardDevice::State SDMMCDevice::getState() const {
    if (card == nullptr) {
        return State::Unmounted;
    }

    bool result = sdmmc_get_status(card) == ESP_OK;

    if (result) {
        return State::Mounted;
    } else {
        return State::Error;
    }
}

}