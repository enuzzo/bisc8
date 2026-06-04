#pragma once

#include <stddef.h>

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
    void ShowWifiSetup(Language language);
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

private:
    void CreateScreen();
    void BuildChrome();
    void BuildSplash();

    // Layouts (call inside an Lvgl_lock).
    void LayoutBoot();                 // full-screen splash + mascot, no chrome
    void LayoutIdle();                 // chrome + mascot + body
    void LayoutMessage();              // chrome + small-caps title + centered body
    void LayoutResponso(const char *count);  // chrome + title + big body + count footer

    void SetText(const char *title, const char *body, const char *footer);
    void SetFooter(const char *left, const char *right);

    lv_obj_t *screen_ = nullptr;
    lv_obj_t *chrome_group_ = nullptr;   // title bar (stripes, close box, glyph, name) + footer border
    lv_obj_t *splash_group_ = nullptr;   // boot double frame
    lv_obj_t *mascot_big_ = nullptr;     // full-size cracker (boot/idle)
    lv_obj_t *mascot_glyph_ = nullptr;   // small cracker in the title bar
    lv_obj_t *title_label_ = nullptr;    // small-caps section label in the body
    lv_obj_t *body_label_ = nullptr;     // main content
    lv_obj_t *footer_left_ = nullptr;    // footer: language / info
    lv_obj_t *footer_right_ = nullptr;   // footer: count
};

}  // namespace bisc8
