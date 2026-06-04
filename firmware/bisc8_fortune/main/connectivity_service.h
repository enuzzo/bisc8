#pragma once

#include <esp_err.h>

#include <string>

#include "captive_dns_service.h"

namespace bisc8 {

struct DeviceSettings;
class DisplayService;
class WebPortal;

struct WifiStatus {
    bool online = false;
    bool setup_active = false;
    std::string ssid_attempt;
    std::string connected_ssid;
    std::string setup_ssid;
    std::string setup_url;
};

class ConnectivityService {
public:
    esp_err_t TryKnownNetworks(const DeviceSettings &settings, DisplayService &display);
    esp_err_t StartSetupPortal(DisplayService &display, WebPortal &portal);
    bool Online() const;
    const WifiStatus &Status() const;

private:
    esp_err_t EnsureInitialized();
    esp_err_t ScanKnownNetworks(const DeviceSettings &settings, bool *visible, bool *found);
    esp_err_t ConnectToNetwork(const char *ssid, const char *password, DisplayService &display);
    void BuildSetupSsid(char *ssid, size_t ssid_len) const;

    bool online_ = false;
    bool initialized_ = false;
    CaptiveDnsService dns_;
    WifiStatus status_;
};

}  // namespace bisc8
