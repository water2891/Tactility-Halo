#pragma once

#include "Tactility/hal/sdcard/SdCardDevice.h"
#include <Tactility/Mutex.h>

#include <sd_protocol_types.h>
#include <utility>
#include <vector>
#include <hal/spi_types.h>
#include <soc/gpio_num.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

namespace tt::hal::sdcard {

/**
 * SD card interface at SDMMC interface
 */
class SDMMCDevice final : public SdCardDevice {

public:

  struct Config {
      Config(
          gpio_num_t sdClk,
          gpio_num_t sdCmd,
          gpio_num_t sdD0,
          MountBehaviour mountBehaviourAtBoot,
          sdmmc_host_t host = SDMMC_HOST_DEFAULT()
      ) : sdClk(sdClk),
          sdCmd(sdCmd),
          sdD0(sdD0),
          mountBehaviourAtBoot(mountBehaviourAtBoot),
          host(host)
      {}

      gpio_num_t sdClk;
      gpio_num_t sdCmd;
      gpio_num_t sdD0;
      SdCardDevice::MountBehaviour mountBehaviourAtBoot;
      sdmmc_host_t host;
      bool formatOnMountFailed = false;
      uint16_t maxOpenFiles = 4;
      uint16_t allocUnitSize = 16 * 1024;
      bool statusCheckEnabled = false;
  };

private:

  std::shared_ptr<Lock> lock = std::make_shared<Mutex>();
  std::string mountPath;
  sdmmc_card_t* card = nullptr;
  std::shared_ptr<Config> config;

  bool mountInternal(const std::string& mountPath);

public:

  explicit SDMMCDevice(std::unique_ptr<Config> config) : SdCardDevice(config->mountBehaviourAtBoot),
    config(std::move(config))
  {}

  std::string getName() const final { return "SD Card"; }
  std::string getDescription() const final { return "SD card via SDMMC interface with 1-SD Mode"; }

  bool mount(const std::string& mountPath) final;
  bool unmount() final;
  std::string getMountPath() const final { return mountPath; }

  Lock& getLock() const final {
      return *lock;
  }

  State getState() const override;

  sdmmc_card_t* _Nullable getCard() { return card; }
};

}