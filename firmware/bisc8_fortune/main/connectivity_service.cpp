#include "connectivity_service.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_netif_ip_addr.h>
#include <esp_system.h>
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

constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;   // got an IP (DHCP done)
constexpr EventBits_t WIFI_FAILED_BIT = BIT1;      // explicit disconnect/auth failure
constexpr EventBits_t WIFI_ASSOCIATED_BIT = BIT2;  // L2 association up, DHCP in flight
constexpr uint16_t kMaxScanRecords = 20;
constexpr const char *kSetupUrl = "http://192.168.4.1";
constexpr uint32_t kSetupIpHostOrder = 0xC0A80401;

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

void OnWifiEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (g_wifi_events == nullptr) {
        return;
    }
    auto *service = static_cast<ConnectivityService *>(arg);
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        char ip[16] = {};
        const auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        if (event != nullptr) {
            snprintf(ip, sizeof(ip), IPSTR, IP2STR(&event->ip_info.ip));
        }
        if (service != nullptr) {
            service->UpdateConnectedIp(ip);
        }
        xEventGroupSetBits(g_wifi_events, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        // Associated; the 4-way handshake passed and DHCP is now in flight.
        // ConnectToNetwork uses this to stop kicking the association.
        xEventGroupSetBits(g_wifi_events, WIFI_ASSOCIATED_BIT);
        DebugSerial::LogAlways("[WIFI]", "associated; waiting for DHCP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (service != nullptr) {
            service->MarkDisconnected();
        }
        xEventGroupClearBits(g_wifi_events, WIFI_ASSOCIATED_BIT);
        xEventGroupSetBits(g_wifi_events, WIFI_FAILED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        const auto *event = static_cast<wifi_event_ap_staconnected_t *>(event_data);
        const uint8_t zero_mac[6] = {};
        const uint8_t *mac = event == nullptr ? zero_mac : event->mac;
        DebugSerial::LogAlways("[WIFI]",
                               "setup client join aid=%u mac=" MACSTR " free_heap=%u",
                               event == nullptr ? 0 : static_cast<unsigned>(event->aid),
                               MAC2STR(mac),
                               static_cast<unsigned>(esp_get_free_heap_size()));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        const auto *event = static_cast<wifi_event_ap_stadisconnected_t *>(event_data);
        const uint8_t zero_mac[6] = {};
        const uint8_t *mac = event == nullptr ? zero_mac : event->mac;
        DebugSerial::LogAlways("[WIFI]",
                               "setup client leave aid=%u reason=%u mac=" MACSTR " free_heap=%u",
                               event == nullptr ? 0 : static_cast<unsigned>(event->aid),
                               event == nullptr ? 0 : static_cast<unsigned>(event->reason),
                               MAC2STR(mac),
                               static_cast<unsigned>(esp_get_free_heap_size()));
    }
}

}  // namespace

void ConnectivityService::UpdateConnectedIp(const char *ip) {
    status_.connected_ip = ip == nullptr ? "" : ip;
    DebugSerial::LogAlways("[WIFI]",
                           "got sta ip=%s",
                           status_.connected_ip.empty() ? "unknown" : status_.connected_ip.c_str());
}

void ConnectivityService::MarkDisconnected() {
    online_ = false;
    status_.online = false;
    status_.connected_ip.clear();
}

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

