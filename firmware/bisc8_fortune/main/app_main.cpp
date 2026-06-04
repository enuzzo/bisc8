#include <stdio.h>
#include <string.h>

#include <esp_bit_defs.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "app_events.h"
#include "app_config.h"
#include "audio_service.h"
#include "board_support.h"
#include "button_controller.h"
#include "connectivity_service.h"
#include "debug_serial.h"
#include "display_service.h"
#include "epaper_config.h"
#include "fortune_service.h"
#include "voice_oracle_service.h"
#include "web_portal.h"
#include "sdkconfig.h"

using namespace bisc8;

namespace {

constexpr uint64_t kAnyButtonWakeMask = BIT64(BOOT_BUTTON_PIN) | BIT64(PWR_BUTTON_PIN);
QueueHandle_t g_event_queue = nullptr;
const char *g_state = "boot";
bool g_board_ready = false;
bool g_audio_ready = false;
bool g_config_ready = false;
size_t g_fortune_count = 0;
uint32_t g_fortune_presses = 0;
uint32_t g_mic_tests = 0;

void print_status() {
    printf("[STATUS] state=%s board=%s audio=%s config=%s debug=%s fortune_count=%u fortune_presses=%lu mic_tests=%lu free_heap=%lu\n",
           g_state,
           g_board_ready ? "ready" : "not-ready",
           g_audio_ready ? "ready" : "off",
           g_config_ready ? "ready" : "not-ready",
           DebugSerial::Verbose() ? "on" : "off",
           static_cast<unsigned>(g_fortune_count),
           static_cast<unsigned long>(g_fortune_presses),
           static_cast<unsigned long>(g_mic_tests),
           static_cast<unsigned long>(esp_get_free_heap_size()));
    fflush(stdout);
}

bool post_serial_event(AppEvent event, const char *name) {
    if (g_event_queue == nullptr) {
        DebugSerial::LogAlways("[SERIAL]", "%s ignored: event queue not ready", name);
        return true;
    }

    if (xQueueSend(g_event_queue, &event, 0) == pdTRUE) {
        DebugSerial::LogAlways("[SERIAL]", "queued %s", name);
    } else {
        DebugSerial::LogAlways("[SERIAL]", "queue full for %s", name);
    }
    return true;
}

bool handle_serial_command(const char *line) {
    if (strcmp(line, "FORTUNE") == 0) {
        return post_serial_event(AppEvent::GenerateFortune, "fortune");
    }
    if (strcmp(line, "MIC") == 0) {
        return post_serial_event(AppEvent::MicTest, "mic");
    }
    if (strcmp(line, "VOICE START") == 0) {
        return post_serial_event(AppEvent::StartVoiceRecording, "voice start");
    }
    if (strcmp(line, "VOICE STOP") == 0) {
        return post_serial_event(AppEvent::FinishVoiceRecording, "voice stop");
    }
    if (strcmp(line, "WIFI SETUP") == 0) {
        return post_serial_event(AppEvent::ForceWifiSetup, "wifi setup");
    }
    if (strcmp(line, "WIFI RESET") == 0 || strcmp(line, "CONFIG RESET") == 0) {
        return post_serial_event(AppEvent::FullConfigReset, "config reset");
    }
    return false;
}

}  // namespace

