#include "connectivity_service.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <lwip/ip4_addr.h>

#include "app_config.h"
#include "debug_serial.h"
#include "display_service.h"
#include "web_portal.h"

namespace bisc8 {
namespace {

constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
constexpr EventBits_t WIFI_FAILED_BIT = BIT1;
constexpr uint16_t kMaxScanRecords = 20;
constexpr const char *kSetupUrl = "http://192.168.4.1";

EventGroupHandle_t g_wifi_events = nullptr;
esp_netif_t *g_sta_netif = nullptr;
esp_netif_t *g_ap_netif = nullptr;

bool CopyWifiString(char *dst, size_t dst_len, const std::string &src) {
    if (dst == nullptr || dst_len == 0 || src.size() >= dst_len) {
        return false;
    }
    std::memset(dst, 0, dst_len);
    std::memcpy(dst, src.c_str(), src.size());
    return true;
}

void OnWifiEvent(void *, esp_event_base_t event_base, int32_t event_id, void *) {
    if (g_wifi_events == nullptr) {
        return;
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(g_wifi_events, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(g_wifi_events, WIFI_FAILED_BIT);
    }
}

}  // namespace

esp_err_t ConnectivityService::EnsureInitialized() {
    if (initialized_) {
        return ESP_OK;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (g_sta_netif == nullptr) {
        g_sta_netif = esp_netif_create_default_wifi_sta();
    }
    if (g_ap_netif == nullptr) {
        g_ap_netif = esp_netif_create_default_wifi_ap();
    }
    if (g_sta_netif == nullptr || g_ap_netif == nullptr) {
        return ESP_FAIL;
    }

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    g_wifi_events = xEventGroupCreate();
    if (g_wifi_events == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &OnWifiEvent, this);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &OnWifiEvent, this);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    initialized_ = true;
    return ESP_OK;
}

esp_err_t ConnectivityService::ScanKnownNetworks(const DeviceSettings &settings, bool visible[kMaxWifiCredentials], bool *found) {
    if (found == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *found = false;
    for (size_t i = 0; i < kMaxWifiCredentials; ++i) {
        visible[i] = false;
    }

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return err;
    }

    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = true;
    err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        return err;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        return err;
    }
    ap_count = std::min(ap_count, kMaxScanRecords);
    wifi_ap_record_t records[kMaxScanRecords] = {};
    err = esp_wifi_scan_get_ap_records(&ap_count, records);
    if (err != ESP_OK) {
        return err;
    }

    for (size_t i = 0; i < settings.wifi_count && i < kMaxWifiCredentials; ++i) {
        for (uint16_t j = 0; j < ap_count; ++j) {
            const char *seen_ssid = reinterpret_cast<const char *>(records[j].ssid);
            if (settings.wifi[i].ssid == seen_ssid) {
                visible[i] = true;
                *found = true;
                break;
            }
        }
    }

