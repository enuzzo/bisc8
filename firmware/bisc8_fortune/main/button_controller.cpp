#include "button_controller.h"

#include "app_events.h"
#include "debug_serial.h"
#include "epaper_config.h"

namespace bisc8 {

namespace {
QueueHandle_t g_event_queue = nullptr;

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

    boot_button_ = new Button(BOOT_BUTTON_PIN);
    power_button_ = new Button(PWR_BUTTON_PIN, false, 1600, 0, false);

    boot_button_->OnClick([]() {
        DebugSerial::LogAlways("[BUTTON]", "BOOT click");
        send_event(AppEvent::GenerateFortune);
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
