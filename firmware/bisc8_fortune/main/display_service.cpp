#include "display_service.h"

#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "epaper_config.h"
#include "port_display.h"
#include "port_lvgl.h"

LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_25)
LV_FONT_DECLARE(lv_font_montserrat_14)
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

        title_label_ = lv_label_create(screen_);
        style_label(title_label_, &lv_font_montserratMedium_25, LV_TEXT_ALIGN_CENTER);

        body_label_ = lv_label_create(screen_);
        style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

        footer_label_ = lv_label_create(screen_);
        style_label(footer_label_, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
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

void DisplayService::ApplyBootLayout() {
    SetDecorations(true, false);
    style_label(title_label_, &lv_font_montserratMedium_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 84);
    lv_obj_set_size(title_label_, 200, 32);

    style_label(body_label_, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 0, 116);
    lv_obj_set_size(body_label_, 200, 20);
    lv_obj_set_style_text_line_space(body_label_, 0, LV_PART_MAIN);

    style_label(footer_label_, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 0, 163);
    lv_obj_set_size(footer_label_, 200, 18);
    lv_obj_set_style_text_line_space(footer_label_, 0, LV_PART_MAIN);
}

void DisplayService::ApplyIdleLayout() {
    SetDecorations(false, true);
    style_label(title_label_, &lv_font_montserratMedium_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 18);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 28, 74);
    lv_obj_set_size(body_label_, 144, 48);
    lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

    lv_obj_set_pos(seal_group_, 53, 128);
    style_label(footer_label_, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 8, 154);
    lv_obj_set_size(footer_label_, 184, 34);
    lv_obj_set_style_text_line_space(footer_label_, 1, LV_PART_MAIN);
}

void DisplayService::ApplyOracleLayout() {
    SetDecorations(false, true);
    style_label(title_label_, &lv_font_montserratMedium_25, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 18);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_body_16, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 18, 56);
    lv_obj_set_size(body_label_, 164, 86);
    lv_obj_set_style_text_line_space(body_label_, 4, LV_PART_MAIN);

    lv_obj_set_pos(seal_group_, 53, 142);
    style_label(footer_label_, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_label_, 8, 158);
    lv_obj_set_size(footer_label_, 184, 30);
    lv_obj_set_style_text_line_space(footer_label_, 1, LV_PART_MAIN);
}

void DisplayService::SetDecorations(bool show_cookie, bool show_seal) {
    set_hidden(cookie_group_, !show_cookie);
    set_hidden(seal_group_, !show_seal);
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

void DisplayService::ShowPowerOff() {
    if (Lvgl_lock(-1)) {
        ApplyBootLayout();
        lv_obj_set_pos(footer_label_, 8, 154);
        lv_obj_set_size(footer_label_, 184, 34);
        SetScreenTextLocked("Bisc8", "by Netmilk Studio", "Press PWR\nto turn me on");
        Lvgl_unlock();
    }
}

void DisplayService::ShowIdle(size_t fortune_count) {
    char footer[64];
    snprintf(footer, sizeof(footer), "Hold BOOT to ask\nPWR power");
    char body[80];
    snprintf(body, sizeof(body), "%u answers\nin the grimoire", static_cast<unsigned>(fortune_count));
    if (Lvgl_lock(-1)) {
        ApplyIdleLayout();
        SetScreenTextLocked("Bisc8", body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiConnecting(const char *ssid, int seconds_left) {
    char body[96];
    snprintf(body, sizeof(body), "Trying\n%s", ssid == nullptr ? "saved Wi-Fi" : ssid);
    char footer[48];
    snprintf(footer, sizeof(footer), "%d seconds", seconds_left);
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Wi-Fi", body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiSetup() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Setup Wi-Fi", "Join Bisc8-XXXX\nOpen 192.168.4.1", "Captive portal ready");
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

void DisplayService::ShowVoiceListening() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Listening", "Speak now.\nRelease BOOT to send.", "15 seconds max");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceThinking() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Thinking", "The oracle is reading your question.", "OpenAI");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceSpeaking(const char *screen_answer) {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Bisc8", screen_answer == nullptr ? "The answer is arriving." : screen_answer, "voice answer");
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicRecording() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Listening", "Speak now.\nRecording one second.", "PWR mic test");
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicPlayback() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Playback", "Playing the sample.", "audio check");
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicDone() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Done", "Microphone and speaker responded.", "Hold BOOT to ask");
        Lvgl_unlock();
    }
}

void DisplayService::ShowAudioUnavailable() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Audio off", "The codec is not ready.\nOffline fortunes still work.", "STATUS for details");
        Lvgl_unlock();
    }
}

void DisplayService::ShowSleep() {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Sleep", "The oracle stays printed on paper.", "wake by button");
        Lvgl_unlock();
    }
}

void DisplayService::ShowError(const char *message) {
    if (Lvgl_lock(-1)) {
        ApplyOracleLayout();
        SetScreenTextLocked("Error", message, "see serial");
        Lvgl_unlock();
    }
}

}  // namespace bisc8