extern "C" void app_main(void) {
    g_event_queue = xQueueCreate(8, sizeof(AppEvent));
    DebugSerial::Start(print_status, handle_serial_command);
    DebugSerial::LogAlways("[BOOT]", "Bisc8 firmware booting");

    if (g_event_queue == nullptr) {
        DebugSerial::LogAlways("[BOOT]", "event queue allocation failed");
        return;
    }

    BoardSupport board;
    DisplayService display;
    FortuneService fortunes;
    AudioService audio;
    ButtonController buttons;
    ConfigStore config_store;
    DeviceSettings settings;
    ConnectivityService connectivity;
    VoiceOracleService oracle;
    WebPortal portal;
    portal.BindStatus(&connectivity.Status());
    portal.BindConfig(&config_store, &settings);
    g_fortune_count = fortunes.Count();

    esp_err_t err = config_store.Init();
    if (err == ESP_OK) {
        err = config_store.Load(&settings);
    }
    g_config_ready = (err == ESP_OK);
    if (!g_config_ready) {
        DebugSerial::LogAlways("[CONFIG]", "config load failed: %s", esp_err_to_name(err));
    } else {
        DebugSerial::LogAlways("[CONFIG]",
                               "loaded language=%s wifi_count=%u openai_key=%s email=%s recipient=%s relay=%s",
                               settings.language.c_str(),
                               static_cast<unsigned>(settings.wifi_count),
                               settings.openai.api_key.empty() ? "missing" : "set",
                               settings.email.enabled ? "enabled" : "disabled",
                               settings.email.recipient.empty() ? "missing" : "set",
                               settings.email.relay_url.empty() ? "missing" : "set");
    }

    err = board.Initialize();
    g_board_ready = (err == ESP_OK);
    if (!g_board_ready) {
        DebugSerial::LogAlways("[BOOT]", "board init failed: %s", esp_err_to_name(err));
        return;
    }

    err = display.Initialize();
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[DISPLAY]", "display init failed: %s", esp_err_to_name(err));
        return;
    }
    display.ShowBoot();
    vTaskDelay(pdMS_TO_TICKS(3600));

    err = audio.Initialize();
    g_audio_ready = (err == ESP_OK);
    if (!g_audio_ready) {
        DebugSerial::LogAlways("[AUDIO]", "audio init failed: %s", esp_err_to_name(err));
        display.ShowAudioUnavailable();
    }

    bool setup_mode_active = false;
    if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
        DebugSerial::LogAlways("[BOOT]", "BOOT held during startup; forcing Wi-Fi setup");
        connectivity.StartSetupPortal(display, portal);
        setup_mode_active = true;
    } else if (connectivity.TryKnownNetworks(settings, display) != ESP_OK) {
        connectivity.StartSetupPortal(display, portal);
        setup_mode_active = true;
    }

    if (g_audio_ready && !setup_mode_active) {
        display.ShowIdle(g_fortune_count);
    }

    buttons.Initialize(g_event_queue);
    g_state = "idle";
    print_status();

    AppEvent event;
    for (;;) {
#if CONFIG_BISC8_IDLE_DEEP_SLEEP_ENABLED
        if (xQueueReceive(g_event_queue, &event, pdMS_TO_TICKS(CONFIG_BISC8_IDLE_SLEEP_DELAY_MS)) != pdTRUE) {
            g_state = "idle-sleep";
            DebugSerial::LogAlways("[POWER]",
                                   "idle timeout after %d ms; wake=BOOT|PWR",
                                   CONFIG_BISC8_IDLE_SLEEP_DELAY_MS);
            print_status();
            board.EnterDeepSleep("idle-timeout", kAnyButtonWakeMask);
            g_state = "idle";
            print_status();
            continue;
        }
#else
        if (xQueueReceive(g_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
#endif

        switch (event) {
            case AppEvent::GenerateFortune: {
                g_state = "fortune";
                g_fortune_presses++;
                FortunePick pick = fortunes.PickRandom();
                display.ShowFortune(pick.text, pick.index, pick.count);
                audio.PlayChime();
#if CONFIG_BISC8_AUTO_SLEEP_AFTER_FORTUNE
                g_state = "sleep";
                DebugSerial::LogAlways("[POWER]", "auto sleep after fortune in %d ms", CONFIG_BISC8_AUTO_SLEEP_DELAY_MS);
                print_status();
                vTaskDelay(pdMS_TO_TICKS(CONFIG_BISC8_AUTO_SLEEP_DELAY_MS));
                board.EnterDeepSleep("fortune", kAnyButtonWakeMask);
#endif
                g_state = "idle";
                break;
            }

            case AppEvent::MicTest:
                g_state = "mic-test";
                g_mic_tests++;
                audio.RunMicTest(display);
                g_state = "idle";
                break;

            case AppEvent::StartVoiceRecording:
                g_state = "listening";
                display.ShowVoiceListening();
                audio.StartVoiceRecording();
                break;

            case AppEvent::FinishVoiceRecording: {
                g_state = "thinking";
                const char *wav_path = audio.FinishVoiceRecording();
                display.ShowVoiceThinking();
                OracleResponse response;
                err = oracle.AskFromRecordedAudio(wav_path, &response);
                if (err == ESP_OK) {
                    display.ShowVoiceSpeaking(response.oracle_answer_screen);
                } else {
                    display.ShowError("Voice oracle is not configured yet.");
                }
                g_state = "idle";
                break;
            }

            case AppEvent::ForceWifiSetup:
                g_state = "wifi-setup";
                connectivity.StartSetupPortal(display, portal);
                print_status();
                break;

            case AppEvent::FullConfigReset:
                g_state = "config-reset";
                err = config_store.Reset();
                g_config_ready = (err == ESP_OK);
                DebugSerial::LogAlways("[CONFIG]", "full config reset requested: %s", esp_err_to_name(err));
                connectivity.StartSetupPortal(display, portal);
                print_status();
                break;

            case AppEvent::Sleep:
                g_state = "power-off";
                display.ShowPowerOff();
                vTaskDelay(pdMS_TO_TICKS(1200));
#if CONFIG_BISC8_MANUAL_DEEP_SLEEP_ENABLED
                board.EnterDeepSleep("power-off", BIT64(PWR_BUTTON_PIN));
#else
                DebugSerial::LogAlways("[POWER]", "power-off deep sleep disabled");
#endif
                g_state = "idle";
                print_status();
                break;
        }
    }
}
