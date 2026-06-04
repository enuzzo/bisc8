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

    // Latest battery charge (0-100, or 255 = unknown -> indicator hidden).
    void SetBattery(uint8_t pct);

private:
    void CreateScreen();
    void BuildChrome();
    void BuildSplash();
    void BuildArrow();

    // Layouts (call inside an Lvgl_lock).
    void LayoutBoot();                 // full-screen splash + mascot, no chrome
    void LayoutIntro();                // chrome + instruction + arrow toward BOOT
    void LayoutMessage();              // chrome + small-caps title + centered body
    void LayoutResponso();             // chrome + big body + count footer
    void LayoutWifiSetup();            // chrome + network line + big IP + hint

    void SetText(const char *title, const char *body, const char *footer);
    void RenderBattery();              // footer-right battery % (or blank if unknown)

    lv_obj_t *screen_ = nullptr;
    lv_obj_t *chrome_group_ = nullptr;
    lv_obj_t *splash_group_ = nullptr;
    lv_obj_t *arrow_group_ = nullptr;    // right-pointing arrow on the idle prompt
    lv_obj_t *mascot_big_ = nullptr;
    lv_obj_t *mascot_glyph_ = nullptr;
    lv_obj_t *title_label_ = nullptr;
    lv_obj_t *body_label_ = nullptr;
    lv_obj_t *footer_left_ = nullptr;
    lv_obj_t *footer_right_ = nullptr;
    uint8_t battery_pct_ = 255;          // 255 = unknown
};

}  // namespace bisc8
