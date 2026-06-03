#include "web_portal.h"

#include <cstdio>
#include <string>

#include "app_config.h"
#include "connectivity_service.h"
#include "debug_serial.h"

namespace bisc8 {

namespace {

const char *const kPortalRoutes[] = {
    "/",
    "/api/status",
    "/api/wifi/scan",
    "/api/wifi/credentials",
    "/api/settings",
    "/api/openai",
    "/api/smtp",
    "/api/reset",
};

const char *const kCaptiveProbePaths[] = {
    "/generate_204",
    "/hotspot-detect.html",
    "/ncsi.txt",
    "/connecttest.txt",
};

const char *const kIndexHtml =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>Bisc8 Setup</title></head><body>"
    "<main><h1>Bisc8 Setup</h1>"
    "<p>Join <strong>Bisc8-XXXX</strong> and open <strong>http://192.168.4.1</strong>.</p>"
    "<p class=\"warning\">OpenAI, Wi-Fi, and SMTP secrets are stored on this device. "
    "Enable flash encryption before production use.</p>"
    "<section id=\"wifi\"></section><section id=\"openai\"></section><section id=\"smtp\"></section>"
    "</main></body></html>";

WebPortal *PortalFromRequest(httpd_req_t *req) {
    return static_cast<WebPortal *>(req->user_ctx);
}

std::string JsonString(const std::string &value) {
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"' || ch == '\\') {
            out.push_back('\\');
        }
        if (ch == '\n') {
            out += "\\n";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('"');
    return out;
}

}  // namespace

const char *const *PortalRoutes(size_t *count) {
    if (count != nullptr) {
        *count = sizeof(kPortalRoutes) / sizeof(kPortalRoutes[0]);
    }
    return kPortalRoutes;
}

const char *const *CaptivePortalProbePaths(size_t *count) {
    if (count != nullptr) {
        *count = sizeof(kCaptiveProbePaths) / sizeof(kCaptiveProbePaths[0]);
    }
    return kCaptiveProbePaths;
}

const char *PortalIndexHtml() {
    return kIndexHtml;
}

void WebPortal::BindStatus(const WifiStatus *wifi, const DeviceSettings *settings) {
    wifi_ = wifi;
    settings_ = settings;
}

esp_err_t WebPortal::RegisterGet(const char *uri, HttpHandler handler) {
    httpd_uri_t route = {};
    route.uri = uri;
    route.method = HTTP_GET;
    route.handler = handler;
    route.user_ctx = this;
    return httpd_register_uri_handler(server_, &route);
}

esp_err_t WebPortal::HandleIndex(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebPortal::HandleStatus(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandlePlaceholderApi(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleCaptiveRedirect(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, nullptr, 0);
}

esp_err_t WebPortal::SendStatusJson(httpd_req_t *req) const {
    std::string setup_ssid;
    std::string setup_url = "http://192.168.4.1";
    std::string connected_ssid;
    std::string ssid_attempt;
    bool online = false;
    bool setup_active = running_;

    if (wifi_ != nullptr) {
        setup_ssid = wifi_->setup_ssid;
        setup_url = wifi_->setup_url.empty() ? setup_url : wifi_->setup_url;
        connected_ssid = wifi_->connected_ssid;
        ssid_attempt = wifi_->ssid_attempt;
        online = wifi_->online;
        setup_active = wifi_->setup_active || running_;
    }

    std::string language = "en";
    std::string openai_key;
    std::string smtp_host;
    std::string smtp_recipient;
    bool smtp_enabled = false;
    size_t wifi_count = 0;
    if (settings_ != nullptr) {
        language = settings_->language;
        openai_key = MaskSecret(settings_->openai.api_key);
        smtp_host = settings_->smtp.host;
        smtp_recipient = MaskSecret(settings_->smtp.recipient);
        smtp_enabled = settings_->smtp.enabled;
        wifi_count = settings_->wifi_count;
    }

    std::string json = "{";
    json += "\"online\":" + std::string(online ? "true" : "false");
    json += ",\"setup_active\":" + std::string(setup_active ? "true" : "false");
    json += ",\"setup_ssid\":" + JsonString(setup_ssid);
    json += ",\"setup_url\":" + JsonString(setup_url);
    json += ",\"connected_ssid\":" + JsonString(connected_ssid);
    json += ",\"ssid_attempt\":" + JsonString(ssid_attempt);
    json += ",\"language\":" + JsonString(language);
    json += ",\"wifi_count\":" + std::to_string(wifi_count);
    json += ",\"openai_key\":" + JsonString(openai_key);
    json += ",\"smtp_enabled\":" + std::string(smtp_enabled ? "true" : "false");
    json += ",\"smtp_host\":" + JsonString(smtp_host);
    json += ",\"smtp_recipient\":" + JsonString(smtp_recipient);
    json += ",\"warning\":\"secrets are stored on this device\"";
    json += "}";

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.size());
}

esp_err_t WebPortal::Start() {
    if (running_) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&server_, &config);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WEB]", "setup portal failed to start: %s", esp_err_to_name(err));
        return err;
    }

    err = RegisterGet("/", &WebPortal::HandleIndex);
    if (err == ESP_OK) {
        err = RegisterGet("/api/status", &WebPortal::HandleStatus);
    }
    for (size_t i = 2; err == ESP_OK && i < sizeof(kPortalRoutes) / sizeof(kPortalRoutes[0]); ++i) {
        err = RegisterGet(kPortalRoutes[i], &WebPortal::HandlePlaceholderApi);
    }
    for (const char *probe : kCaptiveProbePaths) {
        if (err != ESP_OK) {
            break;
        }
        err = RegisterGet(probe, &WebPortal::HandleCaptiveRedirect);
    }

    if (err != ESP_OK) {
        Stop();
        return err;
    }

    DebugSerial::LogAlways("[WEB]", "setup portal ready at http://192.168.4.1 with masked secrets: %s",
                           MaskSecret("sk-example").c_str());
    running_ = true;
    return ESP_OK;
}

void WebPortal::Stop() {
    if (server_ != nullptr) {
        httpd_stop(server_);
        server_ = nullptr;
    }
    if (running_) {
        DebugSerial::LogAlways("[WEB]", "setup portal stopped");
    }
    running_ = false;
}

bool WebPortal::Running() const {
    return running_;
}

}  // namespace bisc8
