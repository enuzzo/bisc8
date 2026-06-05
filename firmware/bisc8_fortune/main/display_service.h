#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <lvgl.h>

#include "localization.h"

namespace bisc8 {

struct WifiStatus;

class DisplayService {
public:
    esp_err_t Initialize();
    void ShowBoot();
    void ShowIntro(Language language);
    void ShowStatus(const WifiStatus &status, Language language);
    void ShowPowerOff(Language language);
    void ShowIdle(size_t fortune_count, Language language);
    void ShowWifiConnecting(const char *ssid, int seconds_left, Language language);
    void ShowWifiSetup(const char *ssid, const char *url, Language language);
    void ShowFortune(const char *fortune, size_t index, size_t count);
    void ShowVoiceListening(Language language);
    void ShowVoiceCooking(Language language);
    void ShowVoiceThinking(Language language);
    void ShowVoiceSpeaking(const char *screen_answer, Language language);
    void ShowMicRecording(Language language);
    void ShowMicPlayback(Language language);
    void ShowMicDone(Language language);
    void ShowAudioUnavailable(Language language);
    void ShowSleep(Language language);
    void ShowError(const char *message, Language language);
    void ShowNoWifi(Language language);
    void ShowLowBattery(Language language);
    void ShowFirstRun(Language language);
    void ShowLowPower(Language language);

    // Latest battery charge (0-100, or 255 = unknown -> indicator hidden).
    void SetBattery(uint8_t pct);

    // Audio-driven UI state hooks. Thread-safe: they take the LVGL lock
    // internally, so they can be called from the AudioService callback thread.
    void OnPlaybackState(bool active);   // animates the speaker glyph while speaking
    void OnListeningState(bool active);  // future "ti ascolto" listening animation seam

private:
    void CreateScreen();
    void BuildChrome();
    void BuildSplash();
    void BuildArrow();
    void BuildSpeaker();
    void BuildWifiGlyph();
    void BuildLowBatteryGlyph();
    void BuildFooterBattery();

    // Layouts (call inside an Lvgl_lock).
    void LayoutBoot();                 // full-screen splash + mascot, no chrome
    void LayoutIntro();                // chrome + instruction + arrow toward BOOT
    void LayoutMessage();              // chrome + small-caps title + centered body
    void LayoutResponso();             // chrome + big body + count footer
    void LayoutWifiSetup();            // chrome + network line + big IP + hint
    void LayoutSpeaking();             // chrome + animated speaker glyph + answer
    void LayoutGlyphMessage();         // chrome + centered glyph + message body
    void LayoutLowPower();             // big logo + "Zzz..." + tap-to-wake line

    void SetText(const char *title, const char *body, const char *footer);
    void RenderBattery();              // footer-right battery % + icon (or blank)

    // Speaker "speaking" animation (driven by the AudioService playback hook).
    // Start/Stop assume the caller already holds the LVGL lock.
    void ResetAuxLayers(bool speaking);
    void StartSpeakingAnimation();
    void StopSpeakingAnimation();
    void TickSpeaking();
    static void SpeakTimerThunk(lv_timer_t *timer);

    // Low-battery "flash" animation: a small bar after the battery glyph that
    // blinks a bounded number of times then settles, echoing the speaker pulse.
    void StartBatteryFlash();
    void StopBatteryFlash();
    void TickBatteryFlash();
    static void BatteryFlashThunk(lv_timer_t *timer);

    lv_obj_t *screen_ = nullptr;
    lv_obj_t *chrome_group_ = nullptr;
    lv_obj_t *splash_group_ = nullptr;
    lv_obj_t *arrow_group_ = nullptr;    // right-pointing arrow on the idle prompt
    lv_obj_t *speaker_group_ = nullptr;  // animated speaker glyph (speaking state)
    lv_obj_t *speaker_wave1_ = nullptr;
    lv_obj_t *speaker_wave2_ = nullptr;
    lv_obj_t *speaker_wave3_ = nullptr;
    lv_obj_t *wifi_group_ = nullptr;     // crossed-out Wi-Fi glyph (no-wifi screen)
    lv_obj_t *batt_big_group_ = nullptr; // large battery glyph (low-battery screen)
    lv_obj_t *batt_big_flash_ = nullptr; // blinking bar after the big battery glyph
    lv_obj_t *batt_icon_group_ = nullptr;// footer battery icon
    lv_obj_t *batt_icon_fill_ = nullptr; // footer battery fill bar (width = charge)
    lv_obj_t *mascot_big_ = nullptr;
    lv_obj_t *mascot_glyph_ = nullptr;
    lv_obj_t *title_label_ = nullptr;
    lv_obj_t *body_label_ = nullptr;
    lv_obj_t *footer_left_ = nullptr;
    lv_obj_t *footer_right_ = nullptr;
    lv_timer_t *speak_timer_ = nullptr;
    int speak_phase_ = 0;
    int speak_ticks_left_ = 0;
    bool speaking_active_ = false;
    lv_timer_t *batt_flash_timer_ = nullptr;
    int batt_flash_ticks_left_ = 0;
    bool batt_flash_on_ = false;
    uint8_t battery_pct_ = 255;          // 255 = unknown
};

}  // namespace bisc8
