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

// Uniform typographic rhythm across every device screen. kBodyLineSpace is THE
// single knob for body interlinea everywhere (oracle messages, accessory
// bodies): keep it low/tight per the house style, tune it here once. Accessory
// titles read as spaced "eyebrow" labels, secondary to the big title font.
constexpr int kBodyLineSpace = 2;
constexpr int kSmallLineSpace = 2;
constexpr int kTitleLineSpace = 2;
constexpr int kEyebrowLetterSpace = 2;
// Message screens (LayoutMessage) center the body within the band between the
// title chip and the footer. This shifts the centring anchor down from the
// screen middle to that band's middle, so tall messages never touch the title.
constexpr int kMessageBodyDrop = 15;

// E-ink refresh policy, applied at the display_service / port_display boundary.
// Partial refresh is the default (fast, low flicker) but it ghosts, so we force
// a full refresh on dramatic reveals and after a small run of partials.
constexpr int kMaxPartialsBeforeFull = 30;
volatile bool g_force_full_refresh = false;  // set under the LVGL lock before a reveal
int g_partials_since_full = 0;               // touched only on the LVGL flush thread

// Request that the NEXT flush use a full (flashing) refresh. Call under the LVGL
// lock right before changing the screen so the change is the one that flashes.
void RequestFullRefresh() {
    g_force_full_refresh = true;
}

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

    const bool forced = g_force_full_refresh;
    g_force_full_refresh = false;
    const bool anti_ghost = g_partials_since_full >= kMaxPartialsBeforeFull;
    if (forced || anti_ghost) {
        EPD_DisplayFull();
        g_partials_since_full = 0;
        DebugSerial::Log("[EPD]", "flush full (%s)", forced ? "reveal" : "anti-ghost");
    } else {
        EPD_DisplayPart();
        g_partials_since_full++;
        DebugSerial::Log("[EPD]", "flush partial n=%d", g_partials_since_full);
    }
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
    int line_space = kSmallLineSpace;
    if (font == &bisc8_font_body) {
        line_space = kBodyLineSpace;
    } else if (font == &bisc8_font_title) {
        line_space = kTitleLineSpace;
    }
    lv_obj_set_style_text_line_space(label, line_space, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

// Page titles render as a centered black chip with white block-caps text, so a
// heading reads as a distinct bar above its body. Width hugs the text. The
// caller sets the text (uppercased) afterwards; an empty title is hidden by
// SetText so no empty chip shows.
void apply_title_chip(lv_obj_t *label, const lv_font_t *font, int y) {
    style_label(label, font, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_text_letter_space(label, kEyebrowLetterSpace, LV_PART_MAIN);
    lv_obj_set_style_bg_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, 255, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_pad_hor(label, 11, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(label, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(label, 0, LV_PART_MAIN);
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_set_align(label, LV_ALIGN_TOP_MID);
    lv_obj_set_x(label, 0);
    lv_obj_set_y(label, y);
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

void CopyLangCode(const char *code, char *dst, size_t dst_len) {
    size_t i = 0;
    for (; code != nullptr && code[i] != '\0' && i + 1 < dst_len; ++i) {
        dst[i] = static_cast<char>(toupper((unsigned char)code[i]));
    }
    dst[i] = '\0';
}

// Page titles render in UPPERCASE so they read as headings, distinct from the
// body below. ASCII-only fold: multibyte UTF-8 (accents) passes through
// untouched, which is fine for the short title strings we use.
const char *UpperAscii(const char *src, char *dst, size_t dst_len) {
    size_t i = 0;
    for (; src != nullptr && src[i] != '\0' && i + 1 < dst_len; ++i) {
        dst[i] = static_cast<char>(toupper((unsigned char)src[i]));
    }
    dst[i] = '\0';
    return dst;
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
        BuildSpeaker();
        BuildWifiGlyph();
        BuildLowBatteryGlyph();
        BuildFooterBattery();
        BuildSleepCorners();
        BuildPressButton();
        BuildMic();
        BuildWaitDots();
        BuildStatusQr();

        mascot_big_ = lv_image_create(screen_);
        lv_image_set_src(mascot_big_, &kBisc8BootLogo);
        // Scale from the top-left so a scaled logo occupies [pos, pos+size]
        // predictably (default pivot is the image centre, which makes a scaled
        // logo grow off-centre and overlap the title above it).
        lv_image_set_pivot(mascot_big_, 0, 0);
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
    chrome_footer_rule_ = create_block(chrome_group_, 0, 178, 200, 2);  // footer separator; hidden on footer-less screens

    create_white(chrome_group_, 7, 5, 12, 12);
    create_frame(chrome_group_, 7, 5, 12, 12, 2);

    // Centered window title. The tiny mascot glyph that used to sit here was
    // illegible at 16px, so it's gone -- just the name on a white chip.
    create_white(chrome_group_, 66, 0, 68, 20);

    lv_obj_t *name = lv_label_create(chrome_group_);
    style_label(name, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(name, "Bisc8");
    lv_obj_set_pos(name, 66, 3);
    lv_obj_set_width(name, 68);
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

void DisplayService::BuildSpeaker() {
    // Right-facing speaker cone with three sound-wave bars that the animation
    // toggles on and off. Pixel-art from solid blocks, matching the chrome.
    speaker_group_ = lv_obj_create(screen_);
    style_plain_obj(speaker_group_);
    lv_obj_set_size(speaker_group_, 31, 29);

    create_block(speaker_group_, 1, 10, 7, 9);    // back box
    create_block(speaker_group_, 8, 9, 3, 11);    // cone, stacked + flaring right
    create_block(speaker_group_, 11, 6, 3, 17);
    create_block(speaker_group_, 14, 3, 3, 23);   // cone face

    speaker_wave1_ = create_block(speaker_group_, 19, 10, 3, 9);
    speaker_wave2_ = create_block(speaker_group_, 23, 6, 3, 17);
    speaker_wave3_ = create_block(speaker_group_, 27, 3, 3, 23);
    set_hidden(speaker_group_, true);
}

void DisplayService::BuildMic() {
    // Pixel-art microphone (capsule head in a U-cradle on a stand) with two
    // sound-wave bars on the right that the animation pulses. "Speak now".
    mic_group_ = lv_obj_create(screen_);
    style_plain_obj(mic_group_);
    lv_obj_set_size(mic_group_, 29, 30);

    create_block(mic_group_, 8, 0, 6, 2);    // head: top cap
    create_block(mic_group_, 6, 2, 10, 13);  // head: body
    create_block(mic_group_, 8, 15, 6, 2);   // head: bottom cap
    create_block(mic_group_, 2, 9, 2, 7);    // cradle: left arm
    create_block(mic_group_, 18, 9, 2, 7);   // cradle: right arm
    create_block(mic_group_, 2, 16, 18, 2);  // cradle: bottom
    create_block(mic_group_, 10, 18, 2, 8);  // stand: stem
    create_block(mic_group_, 4, 26, 14, 2);  // stand: base

    mic_wave1_ = create_block(mic_group_, 23, 5, 2, 7);    // inner wave
    mic_wave2_ = create_block(mic_group_, 26, 2, 2, 13);   // outer wave
    set_hidden(mic_group_, true);
}

void DisplayService::BuildWaitDots() {
    // Three dots; dot1 stays on, dot2/dot3 reveal in sequence ("." ".." "...").
    wait_group_ = lv_obj_create(screen_);
    style_plain_obj(wait_group_);
    lv_obj_set_size(wait_group_, 32, 8);

    create_block(wait_group_, 0, 0, 8, 8);   // dot1 (always on)
    wait_dot2_ = create_block(wait_group_, 12, 0, 8, 8);
    wait_dot3_ = create_block(wait_group_, 24, 0, 8, 8);
    set_hidden(wait_group_, true);
}

void DisplayService::BuildStatusQr() {
    // Wi-Fi status screen: a QR that opens http://<ip>, a dark vertical "SCAN >"
    // pill to its left, and SSID (ellipsized) + IP beneath. The QR + ssid/ip text
    // are filled in ShowStatus; this only builds the static shapes.
    status_group_ = lv_obj_create(screen_);
    style_plain_obj(status_group_);
    lv_obj_set_pos(status_group_, 0, 0);
    lv_obj_set_size(status_group_, EPD_WIDTH, EPD_HEIGHT);

    // QR code (black on white) that opens http://<ip>. It is self-explanatory,
    // so no label/button is needed. The white screen around it is the quiet zone.
    status_qr_ = lv_qrcode_create(status_group_);
    lv_qrcode_set_size(status_qr_, 92);
    lv_qrcode_set_dark_color(status_qr_, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(status_qr_, lv_color_hex(0xffffff));
    lv_obj_remove_flag(status_qr_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(status_qr_, 54, 60);   // centered (200-92)/2, dropped for air below the title

    // SSID (one line, ellipsized) + IP beneath the QR.
    status_net_label_ = lv_label_create(status_group_);
    style_label(status_net_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(status_net_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(status_net_label_, 8, 158);
    lv_obj_set_size(status_net_label_, 184, 16);

    status_ip_label_ = lv_label_create(status_group_);
    style_label(status_ip_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(status_ip_label_, 8, 176);
    lv_obj_set_size(status_ip_label_, 184, 16);

    set_hidden(status_group_, true);
}

void DisplayService::BuildWifiGlyph() {
    // Three nested arcs over a base dot, struck through by a diagonal slash:
    // "no Wi-Fi". Arcs are drawn as downward-opening brackets.
    wifi_group_ = lv_obj_create(screen_);
    style_plain_obj(wifi_group_);
    lv_obj_set_size(wifi_group_, 50, 35);

    create_block(wifi_group_, 5, 2, 40, 3);      // outer arc top
    create_block(wifi_group_, 5, 5, 4, 8);       // outer arc leg L
    create_block(wifi_group_, 41, 5, 4, 8);      // outer arc leg R
    create_block(wifi_group_, 13, 11, 24, 4);    // mid arc top
    create_block(wifi_group_, 13, 15, 4, 6);     // mid arc leg L
    create_block(wifi_group_, 33, 15, 4, 6);     // mid arc leg R
    create_block(wifi_group_, 19, 20, 12, 3);    // inner arc top
    create_block(wifi_group_, 19, 23, 4, 5);     // inner arc leg L
    create_block(wifi_group_, 27, 23, 4, 5);     // inner arc leg R
    create_block(wifi_group_, 22, 29, 5, 5);     // base dot

    // diagonal slash, stair-stepped from top-left to bottom-right
    for (int i = 0; i < 16; ++i) {
        create_block(wifi_group_, 2 + i * 3, 1 + i * 2, 4, 4);
    }
    set_hidden(wifi_group_, true);
}

void DisplayService::BuildLowBatteryGlyph() {
    // Large battery outline with a thin, nearly empty fill: "battery to the bone".
    batt_big_group_ = lv_obj_create(screen_);
    style_plain_obj(batt_big_group_);
    lv_obj_set_size(batt_big_group_, 78, 34);

    create_frame(batt_big_group_, 0, 1, 62, 32, 3);  // shell
    create_block(batt_big_group_, 62, 11, 5, 12);    // terminal nub
    create_block(batt_big_group_, 7, 8, 10, 18);     // low charge sliver
    // Blinking "flash" bar after the battery, toggled by the low-battery
    // animation; same bounded pulse feel as the speaker waves.
    batt_big_flash_ = create_block(batt_big_group_, 71, 8, 7, 18);
    set_hidden(batt_big_flash_, true);
    set_hidden(batt_big_group_, true);
}

void DisplayService::BuildFooterBattery() {
    // Small battery icon for the footer; the fill bar width tracks the charge.
    batt_icon_group_ = lv_obj_create(screen_);
    style_plain_obj(batt_icon_group_);
    lv_obj_set_size(batt_icon_group_, 19, 11);

    create_frame(batt_icon_group_, 0, 1, 16, 9, 2);  // shell
    create_block(batt_icon_group_, 16, 3, 2, 5);     // terminal nub
    batt_icon_fill_ = create_block(batt_icon_group_, 2, 3, 12, 5);  // fill (resized live)
    set_hidden(batt_icon_group_, true);
}

void DisplayService::BuildSleepCorners() {
    // Four corner ornaments framing the sleep card: an L-bracket plus two little
    // "fork pricks", like the docked corners of a baked biscuit.
    sleep_corner_group_ = lv_obj_create(screen_);
    style_plain_obj(sleep_corner_group_);
    lv_obj_set_pos(sleep_corner_group_, 0, 0);
    lv_obj_set_size(sleep_corner_group_, EPD_WIDTH, EPD_HEIGHT);

    constexpr int kArm = 16;   // bracket arm length
    constexpr int kThick = 3;  // bracket thickness
    constexpr int kIn = 6;     // inset from the screen edge
    constexpr int kRight = 200 - kIn - kThick;   // 191
    constexpr int kBottom = 200 - kIn - kThick;  // 191
    constexpr int kArmR = 200 - kIn - kArm;      // 178
    constexpr int kDot = 3;

    // top-left
    create_block(sleep_corner_group_, kIn, kIn, kArm, kThick);
    create_block(sleep_corner_group_, kIn, kIn, kThick, kArm);
    create_block(sleep_corner_group_, kIn + kArm + 4, kIn + 2, kDot, kDot);
    create_block(sleep_corner_group_, kIn + 2, kIn + kArm + 4, kDot, kDot);
    // top-right
    create_block(sleep_corner_group_, kArmR, kIn, kArm, kThick);
    create_block(sleep_corner_group_, kRight, kIn, kThick, kArm);
    create_block(sleep_corner_group_, kArmR - 4 - kDot, kIn + 2, kDot, kDot);
    create_block(sleep_corner_group_, kRight + kThick - 2 - kDot, kIn + kArm + 4, kDot, kDot);
    // bottom-left
    create_block(sleep_corner_group_, kIn, kBottom, kArm, kThick);
    create_block(sleep_corner_group_, kIn, kArmR, kThick, kArm);
    create_block(sleep_corner_group_, kIn + kArm + 4, kBottom + kThick - 2 - kDot, kDot, kDot);
    create_block(sleep_corner_group_, kIn + 2, kArmR - 4 - kDot, kDot, kDot);
    // bottom-right
    create_block(sleep_corner_group_, kArmR, kBottom, kArm, kThick);
    create_block(sleep_corner_group_, kRight, kArmR, kThick, kArm);
    create_block(sleep_corner_group_, kArmR - 4 - kDot, kBottom + kThick - 2 - kDot, kDot, kDot);
    create_block(sleep_corner_group_, kRight + kThick - 2 - kDot, kArmR - 4 - kDot, kDot, kDot);

    set_hidden(sleep_corner_group_, true);
}

void DisplayService::BuildPressButton() {
    // A small dark pill that reads "PREMI ->" in white: the call-to-action on
    // the intro screen, pointing at the physical BOOT button. Built as one group
    // so it blinks as a unit.
    press_btn_group_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(press_btn_group_);
    lv_obj_set_style_bg_color(press_btn_group_, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(press_btn_group_, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(press_btn_group_, 0, LV_PART_MAIN);  // sharp corners: the whole system is hard-cornered
    lv_obj_set_style_border_width(press_btn_group_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(press_btn_group_, 0, LV_PART_MAIN);
    lv_obj_remove_flag(press_btn_group_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(press_btn_group_, 98, 28);

    // White block-caps word, with side padding, nudged up 2px.
    press_label_ = lv_label_create(press_btn_group_);
    style_label(press_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_style_text_color(press_label_, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_pos(press_label_, 11, 3);
    lv_obj_set_size(press_label_, 50, 18);

    // White right-arrow as its own layer so only the arrow blinks; gap from word.
    press_arrow_ = lv_obj_create(press_btn_group_);
    style_plain_obj(press_arrow_);
    lv_obj_set_size(press_arrow_, 24, 16);
    lv_obj_set_pos(press_arrow_, 66, 6);
    create_white(press_arrow_, 0, 6, 15, 3);   // shaft
    create_white(press_arrow_, 13, 1, 3, 13);  // head
    create_white(press_arrow_, 16, 4, 3, 7);
    create_white(press_arrow_, 19, 6, 3, 3);   // tip

    set_hidden(press_btn_group_, true);
}

void DisplayService::SpeakTimerThunk(lv_timer_t *timer) {
    auto *self = static_cast<DisplayService *>(lv_timer_get_user_data(timer));
    if (self != nullptr) {
        self->TickSpeaking();
    }
}

void DisplayService::TickSpeaking() {
    // Runs inside lv_timer_handler (LVGL thread, lock already held).
    if (speak_ticks_left_ <= 0) {
        // Settle to a calm single wave and stop refreshing.
        set_hidden(speaker_wave1_, false);
        set_hidden(speaker_wave2_, true);
        set_hidden(speaker_wave3_, true);
        if (speak_timer_ != nullptr) {
            lv_timer_delete(speak_timer_);
            speak_timer_ = nullptr;
        }
        return;
    }
    --speak_ticks_left_;
    speak_phase_ = (speak_phase_ + 1) % 4;  // 0..3 sound-wave count
    set_hidden(speaker_wave1_, speak_phase_ < 1);
    set_hidden(speaker_wave2_, speak_phase_ < 2);
    set_hidden(speaker_wave3_, speak_phase_ < 3);
}

void DisplayService::StartSpeakingAnimation() {
    if (!speaking_active_) {
        return;
    }
    speak_ticks_left_ = 12;  // ~7s of pulsing, then settle (bounded for battery)
    speak_phase_ = 0;
    if (speak_timer_ == nullptr) {
        speak_timer_ = lv_timer_create(&DisplayService::SpeakTimerThunk, 600, this);
    }
}

void DisplayService::StopSpeakingAnimation() {
    if (speak_timer_ != nullptr) {
        lv_timer_delete(speak_timer_);
        speak_timer_ = nullptr;
    }
    speak_ticks_left_ = 0;
    speak_phase_ = 0;
}

void DisplayService::BatteryFlashThunk(lv_timer_t *timer) {
    auto *self = static_cast<DisplayService *>(lv_timer_get_user_data(timer));
    if (self != nullptr) {
        self->TickBatteryFlash();
    }
}

void DisplayService::TickBatteryFlash() {
    // Runs inside lv_timer_handler (LVGL thread, lock already held).
    if (batt_flash_ticks_left_ <= 0) {
        set_hidden(batt_big_flash_, false);  // settle visible, the extra bar persists
        if (batt_flash_timer_ != nullptr) {
            lv_timer_delete(batt_flash_timer_);
            batt_flash_timer_ = nullptr;
        }
        return;
    }
    --batt_flash_ticks_left_;
    batt_flash_on_ = !batt_flash_on_;
    set_hidden(batt_big_flash_, !batt_flash_on_);
}

void DisplayService::StartBatteryFlash() {
    batt_flash_ticks_left_ = 8;  // ~5s of blinking, then settle (bounded for e-ink)
    batt_flash_on_ = false;
    set_hidden(batt_big_flash_, true);
    if (batt_flash_timer_ == nullptr) {
        batt_flash_timer_ = lv_timer_create(&DisplayService::BatteryFlashThunk, 600, this);
    }
}

void DisplayService::StopBatteryFlash() {
    if (batt_flash_timer_ != nullptr) {
        lv_timer_delete(batt_flash_timer_);
        batt_flash_timer_ = nullptr;
    }
    batt_flash_ticks_left_ = 0;
    batt_flash_on_ = false;
    set_hidden(batt_big_flash_, true);
}

void DisplayService::ArrowBlinkThunk(lv_timer_t *timer) {
    auto *self = static_cast<DisplayService *>(lv_timer_get_user_data(timer));
    if (self != nullptr) {
        self->TickArrowBlink();
    }
}

void DisplayService::TickArrowBlink() {
    if (arrow_blink_ticks_left_ <= 0) {
        set_hidden(press_arrow_, false);  // settle visible
        if (arrow_timer_ != nullptr) {
            lv_timer_delete(arrow_timer_);
            arrow_timer_ = nullptr;
        }
        return;
    }
    --arrow_blink_ticks_left_;
    arrow_on_ = !arrow_on_;
    set_hidden(press_arrow_, !arrow_on_);
}

void DisplayService::StartArrowBlink() {
    // RESTING / IDLE screen: render the press-arrow ONCE, statically — NO blink
    // timer. Each blink tick forced an e-ink partial refresh, and after ~30 of
    // them the anti-ghost logic forces a full-screen flash, so the panel looked
    // like it was "refreshing by itself" while idle / in low power. Same rule as
    // the mic/listening screen: never refresh the e-ink on a timer at rest.
    StopArrowBlink();                 // kill any timer left from a previous screen
    arrow_on_ = true;
    arrow_blink_ticks_left_ = 0;
    set_hidden(press_arrow_, false);  // arrow shown, static
}

void DisplayService::StopArrowBlink() {
    if (arrow_timer_ != nullptr) {
        lv_timer_delete(arrow_timer_);
        arrow_timer_ = nullptr;
    }
    arrow_blink_ticks_left_ = 0;
    arrow_on_ = false;
}

void DisplayService::VoiceAnimThunk(lv_timer_t *timer) {
    auto *self = static_cast<DisplayService *>(lv_timer_get_user_data(timer));
    if (self != nullptr) {
        self->TickVoiceAnim();
    }
}

void DisplayService::TickVoiceAnim() {
    // Runs on the LVGL thread, so it keeps pulsing even while the main task is
    // blocked waiting on the OpenAI worker.
    if (voice_anim_ticks_left_ <= 0) {  // settle: everything visible, stop refreshing
        if (voice_anim_is_mic_) {
            set_hidden(mic_wave1_, false);
            set_hidden(mic_wave2_, false);
        } else {
            set_hidden(wait_dot2_, false);
            set_hidden(wait_dot3_, false);
        }
        if (voice_anim_timer_ != nullptr) {
            lv_timer_delete(voice_anim_timer_);
            voice_anim_timer_ = nullptr;
        }
        return;
    }
    --voice_anim_ticks_left_;
    voice_anim_phase_ = (voice_anim_phase_ + 1) % 3;
    if (voice_anim_is_mic_) {
        set_hidden(mic_wave1_, voice_anim_phase_ < 1);
        set_hidden(mic_wave2_, voice_anim_phase_ < 2);
    } else {
        set_hidden(wait_dot2_, voice_anim_phase_ < 1);
        set_hidden(wait_dot3_, voice_anim_phase_ < 2);
    }
}

void DisplayService::StartVoiceAnim(bool is_mic) {
    voice_anim_is_mic_ = is_mic;
    voice_anim_phase_ = 0;
    if (is_mic) {
        // CAPTURE IS LIVE during the mic ("listening") screen. Each animation
        // tick triggers an e-ink PARTIAL REFRESH, and on this single-core
        // ESP32-C6 that long SPI/BUSY-wait steals the core from the I2S capture
        // task (prio 4), overflowing the mic DMA (~90 ms) and dropping ~2/3 of
        // the audio as a periodic stutter -- the 750 ms timer period lined up
        // exactly with the measured maxRead (~735-776 ms) and the per-recording
        // [EPD] flush cadence. So render the listening screen ONCE, statically
        // (both mic waves shown), and start NO refresh timer: zero screen
        // activity while the mic is recording. The "thinking" screen below runs
        // AFTER capture, so it keeps its pulse.
        voice_anim_ticks_left_ = 0;
        set_hidden(mic_wave1_, false);
        set_hidden(mic_wave2_, false);
        return;
    }
    voice_anim_ticks_left_ = 16;  // ~12s of pulsing, then settle (e-ink bounded)
    set_hidden(wait_dot2_, true);
    set_hidden(wait_dot3_, true);
    if (voice_anim_timer_ == nullptr) {
        voice_anim_timer_ = lv_timer_create(&DisplayService::VoiceAnimThunk, 750, this);
    }
}

void DisplayService::StopVoiceAnim() {
    if (voice_anim_timer_ != nullptr) {
        lv_timer_delete(voice_anim_timer_);
        voice_anim_timer_ = nullptr;
    }
    voice_anim_ticks_left_ = 0;
    voice_anim_phase_ = 0;
}

void DisplayService::ResetAuxLayers(bool speaking) {
    // Shared layer reset for every layout: park the auxiliary glyphs and stop the
    // speaker animation unless we are entering the speaking screen.
    if (!speaking) {
        StopSpeakingAnimation();
    }
    StopBatteryFlash();
    StopArrowBlink();
    StopVoiceAnim();
    speaking_active_ = speaking;
    set_hidden(speaker_group_, !speaking);
    set_hidden(wifi_group_, true);
    set_hidden(batt_big_group_, true);
    set_hidden(sleep_corner_group_, true);
    set_hidden(press_btn_group_, true);
    set_hidden(mic_group_, true);
    set_hidden(wait_group_, true);
    set_hidden(status_group_, true);
    set_hidden(chrome_footer_rule_, false);  // shown by default; footer-less screens opt out
}

void DisplayService::SetText(const char *title, const char *body, const char *footer) {
    const bool has_title = (title != nullptr && title[0] != '\0');
    set_hidden(title_label_, !has_title);  // no empty black chip
    if (has_title) {
        char title_upper[64];
        lv_label_set_text(title_label_, UpperAscii(title, title_upper, sizeof(title_upper)));
    }
    lv_label_set_text(body_label_, body != nullptr ? body : "");
    lv_label_set_text(footer_left_, footer != nullptr ? footer : "");
    lv_obj_update_layout(screen_);
}

void DisplayService::SetBattery(uint8_t pct) {
    battery_pct_ = pct;
}

void DisplayService::RenderBattery() {
    if (footer_right_ != nullptr) {
        if (battery_pct_ <= 100) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%u%%", battery_pct_);
            lv_label_set_text(footer_right_, buf);
        } else {
            lv_label_set_text(footer_right_, "");
        }
    }
    if (batt_icon_group_ == nullptr) {
        return;
    }
    if (battery_pct_ <= 100) {
        set_hidden(batt_icon_group_, false);
        int fill = (battery_pct_ * 12 + 50) / 100;  // inner width is 12px
        if (fill < 1) {
            fill = 1;
        } else if (fill > 12) {
            fill = 12;
        }
        if (batt_icon_fill_ != nullptr) {
            lv_obj_set_width(batt_icon_fill_, fill);
        }
    } else {
        set_hidden(batt_icon_group_, true);
    }
}

void DisplayService::LayoutBoot() {
    ResetAuxLayers(false);
    set_hidden(splash_group_, false);
    set_hidden(chrome_group_, true);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, false);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);
    set_hidden(batt_icon_group_, true);

    lv_image_set_scale(mascot_big_, 256);
    lv_obj_set_pos(mascot_big_, 52, 8);  // 96x96 boot logo, centred; grows up to the same y104 baseline

    style_label(title_label_, &bisc8_font_title, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(title_label_, 0, 110);
    lv_obj_set_size(title_label_, 200, 34);

    style_label(body_label_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_align(body_label_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(body_label_, 0, 142);
    lv_obj_set_size(body_label_, 200, 18);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 164);
    lv_obj_set_size(footer_left_, 200, 18);
}

void DisplayService::LayoutIntro() {
    ResetAuxLayers(false);
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);
    set_hidden(batt_icon_group_, true);
    set_hidden(press_btn_group_, false);

    apply_title_chip(title_label_, &bisc8_font_small, 30);

    // The question prompt, centered.
    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_align(body_label_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(body_label_, 0, 58);
    lv_obj_set_size(body_label_, 200, 58);

    // The dark "PREMI ->" pill at the right edge (5px), pointing at the lateral
    // BOOT button on the right.
    lv_obj_set_pos(press_btn_group_, 97, 130);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 178);
    lv_obj_set_size(footer_left_, 200, 20);
}

void DisplayService::LayoutMessage() {
    ResetAuxLayers(false);
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, false);

    apply_title_chip(title_label_, &bisc8_font_small, 30);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_width(body_label_, 176);
    lv_obj_set_height(body_label_, LV_SIZE_CONTENT);
    // Center the body in the band BETWEEN the title chip and the footer, not in
    // the whole screen: a tall 3-4 line message centered on the screen rides up
    // and touches the title chip ("stuck to the title"). Shifting the centring
    // anchor down by kMessageBodyDrop keeps short bodies balanced and tall ones
    // clear of the title.
    lv_obj_set_align(body_label_, LV_ALIGN_CENTER);
    lv_obj_set_y(body_label_, kMessageBodyDrop);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 100, 16);

    style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_pos(footer_right_, 110, 182);
    lv_obj_set_size(footer_right_, 58, 16);
    lv_obj_set_pos(batt_icon_group_, 172, 184);
    RenderBattery();
}

void DisplayService::LayoutStatusQr() {
    ResetAuxLayers(false);
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, false);
    set_hidden(body_label_, true);     // the QR group replaces the body
    set_hidden(footer_left_, true);
    set_hidden(footer_right_, true);
    set_hidden(batt_icon_group_, true);
    set_hidden(status_group_, false);
    set_hidden(chrome_footer_rule_, true);  // no footer here -> drop the bottom line

    // State word ("CONNESSO") as the title chip; the caller sets the text.
    apply_title_chip(title_label_, &bisc8_font_small, 30);
}

void DisplayService::LayoutResponso() {
    ResetAuxLayers(false);
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, true);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, false);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_width(body_label_, 184);
    lv_obj_set_height(body_label_, LV_SIZE_CONTENT);
    lv_obj_set_align(body_label_, LV_ALIGN_CENTER);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 96, 16);

    style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_pos(footer_right_, 106, 182);
    lv_obj_set_size(footer_right_, 62, 16);
    lv_obj_set_pos(batt_icon_group_, 172, 184);
    RenderBattery();
}

void DisplayService::LayoutSpeaking() {
    ResetAuxLayers(true);  // reveals speaker_group_
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, true);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, false);

    lv_obj_set_pos(speaker_group_, 84, 32);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_align(body_label_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_width(body_label_, 176);
    lv_obj_set_height(body_label_, LV_SIZE_CONTENT);
    lv_obj_set_pos(body_label_, 12, 80);  // anchored below the speaker glyph

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 100, 16);
    style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_pos(footer_right_, 110, 182);
    lv_obj_set_size(footer_right_, 58, 16);
    lv_obj_set_pos(batt_icon_group_, 172, 184);
    RenderBattery();
}

void DisplayService::LayoutGlyphMessage() {
    ResetAuxLayers(false);  // glyph un-hidden by the caller
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, false);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, true);
    set_hidden(title_label_, true);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, false);

    // Body anchored just below the glyph so the glyph+text read as one centered
    // unit (the glyph is placed at y~34 by the caller). Avoids the old "glyph
    // floating high, text floating low" gap.
    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_align(body_label_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_width(body_label_, 188);
    lv_obj_set_height(body_label_, LV_SIZE_CONTENT);
    lv_obj_set_pos(body_label_, 6, 84);

    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(footer_left_, 8, 182);
    lv_obj_set_size(footer_left_, 100, 16);
    style_label(footer_right_, &bisc8_font_small, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_pos(footer_right_, 110, 182);
    lv_obj_set_size(footer_right_, 58, 16);
    lv_obj_set_pos(batt_icon_group_, 172, 184);
    RenderBattery();
}

void DisplayService::LayoutLowPower() {
    // Sleep card: product name up top, a big centered mascot, a drowsy "Zzz"
    // line, a tap-to-wake hint, and biscuit-corner ornaments framing it all.
    ResetAuxLayers(false);
    set_hidden(splash_group_, true);
    set_hidden(chrome_group_, true);
    set_hidden(arrow_group_, true);
    set_hidden(mascot_big_, false);
    set_hidden(title_label_, false);
    set_hidden(body_label_, false);
    set_hidden(footer_left_, false);
    set_hidden(footer_right_, true);
    set_hidden(batt_icon_group_, true);
    set_hidden(sleep_corner_group_, false);

    // Product name as a real title chip (big), with room above the logo.
    // Title detached a touch from the top, with a small gap before the logo.
    apply_title_chip(title_label_, &bisc8_font_title, 10);

    lv_image_set_scale(mascot_big_, 208);  // 96px asset * 208/256 = ~78px; top-left pivot -> (200-78)/2
    lv_obj_set_pos(mascot_big_, 61, 50);

    style_label(body_label_, &bisc8_font_body, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_align(body_label_, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(body_label_, 0, 132);
    lv_obj_set_size(body_label_, 200, 20);

    // "premi per svegliarmi" lifted up, with clear air above the "Zzz" line.
    style_label(footer_left_, &bisc8_font_small, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_pos(footer_left_, 0, 156);
    lv_obj_set_size(footer_left_, 200, 20);
}

void DisplayService::ShowBoot() {
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
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
        char upper[32];
        lv_label_set_text(title_label_, UpperAscii(strings.intro_title, upper, sizeof(upper)));
        lv_label_set_text(body_label_, strings.intro_body);
        lv_label_set_text(press_label_, UpperAscii(strings.intro_press, upper, sizeof(upper)));
        lv_label_set_text(footer_left_, strings.intro_footer);
        lv_obj_update_layout(screen_);
        StartArrowBlink();
        Lvgl_unlock();
    }
}

void DisplayService::ShowStatus(const WifiStatus &status, Language language) {
    const LocalizedStrings &strings = StringsFor(language);

    if (status.setup_active && !status.online) {
        // Pressing PWR while in setup used to render "SSID | IP" cramped onto one
        // line. Render the exact same balanced setup screen instead.
        ShowWifiSetup(status.setup_ssid.c_str(), status.setup_url.c_str(), language);
        return;
    }

    if (Lvgl_lock(-1)) {
        // A QR has fine detail; force a full refresh so e-ink ghosting can't make
        // it unscannable.
        RequestFullRefresh();
        if (status.online) {
            const char *ssid = status.connected_ssid.empty() ? "Wi-Fi" : status.connected_ssid.c_str();
            const char *ip = status.connected_ip.empty() ? "0.0.0.0" : status.connected_ip.c_str();
            LayoutStatusQr();
            char title_buf[32];
            lv_label_set_text(title_label_,
                              UpperAscii(strings.status_connected_title, title_buf, sizeof(title_buf)));
            lv_label_set_text(status_net_label_, ssid);
            lv_label_set_text(status_ip_label_, ip);
            char url[80];
            snprintf(url, sizeof(url), "http://%s", ip);
            lv_qrcode_update(status_qr_, url, strlen(url));
            lv_obj_update_layout(screen_);
        } else {
            LayoutMessage();
            SetText(strings.status_disconnected_title, strings.status_offline_body,
                    strings.status_footer);
        }
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
    const char *net = (ssid == nullptr || ssid[0] == '\0') ? "Bisc8-XXXX" : ssid;
    char address[32];
    SetupDisplayAddress(url, address, sizeof(address));
    if (Lvgl_lock(-1)) {
        // Same clean QR layout as the connected status screen, for consistency.
        // The QR joins the OPEN setup AP: scan -> the phone connects to <ssid> and
        // the captive portal opens. The SSID + address below are the manual path.
        // Full refresh so the QR stays crisp/scannable.
        RequestFullRefresh();
        LayoutStatusQr();
        char title_upper[48];
        lv_label_set_text(title_label_,
                          UpperAscii(strings.wifi_setup_title, title_upper, sizeof(title_upper)));
        lv_label_set_text(status_net_label_, net);
        lv_label_set_text(status_ip_label_, address);
        char join[96];
        snprintf(join, sizeof(join), "WIFI:T:nopass;S:%s;;", net);
        lv_qrcode_update(status_qr_, join, strlen(join));
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowWifiConnectFailed(const char *ssid, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char body[96];
    snprintf(body, sizeof(body), strings.wifi_connect_failed_body,
             (ssid == nullptr || ssid[0] == '\0') ? "Wi-Fi" : ssid);
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutMessage();
        SetText(strings.wifi_connecting_title, body, "");
        Lvgl_unlock();
    }
}

void DisplayService::ShowFortune(const char *fortune, size_t index, size_t count) {
    char counter[24];
    snprintf(counter, sizeof(counter), "%u/%u", static_cast<unsigned>(index + 1), static_cast<unsigned>(count));
    if (Lvgl_lock(-1)) {
        // Cycling fortunes with the button must feel instant, so this is a
        // partial refresh. The periodic anti-ghost full (every
        // kMaxPartialsBeforeFull partials) clears any accumulated ghosting.
        LayoutResponso();
        lv_label_set_text(body_label_, fortune != nullptr ? fortune : "");
        lv_label_set_text(footer_left_, counter);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceListening(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        // No full refresh here: the full e-ink flash on BOOT-press looked like a
        // reset. Partial refresh into the mic glyph is calmer.
        LayoutGlyphMessage();
        set_hidden(mic_group_, false);
        lv_obj_set_pos(mic_group_, 86, 30);
        lv_label_set_text(body_label_, strings.listening_body);
        lv_label_set_text(footer_left_, strings.voice_footer);
        lv_obj_update_layout(screen_);
        StartVoiceAnim(true);  // pulse the mic sound-waves
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
        LayoutGlyphMessage();
        set_hidden(wait_group_, false);
        lv_obj_set_pos(wait_group_, 84, 56);
        lv_label_set_text(body_label_, strings.thinking_body);
        lv_label_set_text(footer_left_, "OpenAI");
        lv_obj_update_layout(screen_);
        StartVoiceAnim(false);  // pulse the "..." dots while OpenAI works
        Lvgl_unlock();
    }
}

void DisplayService::ShowVoiceSpeaking(const char *screen_answer, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char code[4];
    CopyLangCode(strings.code, code, sizeof(code));
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();  // voice responso reveal, like the printed one
        LayoutSpeaking();
        lv_label_set_text(body_label_, screen_answer == nullptr ? strings.speaking_body : screen_answer);
        lv_label_set_text(footer_left_, code);
        lv_obj_update_layout(screen_);
        StartSpeakingAnimation();  // pulses the speaker until audio playback ends
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
        char upper[32];
        lv_label_set_text(title_label_, UpperAscii(strings.intro_title, upper, sizeof(upper)));
        lv_label_set_text(body_label_, strings.intro_body);
        lv_label_set_text(press_label_, UpperAscii(strings.intro_press, upper, sizeof(upper)));
        lv_label_set_text(footer_left_, strings.intro_footer);
        lv_obj_update_layout(screen_);
        StartArrowBlink();
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

void DisplayService::ShowError(const char *message, const char *code, Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        LayoutMessage();
        char footer[24];
        if (code != nullptr && code[0] != '\0') {
            snprintf(footer, sizeof(footer), "cod. %s", code);  // user reports this back
        } else {
            snprintf(footer, sizeof(footer), "%s", strings.error_footer);
        }
        SetText(strings.error_title, message, footer);
        Lvgl_unlock();
    }
}

void DisplayService::ShowNoWifi(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char code[4];
    CopyLangCode(strings.code, code, sizeof(code));
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutGlyphMessage();
        set_hidden(wifi_group_, false);
        lv_obj_set_pos(wifi_group_, 75, 34);
        lv_label_set_text(body_label_, strings.no_wifi_body);
        lv_label_set_text(footer_left_, code);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowLowBattery(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char code[4];
    CopyLangCode(strings.code, code, sizeof(code));
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutGlyphMessage();
        set_hidden(batt_big_group_, false);
        lv_obj_set_pos(batt_big_group_, 61, 34);
        StartBatteryFlash();
        lv_label_set_text(body_label_, strings.low_battery_body);
        lv_label_set_text(footer_left_, code);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowCriticalLowBattery(Language language) {
    // The last thing drawn before the <=10% auto power-off. Big battery glyph +
    // a "powering off to save the cell" message. Rendered ONCE, statically (no
    // blink timer) — we are about to deep-sleep, so no need to keep refreshing.
    const LocalizedStrings &strings = StringsFor(language);
    char code[4];
    CopyLangCode(strings.code, code, sizeof(code));
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutGlyphMessage();
        set_hidden(batt_big_group_, false);
        lv_obj_set_pos(batt_big_group_, 61, 34);
        lv_label_set_text(body_label_, strings.low_battery_off_body);
        lv_label_set_text(footer_left_, code);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::ShowFirstRun(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    char code[4];
    CopyLangCode(strings.code, code, sizeof(code));
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutMessage();
        SetText(strings.first_run_title, strings.first_run_body, code);
        Lvgl_unlock();
    }
}

void DisplayService::ShowLowPower(Language language) {
    const LocalizedStrings &strings = StringsFor(language);
    if (Lvgl_lock(-1)) {
        RequestFullRefresh();
        LayoutLowPower();
        lv_label_set_text(title_label_, "Bisc8");
        lv_label_set_text(body_label_, "Zzz ...zzzz....");
        lv_label_set_text(footer_left_, strings.low_power_body);
        lv_obj_update_layout(screen_);
        Lvgl_unlock();
    }
}

void DisplayService::OnPlaybackState(bool active) {
    // Called from the AudioService playback thread; gate on the speaking screen
    // so cues on other screens never wake the panel.
    if (Lvgl_lock(-1)) {
        if (speaking_active_) {
            if (active) {
                StartSpeakingAnimation();
            } else {
                StopSpeakingAnimation();
                set_hidden(speaker_wave1_, false);
                set_hidden(speaker_wave2_, true);
                set_hidden(speaker_wave3_, true);
            }
        }
        Lvgl_unlock();
    }
}

void DisplayService::OnListeningState(bool active) {
    // Reserved seam for a future "ti ascolto" listening animation. The audio
    // recording observer already drives this hook; the listening screen
    // (ShowVoiceListening) renders a static state for now. Intentionally a
    // no-op until the microphone-input UI is built.
    (void)active;
}

}  // namespace bisc8
