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
    void ShowWifiConnectFailed(const char *ssid, Language language);
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
    void ShowError(const char *message, const char *code, Language language);
    void ShowNoWifi(Language language);
    void ShowLowBattery(Language language);
    void ShowCriticalLowBattery(Language language);  // final frame before <=10% auto power-off
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
    void BuildSleepCorners();          // biscuit-corner ornaments for the sleep card
    void BuildPressButton();           // dark "PREMI ->" pill on the intro screen
    void BuildMic();                   // microphone glyph + sound-wave bars (listening)
    void BuildWaitDots();              // three "..." dots (thinking/waiting)
    void BuildStatusQr();              // QR + SCAN pill + ssid/ip (Wi-Fi status)

    // Layouts (call inside an Lvgl_lock).
    void LayoutBoot();                 // full-screen splash + mascot, no chrome
    void LayoutIntro();                // chrome + instruction + arrow toward BOOT
    void LayoutMessage();              // chrome + small-caps title + centered body
    void LayoutResponso();             // chrome + big body + count footer
    void LayoutSpeaking();             // chrome + animated speaker glyph + answer
    void LayoutGlyphMessage();         // chrome + centered glyph + message body
    void LayoutLowPower();             // big logo + "Zzz..." + tap-to-wake line
    void LayoutStatusQr();             // chrome + state title + QR/SCAN + ssid/ip

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

    // Bounded blink for the intro arrow that points at the BOOT button: draws
    // the eye on entry, then settles visible (no perpetual e-ink refresh).
    void StartArrowBlink();
    void StopArrowBlink();
    void TickArrowBlink();
    static void ArrowBlinkThunk(lv_timer_t *timer);

    // Shared bounded pulse for the voice glyphs: the mic sound-waves while
    // listening, the "..." dots while waiting on OpenAI. mic and dots are never
    // on screen at once, so one timer drives whichever is active.
    void StartVoiceAnim(bool is_mic);
    void StopVoiceAnim();
    void TickVoiceAnim();
    static void VoiceAnimThunk(lv_timer_t *timer);

    lv_obj_t *screen_ = nullptr;
    lv_obj_t *chrome_group_ = nullptr;
    lv_obj_t *chrome_footer_rule_ = nullptr; // bottom separator line (hide when no footer)
    lv_obj_t *splash_group_ = nullptr;
    lv_obj_t *arrow_group_ = nullptr;    // right-pointing arrow on the idle prompt
    lv_obj_t *speaker_group_ = nullptr;  // animated speaker glyph (speaking state)
    lv_obj_t *speaker_wave1_ = nullptr;
    lv_obj_t *speaker_wave2_ = nullptr;
    lv_obj_t *speaker_wave3_ = nullptr;
    lv_obj_t *mic_group_ = nullptr;      // microphone glyph (listening state)
    lv_obj_t *mic_wave1_ = nullptr;      // mic sound-wave bars (animated)
    lv_obj_t *mic_wave2_ = nullptr;
    lv_obj_t *wait_group_ = nullptr;     // "..." dots (thinking/waiting state)
    lv_obj_t *wait_dot2_ = nullptr;      // dot1 is always on; dot2/dot3 animate
    lv_obj_t *wait_dot3_ = nullptr;
    lv_obj_t *wifi_group_ = nullptr;     // crossed-out Wi-Fi glyph (no-wifi screen)
    lv_obj_t *batt_big_group_ = nullptr; // large battery glyph (low-battery screen)
    lv_obj_t *batt_big_flash_ = nullptr; // blinking bar after the big battery glyph
    lv_obj_t *batt_icon_group_ = nullptr;// footer battery icon
    lv_obj_t *sleep_corner_group_ = nullptr; // biscuit-corner ornaments (sleep card)
    lv_obj_t *press_btn_group_ = nullptr; // dark "PREMI ->" pill (intro)
    lv_obj_t *press_label_ = nullptr;     // white "PREMI" text inside the pill
    lv_obj_t *press_arrow_ = nullptr;     // white arrow inside the pill (blinks)
    lv_obj_t *status_group_ = nullptr;    // Wi-Fi status screen: QR + ssid/ip
    lv_obj_t *status_qr_ = nullptr;       // lv_qrcode encoding http://<ip>
    lv_obj_t *status_net_label_ = nullptr;// SSID, ellipsized to one line
    lv_obj_t *status_ip_label_ = nullptr; // IP address, small
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
    lv_timer_t *arrow_timer_ = nullptr;
    int arrow_blink_ticks_left_ = 0;

    lv_timer_t *voice_anim_timer_ = nullptr;
    int voice_anim_ticks_left_ = 0;
    int voice_anim_phase_ = 0;
    bool voice_anim_is_mic_ = false;
    bool arrow_on_ = false;
    uint8_t battery_pct_ = 255;          // 255 = unknown
};

}  // namespace bisc8
