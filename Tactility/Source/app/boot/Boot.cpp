#include "Tactility/TactilityCore.h"

#include "Tactility/app/AppContext.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/Style.h"

#include "Tactility/hal/display/DisplayDevice.h"
#include <Tactility/TactilityPrivate.h>
#include <Tactility/hal/usb/Usb.h>
#include <Tactility/kernel/SystemEvents.h>

#include <lvgl.h>

#ifdef ESP_PLATFORM
#include "Tactility/app/crashdiagnostics/CrashDiagnostics.h"
#include <Tactility/kernel/PanicHandler.h>
#include <sdkconfig.h>
#else
#define CONFIG_TT_SPLASH_DURATION 0
#endif

#define TAG "boot"

namespace tt::app::boot {

static std::shared_ptr<tt::hal::display::DisplayDevice> getHalDisplay() {
    return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
}

class BootApp : public App {

private:

    Thread thread = Thread("boot", 4096, [this]() { return bootThreadCallback(); });

    int32_t bootThreadCallback() {
        TickType_t start_time = kernel::getTicks();

        kernel::publishSystemEvent(kernel::SystemEvent::BootSplash);

        auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);
        if (hal_display->supportsBacklightDuty()) {
            uint8_t backlight_duty = 200;
            app::display::getBacklightDuty(backlight_duty);
            TT_LOG_I(TAG, "backlight %du", backlight_duty);
            hal_display->setBacklightDuty(backlight_duty);
        } else {
            TT_LOG_I(TAG, "no backlight");
        }

        if (hal_display->getGammaCurveCount() > 0) {
            uint8_t gamma_curve;
            if (app::display::getGammaCurve(gamma_curve)) {
                hal_display->setGammaCurve(gamma_curve);
                TT_LOG_I(TAG, "gamma %du", gamma_curve);
            }
        }

        if (hal::usb::isUsbBootMode()) {
            TT_LOG_I(TAG, "Rebooting into mass storage device mode");
            hal::usb::resetUsbBootMode();
            hal::usb::startMassStorageWithSdmmc();
        } else {
            initFromBootApp();

            TickType_t end_time = tt::kernel::getTicks();
            TickType_t ticks_passed = end_time - start_time;
            TickType_t minimum_ticks = (CONFIG_TT_SPLASH_DURATION / portTICK_PERIOD_MS);
            if (minimum_ticks > ticks_passed) {
                kernel::delayTicks(minimum_ticks - ticks_passed);
            }

            tt::service::loader::stopApp();
            startNextApp();
        }

        return 0;
    }

    static void startNextApp() {
#ifdef ESP_PLATFORM
        esp_reset_reason_t reason = esp_reset_reason();
        if (reason == ESP_RST_PANIC) {
            app::crashdiagnostics::start();
            return;
        }
#endif

        auto* config = tt::getConfiguration();
        assert(!config->launcherAppId.empty());
        tt::service::loader::startApp(config->launcherAppId);
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* image = lv_image_create(parent);
        lv_obj_set_size(image, LV_PCT(100), LV_PCT(100));

        auto paths = app.getPaths();
        const char* logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo.png";
        auto logo_path = paths->getSystemPathLvgl(logo);
        TT_LOG_I(TAG, "%s", logo_path.c_str());
        lv_image_set_src(image, logo_path.c_str());

        auto* bottom_label = lv_label_create(parent);
        lv_label_set_text(bottom_label, TT_VERSION);
        lv_obj_align(bottom_label, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_text_color(bottom_label, lv_color_hex(0xFFFFFF), 0);

        lvgl::obj_set_style_bg_blacken(parent);

        // Just in case this app is somehow resumed
        if (thread.getState() == Thread::State::Stopped) {
            thread.start();
        }
    }

    void onDestroy(AppContext& app) override {
        thread.join();
    }
};

extern const AppManifest manifest = {
    .id = "Boot",
    .name = "Boot",
    .type = Type::Boot,
    .createApp = create<BootApp>
};

} // namespace
