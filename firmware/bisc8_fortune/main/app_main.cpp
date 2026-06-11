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
#include <driver/usb_serial_jtag.h>

#include "app_events.h"
#include "app_config.h"
#include "audio_service.h"
#include "battery.h"
#include "board_support.h"
#include "button_controller.h"
#include "connectivity_service.h"
#include "debug_serial.h"
#include "display_service.h"
#include "epaper_config.h"
#include "fortune_service.h"
#include "voice_oracle_service.h"
#include "email_service.h"
#include "web_portal.h"
#include "build_info.h"
#include "sdkconfig.h"

using namespace bisc8;

namespace {

// Deep-sleep wake source. On the ESP32-C6 only LP/RTC GPIOs (GPIO0-7) can wake
// from deep sleep; BOOT (GPIO9) is an HP GPIO, so including it made
// esp_deep_sleep_enable_gpio_wakeup() reject the WHOLE mask with
// ESP_ERR_INVALID_ARG -> EnterDeepSleep bailed before esp_deep_sleep_start() and
// the device never slept, re-flashing the low-power screen every idle timeout.
// PWR (GPIO2) is LP-capable, so wake is PWR-only (matches the manual-sleep path).
constexpr uint64_t kAnyButtonWakeMask = BIT64(PWR_BUTTON_PIN);
constexpr uint32_t kMinimumBootSplashMs = 5000;
constexpr uint32_t kOnlineStatusSplashMs = 2800;
constexpr uint32_t kLowPowerSplashMs = 1600;
constexpr uint32_t kConnectFailedSplashMs = 2600;
constexpr uint8_t kLowBatteryWarnPct = 12;
// At/below this charge the device ALWAYS powers off completely (after writing it
// on screen) to protect the cell from a deep over-discharge. Checked at boot AND
// on every event, so it triggers from any screen.
constexpr uint8_t kCriticalBatteryShutdownPct = 10;

// Audio -> display state trampolines: AudioService fires these from its
// playback/record threads; DisplayService takes the LVGL lock internally.
void OnAudioPlayback(void *ctx, bool active) {
    static_cast<DisplayService *>(ctx)->OnPlaybackState(active);
}
void OnAudioRecording(void *ctx, bool active) {
    static_cast<DisplayService *>(ctx)->OnListeningState(active);
}

// The OpenAI voice flow (STT/Brain/TTS) runs mbedTLS HTTPS handshakes, which
// need far more stack than the main task carries (the inline call panicked with
// a stack-protection fault). Run it on a dedicated worker with a generous stack
// and block until it finishes. Keeping the main task small also leaves a big
// contiguous heap free for the 64 KB microphone buffer.
// The flow runs in two phases on dedicated workers so the screen can show the
// text answer the moment the brain has it, BEFORE the slow TTS audio download
// (which on a weak network can take tens of seconds). Phase 1 = STT + brain,
// phase 2 = TTS; the main task paints the answer between them.
struct OracleTextJob {
    VoiceOracleService *oracle;
    const char *wav_path;
    const OpenAiSettings *openai;
    OracleResponse *response;
    esp_err_t result;
    TaskHandle_t caller;
};

void OracleTextTaskEntry(void *arg) {
    auto *job = static_cast<OracleTextJob *>(arg);
    job->result = job->oracle->AskTextAnswer(job->wav_path, *job->openai, job->response);
    xTaskNotifyGive(job->caller);
    vTaskDelete(nullptr);
}

esp_err_t RunOracleTextOnWorker(VoiceOracleService &oracle, const char *wav_path,
                                const OpenAiSettings &openai, OracleResponse *response) {
    OracleTextJob job{&oracle, wav_path, &openai, response, ESP_FAIL, xTaskGetCurrentTaskHandle()};
    if (xTaskCreate(OracleTextTaskEntry, "oracle_text", 16384, &job, 5, nullptr) != pdPASS) {
        DebugSerial::LogAlways("[ORACLE]", "text worker task create failed (no mem)");
        return ESP_ERR_NO_MEM;
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return job.result;
}

struct OracleSpeakJob {
    VoiceOracleService *oracle;
    const OpenAiSettings *openai;
    esp_err_t result;
    TaskHandle_t caller;
};

void OracleSpeakTaskEntry(void *arg) {
    auto *job = static_cast<OracleSpeakJob *>(arg);
    job->result = job->oracle->SpeakAnswer(*job->openai);
    xTaskNotifyGive(job->caller);
    vTaskDelete(nullptr);
}

esp_err_t RunOracleSpeakOnWorker(VoiceOracleService &oracle, const OpenAiSettings &openai) {
    OracleSpeakJob job{&oracle, &openai, ESP_FAIL, xTaskGetCurrentTaskHandle()};
    if (xTaskCreate(OracleSpeakTaskEntry, "oracle_tts", 16384, &job, 5, nullptr) != pdPASS) {
        DebugSerial::LogAlways("[ORACLE]", "tts worker task create failed (no mem)");
        return ESP_ERR_NO_MEM;
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return job.result;
}

// The email relay POST also runs mbedTLS, so it needs the same generous stack as
// the oracle. Same worker pattern: run it off the main task and block.
struct EmailJob {
    EmailService *email;
    const EmailSettings *settings;
    const OracleResponse *response;
    const char *audio_path;
    uint32_t answer_audio_bytes;
    esp_err_t result;
    TaskHandle_t caller;
};

void EmailTaskEntry(void *arg) {
    auto *job = static_cast<EmailJob *>(arg);
    job->result = job->email->SendOracleEmail(*job->settings, *job->response, job->audio_path,
                                              job->answer_audio_bytes);
    xTaskNotifyGive(job->caller);
    vTaskDelete(nullptr);
}

esp_err_t RunEmailOnWorker(EmailService &email, const EmailSettings &settings,
                           const OracleResponse &response, const char *audio_path,
                           uint32_t answer_audio_bytes) {
    EmailJob job{&email, &settings, &response, audio_path, answer_audio_bytes, ESP_FAIL,
                 xTaskGetCurrentTaskHandle()};
    if (xTaskCreate(EmailTaskEntry, "email", 16384, &job, 5, nullptr) != pdPASS) {
        DebugSerial::LogAlways("[EMAIL]", "worker task create failed (no mem)");
        return ESP_ERR_NO_MEM;
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return job.result;
}

// Map an oracle failure onto a friendly localized message plus a short code the
// user reads off the error screen and reports back for diagnosis.
struct OracleErr {
    const char *message;
    const char *code;
};
OracleErr OracleErrorInfo(const LocalizedStrings &s, VoiceOracleService::OracleFailure f) {
    using F = VoiceOracleService::OracleFailure;
    switch (f) {
        case F::NoKey:      return {s.voice_oracle_unconfigured_body, "E02"};
        case F::Transcribe: return {s.voice_error_generic_body, "E03"};
        case F::NoSpeech:   return {s.voice_no_speech_body, "E04"};
        case F::Brain:      return {s.voice_error_generic_body, "E05"};
        default:            return {s.voice_error_generic_body, "E00"};
    }
}

bool battery_is_low() {
    const uint8_t level = BatteryLevel();
    return level != 255 && level <= kLowBatteryWarnPct;
}

bool battery_is_critical() {
    const uint8_t level = BatteryLevel();
    return level != 255 && level <= kCriticalBatteryShutdownPct;
}

bool UsbHostConnected() {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    return usb_serial_jtag_is_driver_installed() && usb_serial_jtag_is_connected();
#else
    return false;
#endif
}

bool StayAwakeForUsb(const char *reason) {
    if (!UsbHostConnected()) {
        return false;
    }
    DebugSerial::LogAlways("[POWER]", "USB host connected; staying awake reason=%s",
                           reason == nullptr ? "unknown" : reason);
    return true;
}

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

void WaitForMinimumBootSplash(TickType_t boot_started) {
    const TickType_t minimum_ticks = pdMS_TO_TICKS(kMinimumBootSplashMs);
    const TickType_t elapsed_ticks = xTaskGetTickCount() - boot_started;
    if (elapsed_ticks < minimum_ticks) {
        vTaskDelay(minimum_ticks - elapsed_ticks);
    }
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
    if (strncmp(line, "SCREEN ", 7) == 0) {
        const char *which = line + 7;
        if (strcmp(which, "NOWIFI") == 0) {
            return post_serial_event(AppEvent::PreviewNoWifi, "screen no-wifi");
        }
        if (strcmp(which, "LOWBATT") == 0) {
            return post_serial_event(AppEvent::PreviewLowBattery, "screen low-battery");
        }
        if (strcmp(which, "FIRSTRUN") == 0) {
            return post_serial_event(AppEvent::PreviewFirstRun, "screen first-run");
        }
        if (strcmp(which, "SPEAK") == 0) {
            return post_serial_event(AppEvent::PreviewSpeaking, "screen speaking");
        }
        if (strcmp(which, "LOWPOWER") == 0) {
            return post_serial_event(AppEvent::PreviewLowPower, "screen low-power");
        }
        if (strcmp(which, "STATUS") == 0) {
            return post_serial_event(AppEvent::ShowStatus, "screen status");
        }
        if (strcmp(which, "CONNFAIL") == 0) {
            return post_serial_event(AppEvent::PreviewConnectFailed, "screen connect-failed");
        }
        if (strcmp(which, "ERROR") == 0) {
            return post_serial_event(AppEvent::PreviewError, "screen error");
        }
        if (strcmp(which, "LISTENING") == 0) {
            return post_serial_event(AppEvent::PreviewListening, "screen listening");
        }
        if (strcmp(which, "THINKING") == 0) {
            return post_serial_event(AppEvent::PreviewThinking, "screen thinking");
        }
        if (strcmp(which, "WIFISETUP") == 0) {
            return post_serial_event(AppEvent::PreviewWifiSetup, "screen wifi-setup");
        }
    }
    return false;
}

}  // namespace

extern "C" void app_main(void) {
    g_event_queue = xQueueCreate(8, sizeof(AppEvent));
    DebugSerial::Start(print_status, handle_serial_command);
    DebugSerial::LogAlways("[BOOT]", "Bisc8 firmware booting");
    DebugSerial::LogAlways("[BUILD]", "%s", BISC8_BUILD_VERSION);

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
    EmailService email;
    WebPortal portal;
    portal.BindStatus(&connectivity.Status());
    portal.BindConfig(&config_store, &settings);
    portal.BindRuntime(&connectivity, &display);
    g_fortune_count = fortunes.Count(Language::English);

    esp_err_t err = config_store.Init();
    if (err == ESP_OK) {
        err = config_store.Load(&settings);
    }
    g_config_ready = (err == ESP_OK);
    if (!g_config_ready) {
        DebugSerial::LogAlways("[CONFIG]", "config load failed: %s", esp_err_to_name(err));
    } else {
        g_fortune_count = fortunes.Count(ParseLanguage(settings.language.c_str()));
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

    BatteryInit();
    DebugSerial::LogAlways("[BATT]", "level=%u voltage=%.2fV", BatteryLevel(), BatteryVoltage());

    err = display.Initialize();
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[DISPLAY]", "display init failed: %s", esp_err_to_name(err));
        return;
    }
    display.ShowBoot();
    const TickType_t boot_started = xTaskGetTickCount();
    const Language startup_language = ParseLanguage(settings.language.c_str());

    audio.SetPlaybackObserver(&OnAudioPlayback, &display);
    audio.SetRecordingObserver(&OnAudioRecording, &display);

    err = audio.Initialize();
    g_audio_ready = (err == ESP_OK);
    if (!g_audio_ready) {
        DebugSerial::LogAlways("[AUDIO]", "audio init failed: %s", esp_err_to_name(err));
        display.ShowAudioUnavailable(startup_language);
    } else {
        audio.PlayCueAsync(AudioCue::Boot);
        audio.RearmQuestionSpool();  // pre-erase now so the first question records instantly
    }
    WaitForMinimumBootSplash(boot_started);
    display.SetBattery(BatteryLevel());
    display.ShowIntro(startup_language);

    bool setup_mode_active = false;
    if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
        DebugSerial::LogAlways("[BOOT]", "BOOT held during startup; forcing Wi-Fi setup");
        connectivity.StartSetupPortal(display, portal, startup_language, true);
        setup_mode_active = true;
    } else if (connectivity.TryKnownNetworks(settings, display, startup_language, false) != ESP_OK) {
        if (settings.wifi_count > 0) {
            // We had saved networks but reached none: name the one that failed
            // before dropping into the setup portal, so a just-saved network
            // gives the user a clear "couldn't connect" instead of a silent
            // bounce back to setup.
            DebugSerial::LogAlways("[WIFI]", "saved networks unreachable; last=%s",
                                   connectivity.LastAttemptSsid().c_str());
            display.ShowWifiConnectFailed(connectivity.LastAttemptSsid().c_str(), startup_language);
            vTaskDelay(pdMS_TO_TICKS(kConnectFailedSplashMs));
        }
        connectivity.StartSetupPortal(display, portal, startup_language, true);
        setup_mode_active = true;
    }

    if (!setup_mode_active && connectivity.Online()) {
        // Keep the config portal reachable on the home network too (on the STA
        // IP), so Wi-Fi, OpenAI and email can be changed without forcing setup
        // mode. The httpd binds to all interfaces; no SoftAP/captive DNS here.
        esp_err_t portal_err = portal.Start();
        DebugSerial::LogAlways("[WEB]", "config portal on LAN ip=%s result=%s",
                               connectivity.Status().connected_ip.empty() ? "?" : connectivity.Status().connected_ip.c_str(),
                               esp_err_to_name(portal_err));
        display.ShowStatus(connectivity.Status(), startup_language);
        vTaskDelay(pdMS_TO_TICKS(kOnlineStatusSplashMs));
        display.ShowIntro(startup_language);
    } else if (g_audio_ready && !setup_mode_active) {
        display.ShowIntro(startup_language);
    }

    // Hard battery cutoff: at/below kCriticalBatteryShutdownPct, write it on
    // screen and power off completely (deep sleep) to protect the cell. USB host
    // sessions are kept awake so development/flash loops do not lose the board.
    auto power_off_if_critical = [&]() {
        if (!battery_is_critical()) {
            return;
        }
        if (StayAwakeForUsb("low-battery")) {
            return;
        }
        const uint8_t level = BatteryLevel();
        DebugSerial::LogAlways("[BATT]", "critical %u%% <= %u%%: powering off to protect the cell",
                               level, kCriticalBatteryShutdownPct);
        g_state = "low-battery-off";
        print_status();
        display.ShowCriticalLowBattery(ParseLanguage(settings.language.c_str()));
        if (g_audio_ready) {
            audio.PlayCueAsync(AudioCue::Shutdown);
            audio.WaitForPlayback(8000);
        }
        vTaskDelay(pdMS_TO_TICKS(2400));  // hold the message on-screen a beat
        board.EnterDeepSleep("low-battery", kAnyButtonWakeMask);
    };
    power_off_if_critical();  // boot/wake check — fires from any state, any case

    // Resting-screen overrides: nothing to read yet, or the battery is at the
    // bone. Runs on every boot, including wake from deep sleep, so it doubles
    // as the low-battery auto-trigger.
    if (!setup_mode_active) {
        if (g_fortune_count == 0) {
            display.ShowFirstRun(startup_language);
        } else if (battery_is_low()) {
            DebugSerial::LogAlways("[BATT]", "low battery warning at boot level=%u", BatteryLevel());
            display.ShowLowBattery(startup_language);
        }
    }

    buttons.Initialize(g_event_queue);
    g_state = "idle";
    print_status();

    AppEvent event;
    for (;;) {
#if CONFIG_BISC8_IDLE_DEEP_SLEEP_ENABLED
        if (xQueueReceive(g_event_queue, &event, pdMS_TO_TICKS(CONFIG_BISC8_IDLE_SLEEP_DELAY_MS)) != pdTRUE) {
            if (StayAwakeForUsb("idle-timeout")) {
                continue;
            }
            if (portal.Running() && portal.ConsumeActivity()) {
                // Someone is configuring over the network: stay awake instead of
                // deep-sleeping (which would kill the portal mid-session and play
                // the boot cue on the next wake).
                DebugSerial::LogAlways("[POWER]", "idle timeout but config portal active; staying awake");
                continue;
            }
            g_state = "idle-sleep";
            DebugSerial::LogAlways("[POWER]",
                                   "idle timeout after %d ms; wake=BOOT|PWR",
                                   CONFIG_BISC8_IDLE_SLEEP_DELAY_MS);
            display.ShowLowPower(ParseLanguage(settings.language.c_str()));
            vTaskDelay(pdMS_TO_TICKS(kLowPowerSplashMs));
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

        display.SetBattery(BatteryLevel());
        power_off_if_critical();  // <=10% from any screen -> write it on screen + power off
        switch (event) {
            case AppEvent::GenerateFortune: {
                g_state = "fortune";
                g_fortune_presses++;
                const Language language = ParseLanguage(settings.language.c_str());
                FortunePick pick = fortunes.PickRandom(language);
                display.ShowFortune(pick.text, pick.index, pick.count);
                audio.PlayCue(AudioCue::OracleButton);
#if CONFIG_BISC8_AUTO_SLEEP_AFTER_FORTUNE
                if (!StayAwakeForUsb("fortune")) {
                    g_state = "sleep";
                    DebugSerial::LogAlways("[POWER]", "auto sleep after fortune in %d ms", CONFIG_BISC8_AUTO_SLEEP_DELAY_MS);
                    print_status();
                    vTaskDelay(pdMS_TO_TICKS(CONFIG_BISC8_AUTO_SLEEP_DELAY_MS));
                    board.EnterDeepSleep("fortune", kAnyButtonWakeMask);
                }
#endif
                g_state = "idle";
                break;
            }

            case AppEvent::MicTest:
                g_state = "mic-test";
                g_mic_tests++;
                audio.RunMicTest(display, ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::StartVoiceRecording:
                g_state = "listening";
                display.ShowVoiceListening(ParseLanguage(settings.language.c_str()));
                audio.StartVoiceRecording();
                break;

            case AppEvent::FinishVoiceRecording: {
                g_state = "thinking";
                const Language language = ParseLanguage(settings.language.c_str());
                display.ShowVoiceCooking(language);
                audio.PlayCueAsync(AudioCue::VoiceSubmit);
                const char *wav_path = audio.FinishVoiceRecording();
                if (wav_path == nullptr) {
                    // Recording never started (e.g. mic buffer unavailable): do
                    // not answer a stale question still sitting in the spool.
                    display.ShowError(StringsFor(language).recording_failed_body, "E01", language);
                    g_state = "idle";
                    break;
                }
                display.ShowVoiceThinking(language);
                OracleResponse response;
                // Phase 1 (STT + brain): get the text answer fast.
                err = RunOracleTextOnWorker(oracle, wav_path, settings.openai, &response);
                if (err == ESP_OK) {
                    // Paint the answer NOW, before fetching the spoken audio. The TTS
                    // download is the slow part on a weak network; showing the text
                    // first means the screen never looks frozen waiting for voice.
                    display.ShowVoiceSpeaking(response.oracle_answer_screen, language);
                    // Phase 2 (TTS): synthesize + play the spoken answer when ready.
                    RunOracleSpeakOnWorker(oracle, settings.openai);
                    if (oracle.HasAnswerAudio()) {
                        audio.PlayAnswerAudio(oracle.AnswerAudioBytes());  // speaks the answer; animates the glyph
                    }
                    if (settings.email.enabled) {
                        // Relay transcript + answer + the question WAV and the
                        // generated answer audio (both still in the spool here,
                        // before the rearm erases the question region).
                        const uint32_t answer_bytes =
                            oracle.HasAnswerAudio() ? oracle.AnswerAudioBytes() : 0;
                        RunEmailOnWorker(email, settings.email, response, wav_path, answer_bytes);
                    }
                } else {
                    const OracleErr e = OracleErrorInfo(StringsFor(language), oracle.LastFailure());
                    display.ShowError(e.message, e.code, language);
                }
                // Question WAV no longer needed (STT + email done); pre-erase the
                // spool now so the next recording starts instantly.
                audio.RearmQuestionSpool();
                g_state = "idle";
                break;
            }

            case AppEvent::ForceWifiSetup:
                g_state = "wifi-setup";
                connectivity.StartSetupPortal(display, portal, ParseLanguage(settings.language.c_str()), true);
                print_status();
                break;

            case AppEvent::FullConfigReset:
                g_state = "config-reset";
                err = config_store.Reset();
                g_config_ready = (err == ESP_OK);
                DebugSerial::LogAlways("[CONFIG]", "full config reset requested: %s", esp_err_to_name(err));
                connectivity.StartSetupPortal(display, portal, ParseLanguage(settings.language.c_str()), true);
                print_status();
                break;

            case AppEvent::ShowStatus: {
                g_state = "status";
                const Language language = ParseLanguage(settings.language.c_str());
                if (!connectivity.Online() && !connectivity.Status().setup_active) {
                    connectivity.StartSetupPortal(display, portal, language, false);
                }
                display.ShowStatus(connectivity.Status(), language);
                print_status();
                g_state = "idle";
                break;
            }

            case AppEvent::PreviewNoWifi:
                g_state = "no-wifi";
                display.ShowNoWifi(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewLowBattery:
                g_state = "low-battery";
                display.ShowLowBattery(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewFirstRun:
                g_state = "first-run";
                display.ShowFirstRun(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewSpeaking:
                g_state = "speaking";
                display.ShowVoiceSpeaking("Si.\nMa non dirlo\na nessuno.", ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewLowPower:
                g_state = "low-power";
                display.ShowLowPower(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewWifiSetup:
                g_state = "wifi-setup";
                display.ShowWifiSetup("Bisc8-XXXX", "192.168.4.1", ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewConnectFailed:
                g_state = "connect-failed";
                display.ShowWifiConnectFailed(
                    settings.wifi_count > 0 ? settings.wifi[0].ssid.c_str() : "Casa Wi-Fi",
                    ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewError: {
                g_state = "error";
                const Language lang = ParseLanguage(settings.language.c_str());
                display.ShowError(StringsFor(lang).voice_oracle_unconfigured_body, "E02", lang);
                g_state = "idle";
                break;
            }

            case AppEvent::PreviewListening:
                g_state = "listening";
                display.ShowVoiceListening(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::PreviewThinking:
                g_state = "thinking";
                display.ShowVoiceThinking(ParseLanguage(settings.language.c_str()));
                g_state = "idle";
                break;

            case AppEvent::Sleep:
                if (StayAwakeForUsb("power-off")) {
                    g_state = "idle";
                    print_status();
                    break;
                }
                g_state = "power-off";
                display.ShowPowerOff(ParseLanguage(settings.language.c_str()));
                audio.PlayCueAsync(AudioCue::Shutdown);
                audio.WaitForPlayback(20000);
                vTaskDelay(pdMS_TO_TICKS(300));
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
