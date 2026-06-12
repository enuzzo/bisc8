#pragma once

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <esp_http_server.h>

namespace bisc8 {

struct DeviceSettings;
struct WifiStatus;
class ConfigStore;
class ConnectivityService;
class DisplayService;

using HttpHandler = esp_err_t (*)(httpd_req_t *req);

const char *const *PortalRoutes(size_t *count);
const char *const *CaptivePortalProbePaths(size_t *count);
const char *PortalIndexHtml();

class WebPortal {
public:
    void BindStatus(const WifiStatus *wifi);
    void BindConfig(ConfigStore *config_store, DeviceSettings *settings);
    void BindRuntime(ConnectivityService *connectivity, DisplayService *display);
    esp_err_t Start();
    void Stop();
    bool Running() const;
    // True if the config page hit the server since the last call (and clears it).
    // The main loop uses this to stay awake while someone is configuring.
    bool ConsumeActivity();

private:
    static esp_err_t HandleIndex(httpd_req_t *req);
    static esp_err_t HandleStatus(httpd_req_t *req);
    static esp_err_t HandleWifiScan(httpd_req_t *req);
    static esp_err_t HandleWifiCredentials(httpd_req_t *req);
    static esp_err_t HandleSettings(httpd_req_t *req);
    static esp_err_t HandleOpenAi(httpd_req_t *req);
    static esp_err_t HandleEmail(httpd_req_t *req);
    static esp_err_t HandleReset(httpd_req_t *req);
    static esp_err_t HandleReboot(httpd_req_t *req);
    // Full-quality WAV pulls from the flash spool (mic question / TTS answer),
    // for bench QA and "what did it actually hear" checks over the LAN portal.
    static esp_err_t HandleQuestionAudio(httpd_req_t *req);
    static esp_err_t HandleAnswerAudio(httpd_req_t *req);
    static esp_err_t StreamSpoolWav(httpd_req_t *req, uint32_t base_offset);
    static esp_err_t HandleCaptiveRedirect(httpd_req_t *req);
    esp_err_t RegisterRoute(const char *uri, httpd_method_t method, HttpHandler handler);
    esp_err_t SendStatusJson(httpd_req_t *req) const;
    esp_err_t SendWifiScanJson(httpd_req_t *req) const;
    esp_err_t SaveCurrentSettings();

    httpd_handle_t server_ = nullptr;
    const WifiStatus *wifi_ = nullptr;
    ConnectivityService *connectivity_ = nullptr;
    DisplayService *display_ = nullptr;
    DeviceSettings *settings_ = nullptr;
    ConfigStore *config_store_ = nullptr;
    bool reboot_required_ = false;
    bool running_ = false;
    volatile bool activity_ = false;
};

}  // namespace bisc8