esp_err_t ConnectivityService::ConnectToNetwork(const char *ssid, const char *password, DisplayService &display, Language language, bool show_progress) {
    if (ssid == nullptr || password == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    online_ = false;
    status_.online = false;
    status_.ssid_attempt = ssid;
    status_.connected_ssid.clear();
    status_.connected_ip.clear();
    esp_wifi_disconnect();
    xEventGroupClearBits(g_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT | WIFI_ASSOCIATED_BIT);

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

    const int budget_s = static_cast<int>(kWifiAttemptTimeoutMs / 1000);
    constexpr int kKickEverySec = 3;  // re-issue connect if the association stalls
    DebugSerial::LogAlways("[WIFI]", "connecting ssid=%s budget_ms=%lu", ssid, static_cast<unsigned long>(kWifiAttemptTimeoutMs));
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        return err;
    }

    int attempts = 1;
    for (int sec = 1; sec <= budget_s; ++sec) {
        if (show_progress) {
            display.ShowWifiConnecting(ssid, budget_s - sec + 1, language);
        }
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
            DebugSerial::LogAlways("[WIFI]",
                                   "connected ssid=%s ip=%s attempts=%d",
                                   ssid,
                                   status_.connected_ip.empty() ? "pending" : status_.connected_ip.c_str(),
                                   attempts);
            return ESP_OK;
        }
        // The first association after boot stalls ~half the time on the C6: it
        // neither connects nor reports a disconnect. Re-kick esp_wifi_connect on
        // an explicit failure, or every few stalled seconds; a prompt retry
        // almost always associates. This is what kept dropping us into setup.
        //
        // BUT: once we are ASSOCIATED, the radio is fine and we are only waiting
        // for DHCP. Kicking there tears down a healthy association and restarts
        // the whole join, so a router whose assoc+DHCP takes >3 s in total could
        // never finish inside the budget (slow TIM/mesh gear shows exactly this).
        const bool failed = (bits & WIFI_FAILED_BIT) != 0;
        const bool associated = (bits & WIFI_ASSOCIATED_BIT) != 0;
        const bool stalled = (sec % kKickEverySec == 0) && !associated;
        if ((failed || stalled) && sec < budget_s) {
            esp_wifi_disconnect();
            xEventGroupClearBits(g_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT | WIFI_ASSOCIATED_BIT);
            esp_wifi_connect();
            ++attempts;
            DebugSerial::LogAlways("[WIFI]", "reconnect kick ssid=%s attempt=%d reason=%s",
                                   ssid, attempts, failed ? "disconnect" : "stall");
        }
    }

    esp_wifi_disconnect();
    online_ = false;
    status_.online = false;
    status_.connected_ip.clear();
    DebugSerial::LogAlways("[WIFI]", "connection timeout ssid=%s attempts=%d", ssid, attempts);
    return ESP_ERR_TIMEOUT;
}

esp_err_t ConnectivityService::TryKnownNetworks(const DeviceSettings &settings, DisplayService &display, Language language, bool show_progress) {
    dns_.Stop();
    online_ = false;
    status_ = WifiStatus{};
    last_attempt_ssid_.clear();

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
        // A scan failure must not block a direct connect: the radio can still
        // associate even when the scan errored or came back empty.
        DebugSerial::LogAlways("[WIFI]", "scan failed: %s (trying saved networks anyway)", esp_err_to_name(err));
        for (size_t i = 0; i < kMaxWifiCredentials; ++i) {
            visible[i] = false;
        }
    } else if (!found) {
        DebugSerial::LogAlways("[WIFI]", "no saved SSID in scan results; trying saved networks anyway");
    }

    // Two passes: prefer SSIDs the scan actually saw, but never let a scan miss
    // alone drop us to the setup portal. A present AP can be absent from a single
    // scan (congested 2.4GHz, hidden, momentary), so still try the rest after.
    for (int pass = 0; pass < 2; ++pass) {
        const bool want_visible = (pass == 0);
        for (size_t i = 0; i < settings.wifi_count && i < kMaxWifiCredentials; ++i) {
            if (visible[i] != want_visible) {
                continue;
            }
            last_attempt_ssid_ = settings.wifi[i].ssid;
            err = ConnectToNetwork(settings.wifi[i].ssid.c_str(), settings.wifi[i].password.c_str(), display, language, show_progress);
            if (err == ESP_OK) {
                return ESP_OK;
            }
        }
    }

    online_ = false;
    status_.online = false;
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ConnectivityService::TestCredentials(const char *ssid, const char *password, DisplayService &display, Language language) {
    esp_err_t err = EnsureInitialized();
    if (err != ESP_OK) {
        return err;
    }
    return ConnectToNetwork(ssid, password, display, language, false);
}

void ConnectivityService::BuildSetupSsid(char *ssid, size_t ssid_len) const {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(ssid, ssid_len, "Bisc8-%02X%02X", mac[4], mac[5]);
}

esp_err_t ConnectivityService::StartSetupPortal(DisplayService &display, WebPortal &portal, Language language, bool show_display) {
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
    ap_config.ap.max_connection = 2;
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
    status_.connected_ssid.clear();
    status_.connected_ip.clear();
    status_.setup_ssid = setup_ssid;
    status_.setup_url = kSetupUrl;
    online_ = false;

    DebugSerial::LogAlways("[WIFI]", "starting SoftAP %s and setup portal at %s", setup_ssid, kSetupUrl);
    err = dns_.Start(kSetupIpHostOrder);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WIFI]", "captive DNS failed: %s", esp_err_to_name(err));
    }
    if (show_display) {
        display.ShowWifiSetup(setup_ssid, kSetupUrl, language);
    }
    return portal.Start();
}

bool ConnectivityService::Online() const {
    return online_;
}

const WifiStatus &ConnectivityService::Status() const {
    return status_;
}

const std::string &ConnectivityService::LastAttemptSsid() const {
    return last_attempt_ssid_;
}

}  // namespace bisc8
