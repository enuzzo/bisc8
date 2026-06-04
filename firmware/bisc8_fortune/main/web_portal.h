#pragma once

#include <stddef.h>

#include <string>

#include <esp_err.h>
#include <esp_http_server.h>

namespace bisc8 {

struct DeviceSettings;
struct WifiStatus;
class ConfigStore;

using HttpHandler = esp_err_t (*)(httpd_req_t *req);

const char *const *PortalRoutes(size_t *count);
const char *const *CaptivePortalProbePaths(size_t *count);
const char *PortalIndexHtml();

class WebPortal {
public:
    void BindStatus(const WifiStatus *wifi);
    void BindConfig(ConfigStore *config_store, DeviceSettings *settings);
    void GeneratePairingPin();
    const std::string &PairingPin() const;
    esp_err_t Start();
    void Stop();
    bool Running() const;

private:
    static esp_err_t HandleIndex(httpd_req_t *req);
    static esp_err_t HandleStatus(httpd_req_t *req);
    static esp_err_t HandleWifiScan(httpd_req_t *req);
    static esp_err_t HandleWifiCredentials(httpd_req_t *req);
    static esp_err_t HandleSettings(httpd_req_t *req);
    static esp_err_t HandleOpenAi(httpd_req_t *req);
    static esp_err_t HandleEmail(httpd_req_t *req);
    static esp_err_t HandleReset(httpd_req_t *req);
    static esp_err_t HandleCaptiveRedirect(httpd_req_t *req);
    esp_err_t RegisterRoute(const char *uri, httpd_method_t method, HttpHandler handler);
    esp_err_t SendStatusJson(httpd_req_t *req) const;
    esp_err_t SendWifiScanJson(httpd_req_t *req) const;
    esp_err_t SaveCurrentSettings();

    httpd_handle_t server_ = nullptr;
    const WifiStatus *wifi_ = nullptr;
    DeviceSettings *settings_ = nullptr;
    ConfigStore *config_store_ = nullptr;
    std::string setup_pin_;
    bool running_ = false;
};

}  // namespace bisc8