    return ESP_OK;
}

esp_err_t ConnectivityService::ConnectToNetwork(const char *ssid, const char *password, DisplayService &display) {
    if (ssid == nullptr || password == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    status_.ssid_attempt = ssid;
    esp_wifi_disconnect();
    xEventGroupClearBits(g_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT);

    wifi_config_t sta_config = {};
    if (!CopyWifiString(reinterpret_cast<char *>(sta_config.sta.ssid), sizeof(sta_config.sta.ssid), ssid)) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (!CopyWifiString(reinterpret_cast<char *>(sta_config.sta.password), sizeof(sta_config.sta.password), password)) {
        return ESP_ERR_INVALID_SIZE;
    }
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (err != ESP_OK) {
        return err;
    }

    DebugSerial::LogAlways("[WIFI]", "connecting ssid=%s timeout_ms=%lu", ssid, static_cast<unsigned long>(kWifiAttemptTimeoutMs));
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        return err;
    }

    const int seconds = static_cast<int>(kWifiAttemptTimeoutMs / 1000);
    for (int remaining = seconds; remaining > 0; --remaining) {
        display.ShowWifiConnecting(ssid, remaining);
        EventBits_t bits = xEventGroupWaitBits(g_wifi_events,
                                               WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(1000));
        if ((bits & WIFI_CONNECTED_BIT) != 0) {
            online_ = true;
            status_.online = true;
            status_.connected_ssid = ssid;
            status_.ssid_attempt.clear();
            DebugSerial::LogAlways("[WIFI]", "connected ssid=%s", ssid);
            return ESP_OK;
        }
        if ((bits & WIFI_FAILED_BIT) != 0) {
            break;
        }
    }

    esp_wifi_disconnect();
    DebugSerial::LogAlways("[WIFI]", "connection timeout ssid=%s", ssid);
    return ESP_ERR_TIMEOUT;
}

esp_err_t ConnectivityService::TryKnownNetworks(const DeviceSettings &settings, DisplayService &display) {
    online_ = false;
    status_ = WifiStatus{};

    esp_err_t err = EnsureInitialized();
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WIFI]", "init failed: %s", esp_err_to_name(err));
        return err;
    }

    DebugSerial::LogAlways("[WIFI]",
                           "scan known networks saved=%u max=%u attempt_timeout_ms=%lu",
                           static_cast<unsigned>(settings.wifi_count),
                           static_cast<unsigned>(kMaxWifiCredentials),
                           static_cast<unsigned long>(kWifiAttemptTimeoutMs));

    bool visible[kMaxWifiCredentials] = {};
    bool found = false;
    err = ScanKnownNetworks(settings, visible, &found);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WIFI]", "scan failed: %s", esp_err_to_name(err));
        return err;
    }
    if (!found) {
        DebugSerial::LogAlways("[WIFI]", "no saved SSID found in scan results");
        return ESP_ERR_NOT_FOUND;
    }

    for (size_t i = 0; i < settings.wifi_count && i < kMaxWifiCredentials; ++i) {
        if (!visible[i]) {
            continue;
        }
        err = ConnectToNetwork(settings.wifi[i].ssid.c_str(), settings.wifi[i].password.c_str(), display);
        if (err == ESP_OK) {
            return ESP_OK;
        }
    }

    online_ = false;
    status_.online = false;
    return ESP_ERR_NOT_FOUND;
}

void ConnectivityService::BuildSetupSsid(char *ssid, size_t ssid_len) const {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(ssid, ssid_len, "Bisc8-%02X%02X", mac[4], mac[5]);
}

esp_err_t ConnectivityService::StartSetupPortal(DisplayService &display, WebPortal &portal) {
    esp_err_t err = EnsureInitialized();
    if (err != ESP_OK) {
        return err;
    }

    char setup_ssid[sizeof("Bisc8-XXXX")] = {};
    BuildSetupSsid(setup_ssid, sizeof(setup_ssid));

    esp_netif_ip_info_t ip_info = {};
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(g_ap_netif);
    esp_netif_set_ip_info(g_ap_netif, &ip_info);
    esp_netif_dhcps_start(g_ap_netif);

    wifi_config_t ap_config = {};
    CopyWifiString(reinterpret_cast<char *>(ap_config.ap.ssid), sizeof(ap_config.ap.ssid), setup_ssid);
    ap_config.ap.ssid_len = std::strlen(setup_ssid);
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_OPEN;

    err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        return err;
    }

    status_.online = false;
    status_.setup_active = true;
    status_.setup_ssid = setup_ssid;
    status_.setup_url = kSetupUrl;
    online_ = false;

    DebugSerial::LogAlways("[WIFI]", "starting SoftAP %s and setup portal at %s", setup_ssid, kSetupUrl);
    display.ShowWifiSetup();
    return portal.Start();
}

bool ConnectivityService::Online() const {
    return online_;
}

const WifiStatus &ConnectivityService::Status() const {
    return status_;
}

}  // namespace bisc8
