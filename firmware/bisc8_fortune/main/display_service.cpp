#include "display_service.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "debug_serial.h"
#include "connectivity_service.h"
#include "epaper_config.h"
#include "generated/logo_assets.h"
#include "port_display.h"
#include "port_lvgl.h"

LV_FONT_DECLARE(bisc8_font_title)
LV_FONT_DECLARE(bisc8_font_small)
LV_FONT_DECLARE(bisc8_font_body)

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
    lv_obj_set_style_text_line_space(label, 2, LV_PART_MAIN);
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

lv_obj_t *create_white(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *obj = lv_obj_create(parent);
    style_plain_obj(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
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

const char *SetupDisplayAddress(const char *url, char *buffer, size_t buffer_len) {
    if (buffer == nullptr || buffer_len == 0) {
        return "";
    }
    const char *source = (url == nullptr || url[0] == '\0') ? "192.168.4.1" : url;
    if (strncmp(source, "http://", 7) == 0) {
        source += 7;
    } else if (strncmp(source, "https://", 8) == 0) {
        source += 8;
    }
    size_t i = 0;
    while (source[i] != '\0' && source[i] != '/' && i + 1 < buffer_len) {
        buffer[i] = source[i];
        ++i;
    }
    buffer[i] = '\0';
    return buffer;
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

        BuildSplash();
        BuildChrome();
        BuildArrow();

        mascot_big_ = lv_image_create(screen_);
        lv_image_set_src(mascot_big_, &kBisc8BootLogo);
        lv_obj_remove_flag(mascot_big_, LV_OBJ_FLAG_SCROLLABLE);

        title_label_ = lv_label_create(screen_);
        style_label(title_label_, &bisc8_font_title, LV_TEXT_ALIGN_CENTER);

        body_label_ = lv_label_create(screen_);
        style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);

        footer_left_ = lv_label_create(screen_);
        style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);

        footer_right_ = lv_label_create(screen_);
        style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);

        LayoutBoot();
        lv_label_set_text(title_label_, "Bisc8");
        lv_label_set_text(body_label_, "by Netmilk Studio");
        lv_label_set_text(footer_left_, "premi PWR");
        lv_obj_update_layout(screen_);

        lv_screen_load(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::BuildSplash() {
    splash_group_ = lv_obj_create(screen_);
    style_plain_obj(splash_group_);
    lv_obj_set_pos(splash_group_, 0, 0);
    lv_obj_set_size(splash_group_, EPD_WIDTH, EPD_HEIGHT);
    create_frame(splash_group_, 8, 8, 184, 184, 2);
    create_frame(splash_group_, 12, 12, 176, 176, 1);
}

void DisplayService::BuildChrome() {
    chrome_group_ = lv_obj_create(screen_);
    style_plain_obj(chrome_group_);
    lv_obj_set_pos(chrome_group_, 0, 0);
    lv_obj_set_size(chrome_group_, EPD_WIDTH, EPD_HEIGHT);

    const int stripe_y[] = {4, 7, 10, 13, 16};
    for (int y : stripe_y) {
        create_block(chrome_group_, 0, y, 200, 1);
    }
    create_block(chrome_group_, 0, 20, 200, 2);
    create_block(chrome_group_, 0, 178, 200, 2);

    create_white(chrome_group_, 7, 5, 12, 12);
    create_frame(chrome_group_, 7, 5, 12, 12, 2);

    create_white(chrome_group_, 60, 0, 80, 20);

    mascot_glyph_ = lv_image_create(chrome_group_);
    lv_image_set_src(mascot_glyph_, &kBisc8BootLogo);
    lv_image_set_pivot(mascot_glyph_, 0, 0);
    lv_image_set_scale(mascot_glyph_, 64);  // 25% of 64px -> 16px
    lv_obj_set_pos(mascot_glyph_, 68, 2);
    lv_obj_remove_flag(mascot_glyph_, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name = lv_label_create(chrome_group_);
    style_label(name, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(name, "bisc8");
    lv_obj_set_pos(name, 88, 3);
}

void DisplayService::BuildArrow() {
    arrow_group_ = lv_obj_create(screen_);
    style_plain_obj(arrow_group_);
    lv_obj_set_size(arrow_group_, 24, 16);
    create_block(arrow_group_, 0, 6, 15, 3);    // shaft
    create_block(arrow_group_, 13, 1, 3, 13);   // head, widest
    create_block(arrow_group_, 16, 4, 3, 7);
    create_block(arrow_group_, 19, 6, 3, 3);    // tip
    set_hidden(arrow_group_, true);
}

void DisplayService::SetText(const char *title, const char *body, const char *footer) {
    lv_label_set_text(title_label_, title != nullptr ? title : "");
    lv_label_set_text(body_label_, body != nullptr ? body : "");
    lv_label_set_text(footer_left_, footer != nullptr ? footer : "");
    lv_obj_update_layout(screen_);
}

void DisplayService::LayoutBoot() {
    set_hidden(splash_group_, false);
    set_hidden(chrome_group_, true);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, false);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);

    lv_image_set_scale(mascot_big_, 256);
    lv_obj_set_pos(mascot_big_, 68, 40);

    style_label(title_label_, &bisc8_font_title, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 110);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 0, 142);
    lv_obj_set_size(body_label_, 200, 18);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 164);
    lv_obj_set_size(footer_left_, 200, 18);
}

