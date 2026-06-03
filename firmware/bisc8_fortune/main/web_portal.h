#pragma once

#include <stddef.h>

#include <esp_err.h>
#include <esp_http_server.h>

namespace bisc8 {

struct DeviceSettings;
struct WifiStatus;

using HttpHandler = esp_err_t (*)(httpd_req_t *req);

const char *const *PortalRoutes(size_t *count);
const char *const *CaptivePortalProbePaths(size_t *count);
const char *PortalIndexHtml();

class WebPortal {
public:
    void BindStatus(const WifiStatus *wifi, const DeviceSettings *settings);
    esp_err_t Start();
    void Stop();
    bool Running() const;

private:
    static esp_err_t HandleIndex(httpd_req_t *req);
    static esp_err_t HandleStatus(httpd_req_t *req);
    static esp_err_t HandlePlaceholderApi(httpd_req_t *req);
    static esp_err_t HandleCaptiveRedirect(httpd_req_t *req);
    esp_err_t RegisterGet(const char *uri, HttpHandler handler);
    esp_err_t SendStatusJson(httpd_req_t *req) const;

    httpd_handle_t server_ = nullptr;
    const WifiStatus *wifi_ = nullptr;
    const DeviceSettings *settings_ = nullptr;
    bool running_ = false;
};

}  // namespace bisc8
