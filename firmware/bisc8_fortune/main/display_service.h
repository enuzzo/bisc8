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
    void CreateOracleFrame();
    void CreateCookieIcon();
    void CreateLogoIcon();
    void CreateSeal();
    void CreateButtonHints();
    void ApplyBootLayout();
    void ApplyIdleLayout();
    void ApplyOracleLayout();
    void SetDecorations(bool show_cookie, bool show_seal, bool show_button_hints);
    void SetScreenText(const char *title, const char *body, const char *footer);
    void SetScreenTextLocked(const char *title, const char *body, const char *footer);

    lv_obj_t *screen_ = nullptr;
    lv_obj_t *title_label_ = nullptr;
    lv_obj_t *body_label_ = nullptr;
    lv_obj_t *footer_label_ = nullptr;
    lv_obj_t *cookie_group_ = nullptr;
    lv_obj_t *seal_group_ = nullptr;
    lv_obj_t *button_hint_group_ = nullptr;
};

}  // namespace bisc8
