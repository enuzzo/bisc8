#include "button_controller.h"

#include <driver/gpio.h>

#include "app_events.h"
#include "debug_serial.h"
#include "epaper_config.h"

namespace bisc8 {

namespace {
QueueHandle_t g_event_queue = nullptr;
bool g_boot_voice_active = false;

void send_event(AppEvent event) {
    if (g_event_queue == nullptr) {
        return;
    }
    xQueueSend(g_event_queue, &event, 0);
}
}  // namespace

void ButtonController::Initialize(QueueHandle_t queue) {
    DebugSerial::LogAlways("[BUTTON]", "initializing BOOT=%d PWR=%d", BOOT_BUTTON_PIN, PWR_BUTTON_PIN);
    g_event_queue = queue;

    boot_button_ = new Button(BOOT_BUTTON_PIN, false, 1600, 0, false);
    power_button_ = new Button(PWR_BUTTON_PIN, false, 1600, 0, false);

    boot_button_->OnPressDown([]() {
        DebugSerial::LogAlways("[BUTTON]", "BOOT press down");
    });

    boot_button_->OnPressUp([]() {
        DebugSerial::LogAlways("[BUTTON]", "BOOT release");
        if (g_boot_voice_active) {
            g_boot_voice_active = false;
            send_event(AppEvent::FinishVoiceRecording);
        }
    });

    boot_button_->OnClick([]() {
        DebugSerial::LogAlways("[BUTTON]", "BOOT click");
        send_event(AppEvent::GenerateFortune);
    });

    boot_button_->OnLongPress([]() {
        if (gpio_get_level(PWR_BUTTON_PIN) == 0) {
            DebugSerial::LogAlways("[BUTTON]", "BOOT+PWR long press: full config reset");
            send_event(AppEvent::FullConfigReset);
            return;
        }
        DebugSerial::LogAlways("[BUTTON]", "BOOT hold start");
        g_boot_voice_active = true;
        send_event(AppEvent::StartVoiceRecording);
    });

    power_button_->OnClick([]() {
        DebugSerial::LogAlways("[BUTTON]", "PWR click");
        send_event(AppEvent::MicTest);
    });

    power_button_->OnLongPress([]() {
        DebugSerial::LogAlways("[BUTTON]", "PWR long press");
        send_event(AppEvent::Sleep);
    });
}

}  // namespace bisc8