void DisplayService::LayoutIntro() {
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, false);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, true);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 6, 70);
    lv_obj_set_size(body_label_, 150, 70);

    lv_obj_set_pos(arrow_group_, 162, 92);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 182);
    lv_obj_set_size(footer_left_, 200, 16);
}

void DisplayService::LayoutMessage() {
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);

    style_label(title_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 32);
    lv_obj_set_size(title_label_, 200, 18);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 12, 60);
    lv_obj_set_size(body_label_, 176, 110);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 184, 16);
}

void DisplayService::LayoutResponso(const char *count) {
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, true);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, false);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(body_label_, 10, 40);
    lv_obj_set_size(body_label_, 180, 134);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 80, 16);

    style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_pos(footer_right_, 112, 182);
    lv_obj_set_size(footer_right_, 80, 16);
    lv_label_set_text(footer_right_, count != nullptr ? count : "");
}

void DisplayService::LayoutWifiSetup() {
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);

    style_label(title_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 4, 40);
    lv_obj_set_size(title_label_, 192, 20);

    style_label(body_label_, &bisc8_font_title, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(body_label_, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(body_label_, 0, 78);
    lv_obj_set_size(body_label_, 200, 40);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 182);
    lv_obj_set_size(footer_left_, 200, 16);
}

void DisplayService::ShowBoot() {
    if (Lvgl_lock(-1)) {
        LayoutBoot();
        lv_label_set_text(title_label_, "Bisc8");
        lv_label_set_text(body_label_, "by Netmilk Studio");
        lv_label_set_text(footer_left_, "premi PWR");
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowIntro(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutIntro();
        lv_label_set_text(body_label_, strings.intro_body);
        lv_label_set_text(footer_left_, strings.intro_footer);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowStatus(const WifiStatus &status, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char body[96];
    char footer[48];
    snprintf(footer, sizeof(footer), "%s", strings.status_footer);
    if (status.online) {
        snprintf(body,
                 sizeof(body),
                 strings.status_online_body,
                 status.connected_ip.empty() ? "IP pending" : status.connected_ip.c_str(),
                 status.connected_ssid.empty() ? "Wi-Fi" : status.connected_ssid.c_str());
    } else if (status.setup_active) {
        char address[32];
        snprintf(body,
                 sizeof(body),
                 strings.status_setup_body,
                 status.setup_ssid.empty() ? "Bisc8-XXXX" : status.setup_ssid.c_str(),
                 SetupDisplayAddress(status.setup_url.c_str(), address, sizeof(address)));
    } else {
        snprintf(body, sizeof(body), "%s", strings.status_offline_body);
    }
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.status_title, body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowPowerOff(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.offline_title, "Zzz", strings.sleep_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowIdle(size_t fortune_count, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char body[80];
    snprintf(body, sizeof(body), "%u %s", static_cast<unsigned>(fortune_count), strings.idle_body);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText("", body, strings.idle_footer);
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
        LayoutMessage();
        SetText(strings.wifi_connecting_title, body, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiSetup(const char *ssid, const char *url, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char net[64];
    snprintf(net, sizeof(net), strings.wifi_setup_body, (ssid == nullptr || ssid[0] == '\0') ? "Bisc8-XXXX" : ssid);
    char address[32];
    SetupDisplayAddress(url, address, sizeof(address));
    if (Lvgl_lock(-1)) {
        LayoutWifiSetup();
        lv_label_set_text(title_label_, net);
        lv_label_set_text(body_label_, address);
        lv_label_set_text(footer_left_, strings.wifi_setup_footer);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowFortune(const char *fortune, size_t index, size_t count) {
    char counter[24];
    snprintf(counter, sizeof(counter), "%u/%u", static_cast<unsigned>(index + 1), static_cast<unsigned>(count));
    if (Lvgl_lock(-1)) {
        LayoutResponso(counter);
        lv_label_set_text(body_label_, fortune != nullptr ? fortune : "");
        lv_label_set_text(footer_left_, "");
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceListening(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.listening_title, strings.listening_body, strings.voice_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceCooking(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.cooking_title, strings.cooking_body, "OpenAI");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceThinking(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.thinking_title, strings.thinking_body, "OpenAI");
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceSpeaking(const char *screen_answer, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.speaking_title, screen_answer == nullptr ? strings.speaking_body : screen_answer, strings.speaking_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicRecording(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.listening_title, strings.listening_body, strings.voice_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicPlayback(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.speaking_title, strings.speaking_body, strings.speaking_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowMicDone(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutIntro();
        lv_label_set_text(body_label_, strings.intro_body);
        lv_label_set_text(footer_left_, strings.intro_footer);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowAudioUnavailable(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.error_title, strings.audio_unavailable_body, strings.error_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowSleep(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.offline_title, strings.offline_body, strings.sleep_footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowError(const char *message, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        SetText(strings.error_title, message, strings.error_footer);
        Lvgl_unlock();
    }
}

}  // namespace bisc8
