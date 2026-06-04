#include "display_service.h"

#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "connectivity_service.h"
#include "epaper_config.h"
#include "port_display.h"
#include "port_lvgl.h"

LV_FONT_DECLARE(bisc8_font_title_25)
LV_FONT_DECLARE(bisc8_font_ui_14)
LV_FONT_DECLARE(bisc8_font_body_16)

namespace bisc8 {

namespace {

constexpr uint16_t kEpaperBlackThresholdRgb565 = 0xc618;

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    uint16_t *source = reinterpret_cast<uint16_t *>(color_p);
    EPD_Clear();
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint8_t color = (*source < kEpaperBlackThresholdRgb565) ? DRIVER_COLOR_BLACK : DRIVER_COLOR_WHITE;
            EPD_DrawColorPixel(x, y, color);
            source++;
        }
    }
    EPD_DisplayPart();
    lv_disp_flush_ready(disp);
}

void style_label(lv_obj_t *label, const lv_font_t *font, lv_text_align_t align) {
    lv_obj_remove_style_all(label);
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(label, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

void style_plain_obj(lv_obj_t *obj) {
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *create_frame(lv_obj_t *parent, int x, int y, int w, int h, int border_width) {
    lv_obj_t *obj = lv_obj_create(parent);
    style_plain_obj(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, border_width, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    return obj;
}

lv_obj_t *create_block(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *obj = lv_obj_create(parent);
    style_plain_obj(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN);
    return obj;
}

lv_obj_t *create_dot(lv_obj_t *parent, int x, int y, int size) {
    lv_obj_t *obj = create_block(parent, x, y, size, size);
    lv_obj_set_style_radius(obj, size / 2, LV_PART_MAIN);
    return obj;
}

void set_hidden(lv_obj_t *obj, bool hidden) {
    if (obj == nullptr) {
        return;
    }
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace

esp_err_t DisplayService::Initialize() {
    DebugSerial::LogAlways("[DISPLAY]", "initializing e-paper and LVGL");
    PortLvgl_Start_Init();
    Lvgl_PortInit(lvgl_flush_cb);
    CreateScreen();
    return ESP_OK;
}

void DisplayService::CreateScreen() {
    if (Lvgl_lock(-1)) {
        screen_ = lv_obj_create(nullptr);
        lv_obj_remove_style_all(screen_);
        lv_obj_set_size(screen_, EPD_WIDTH, EPD_HEIGHT);
        lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_opa(screen_, 255, LV_PART_MAIN);
        lv_obj_set_style_bg_color(screen_, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_pad_all(screen_, 0, LV_PART_MAIN);
        lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

        CreateOracleFrame();
        CreateLogoIcon();
        CreateSeal();
        CreateButtonHints();

        title_label_ = lv_label_create(screen_);
        style_label(title_label_, &bisc8_font_title_25, LV_TEXT_ALIGN_CENTER);

        body_label_ = lv_label_create(screen_);
        style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

        footer_label_ = lv_label_create(screen_);
        style_label(footer_label_, &bisc8_font_ui_14, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_line_space(footer_label_, 1, LV_PART_MAIN);

        ApplyBootLayout();
        lv_screen_load(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::CreateOracleFrame() {
    create_frame(screen_, 2, 2, 196, 196, 2);
    create_frame(screen_, 7, 7, 186, 186, 1);

    create_block(screen_, 12, 12, 25, 2);
    create_block(screen_, 12, 12, 2, 25);
    create_block(screen_, 163, 12, 25, 2);
    create_block(screen_, 186, 12, 2, 25);
    create_block(screen_, 12, 186, 25, 2);
    create_block(screen_, 12, 163, 2, 25);
    create_block(screen_, 163, 186, 25, 2);
    create_block(screen_, 186, 163, 2, 25);
}

void DisplayService::CreateCookieIcon() {
    cookie_group_ = lv_obj_create(screen_);
    style_plain_obj(cookie_group_);
    lv_obj_set_pos(cookie_group_, 70, 27);
    lv_obj_set_size(cookie_group_, 60, 46);

    lv_obj_t *body = create_frame(cookie_group_, 5, 7, 50, 35, 3);
    lv_obj_set_style_radius(body, 18, LV_PART_MAIN);
    create_block(cookie_group_, 17, 23, 17, 3);
    create_block(cookie_group_, 33, 23, 3, 12);
    create_block(cookie_group_, 36, 33, 13, 3);
    create_dot(cookie_group_, 22, 17, 4);
    create_dot(cookie_group_, 38, 21, 4);
    create_dot(cookie_group_, 24, 32, 4);
}

void DisplayService::CreateLogoIcon() {
    CreateCookieIcon();
}

void DisplayService::CreateSeal() {
    seal_group_ = lv_obj_create(screen_);
    style_plain_obj(seal_group_);
    lv_obj_set_pos(seal_group_, 53, 142);
    lv_obj_set_size(seal_group_, 94, 10);
    create_block(seal_group_, 11, 4, 72, 2);
    create_frame(seal_group_, 0, 1, 8, 8, 1);
    create_frame(seal_group_, 86, 1, 8, 8, 1);
}

void DisplayService::CreateButtonHints() {
    button_hint_group_ = lv_obj_create(screen_);
    style_plain_obj(button_hint_group_);
    lv_obj_set_pos(button_hint_group_, 166, 50);
    lv_obj_set_size(button_hint_group_, 24, 100);

    create_block(button_hint_group_, 0, 12, 18, 2);
    create_block(button_hint_group_, 14, 8, 2, 2);
    create_block(button_hint_group_, 16, 10, 2, 2);
    create_block(button_hint_group_, 18, 12, 2, 2);
    create_block(button_hint_group_, 16, 14, 2, 2);
    create_block(button_hint_group_, 14, 16, 2, 2);

    create_block(button_hint_group_, 0, 78, 18, 2);
    create_block(button_hint_group_, 14, 74, 2, 2);
    create_block(button_hint_group_, 16, 76, 2, 2);
    create_block(button_hint_group_, 18, 78, 2, 2);
    create_block(button_hint_group_, 16, 80, 2, 2);
    create_block(button_hint_group_, 14, 82, 2, 2);
}

void DisplayService::ApplyBootLayout() {
    SetDecorations(true, false, false);
    style_label(title_label_, &bisc8_font_title_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 84);
    lv_obj_set_size(title_label_, 200, 32);

    style_label(body_label_, &bisc8_font_ui_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 0, 116);
    lv_obj_set_size(body_label_, 200, 20);
    lv_obj_set_style_text_line_space(body_label_, 0, LV_PART_MAIN);

    style_label(footer_label_, &bisc8_font_ui_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 0, 163);
    lv_obj_set_size(footer_label_, 200, 18);
    lv_obj_set_style_text_line_space(footer_label_, 0, LV_PART_MAIN);
}

void DisplayService::ApplyIdleLayout() {
    SetDecorations(false, true, true);
    style_label(title_label_, &bisc8_font_title_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 18);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 28, 74);
    lv_obj_set_size(body_label_, 144, 48);
    lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

    lv_obj_set_pos(seal_group_, 53, 128);
    style_label(footer_label_, &bisc8_font_ui_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 8, 154);
    lv_obj_set_size(footer_label_, 184, 34);
    lv_obj_set_style_text_line_space(footer_label_, 1, LV_PART_MAIN);
}

void DisplayService::ApplyOracleLayout() {
    SetDecorations(false, true, false);
    style_label(title_label_, &bisc8_font_title_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 18);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 18, 56);
    lv_obj_set_size(body_label_, 164, 86);
    lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

    lv_obj_set_pos(seal_group_, 53, 142);
    style_label(footer_label_, &bisc8_font_ui_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 8, 158);
    lv_obj_set_size(footer_label_, 184, 30);
    lv_obj_set_style_text_line_space(footer_label_, 1, LV_PART_MAIN);
}

void DisplayService::SetDecorations(bool show_cookie, bool show_seal, bool show_button_hints) {
    set_hidden(cookie_group_, !show_cookie);
    set_hidden(seal_group_, !show_seal);
    set_hidden(button_hint_group_, !show_button_hints);
}

void DisplayService::SetScreenTextLocked(const char *title, const char *body, const char *footer) {
    lv_label_set_text(title_label_, title);
    lv_label_set_text(body_label_, body);
    lv_label_set_text(footer_label_, footer);
    lv_obj_update_layout(screen_);
}

void DisplayService::SetScreenText(const char *title, const char *body, const char *footer) {
    if (Lvgl_lock(-1)) {
        SetScreenTextLocked(title, body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowBoot() {
    if (Lvgl_lock(-1)) {
        ApplyBootLayout();
        SetScreenTextLocked("Bisc8", "by Netmilk Studio", "oracle loading");
        Lvgl_unlock();
    }
}

void DisplayService::ShowIntro(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyIdleLayout();
        SetScreenTextLocked(strings.intro_title, strings.intro_body, strings.intro_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowStatus(const WifiStatus &status, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char body[96];
    if (status.online) {
        snprintf(body, sizeof(body), strings.status_online_body, status.connected_ssid.empty() ? "Wi-Fi" : status.connected_ssid.c_str());
    } else if (status.setup_active) {
        snprintf(body,
                 sizeof(body),
                 strings.status_setup_body,
                 status.setup_ssid.empty() ? "Bisc8-XXXX" : status.setup_ssid.c_str(),
                 status.setup_url.empty() ? "192.168.4.1" : status.setup_url.c_str());
    } else {
        snprintf(body, sizeof(body), "%s", strings.status_offline_body);
    }
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.status_title, body, strings.status_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowPowerOff(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyBootLayout();
        lv_obj_set_pos(footer_label_, 8, 154);
        lv_obj_set_size(footer_label_, 184, 34);
        SetScreenTextLocked("Bisc8", "by Netmilk Studio", strings.sleep_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowIdle(size_t fortune_count, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char footer[64];
    snprintf(footer, sizeof(footer), "%s", strings.idle_footer);
    char body[80];
    snprintf(body, sizeof(body), "%u %s", static_cast<unsigned>(fortune_count), strings.idle_body);
    if (Lvgl_lock(-1)) {
        ApplyIdleLayout();
        SetScreenTextLocked("Bisc8", body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiConnecting(const char *ssid, int seconds_left, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char body[96];
    snprintf(body, sizeof(body), strings.wifi_connecting_body, ssid == nullptr ? "saved Wi-Fi" : ssid);
    char footer[48];
    snprintf(footer, sizeof(footer), strings.wifi_connecting_footer, seconds_left);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.wifi_connecting_title, body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiSetup(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.wifi_setup_title, strings.wifi_setup_body, strings.wifi_setup_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowFortune(const char *fortune, size_t index, size_t count) {
    char footer[64];
    snprintf(footer, sizeof(footer), "#%u / %u", static_cast<unsigned>(index + 1), static_cast<unsigned>(count));
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Bisc8", fortune, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceListening(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.listening_title, strings.listening_body, strings.voice_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceCooking(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.cooking_title, strings.cooking_body, "OpenAI");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceThinking(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.thinking_title, strings.thinking_body, "OpenAI");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceSpeaking(const char *screen_answer, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.speaking_title, screen_answer == nullptr ? strings.speaking_body : screen_answer, strings.speaking_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicRecording(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.listening_title, strings.listening_body, strings.voice_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicPlayback(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.speaking_title, strings.speaking_body, strings.speaking_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicDone(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.intro_title, strings.intro_body, strings.intro_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowAudioUnavailable(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.error_title, strings.audio_unavailable_body, strings.error_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowSleep(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.offline_title, strings.offline_body, strings.sleep_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowError(const char *message, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked(strings.error_title, message, strings.error_footer);
        Lvgl_unlock();
    }
}

}  // namespace bisc8
