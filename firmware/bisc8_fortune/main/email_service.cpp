#include "email_service.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_partition.h>

#include "app_config.h"
#include "debug_serial.h"
#include "voice_oracle_service.h"

namespace bisc8 {

namespace {
constexpr const char *kSpoolPartition = "spool";  // question WAV lives at offset 0
constexpr const char *kEmailBoundary = "----bisc8emailMUL7boundaryZ9";
constexpr uint32_t kEmailTimeoutMs = 30000;

std::string Str(const char *s) { return s == nullptr ? std::string() : std::string(s); }

void AddField(std::string &out, const std::string &boundary, const char *name, const std::string &value) {
    out += "--" + boundary + "\r\n";
    out += "Content-Disposition: form-data; name=\"" + std::string(name) + "\"\r\n\r\n";
    out += value + "\r\n";
}
}  // namespace

esp_err_t EmailService::SendOracleEmail(const EmailSettings &settings, const OracleResponse &response, const char *audio_path) {
    if (!settings.enabled || settings.recipient.empty() || settings.relay_url.empty()) {
        DebugSerial::LogAlways("[EMAIL]", "relay skipped; settings incomplete");
        return ESP_ERR_INVALID_STATE;
    }

    // Read the question WAV from the spool (offset 0) and trust its header for the
    // exact recorded length, so we attach precisely the recorded bytes.
    const bool want_attach = (audio_path != nullptr && audio_path[0] != '\0');
    const esp_partition_t *spool =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);
    uint32_t wav_total = 0;
    if (want_attach && spool != nullptr) {
        uint8_t header[44] = {};
        if (esp_partition_read(spool, 0, header, sizeof(header)) == ESP_OK &&
            memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVE", 4) == 0) {
            const uint32_t data_bytes = static_cast<uint32_t>(header[40]) | (static_cast<uint32_t>(header[41]) << 8) |
                                        (static_cast<uint32_t>(header[42]) << 16) | (static_cast<uint32_t>(header[43]) << 24);
            const uint32_t total = 44 + data_bytes;
            if (data_bytes > 0 && total <= static_cast<uint32_t>(spool->size)) {
                wav_total = total;
            }
        }
    }
    const bool attach = (wav_total > 0);

    const std::string boundary = kEmailBoundary;
    std::string preamble;
    AddField(preamble, boundary, "token", settings.relay_token);
    AddField(preamble, boundary, "to", settings.recipient);
    AddField(preamble, boundary, "transcript", Str(response.transcript));
    AddField(preamble, boundary, "answer", Str(response.oracle_answer_full));
    AddField(preamble, boundary, "lang", Str(response.detected_language));

    std::string file_header;
    if (attach) {
        file_header += "--" + boundary + "\r\n";
        file_header += "Content-Disposition: form-data; name=\"audio\"; filename=\"question.wav\"\r\n";
        file_header += "Content-Type: audio/wav\r\n\r\n";
    }
    const std::string epilogue = std::string(attach ? "\r\n" : "") + "--" + boundary + "--\r\n";
    const int content_length =
        static_cast<int>(preamble.size() + file_header.size() + (attach ? wav_total : 0) + epilogue.size());

    esp_http_client_config_t cfg = {};
    cfg.url = settings.relay_url.c_str();
    cfg.method = HTTP_METHOD_POST;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = kEmailTimeoutMs;
    cfg.buffer_size = 2048;
    cfg.buffer_size_tx = 2048;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        return ESP_FAIL;
    }
    const std::string content_type = "multipart/form-data; boundary=" + boundary;
    esp_http_client_set_header(client, "Content-Type", content_type.c_str());
    if (!settings.relay_token.empty()) {  // some hosts strip Authorization; the token is also a form field
        const std::string auth = "Bearer " + settings.relay_token;
        esp_http_client_set_header(client, "Authorization", auth.c_str());
    }

    auto write_all = [&](const char *data, size_t len) -> bool {
        size_t done = 0;
        while (done < len) {
            const int w = esp_http_client_write(client, data + done, len - done);
            if (w <= 0) {
                return false;
            }
            done += static_cast<size_t>(w);
        }
        return true;
    };

    esp_err_t err = esp_http_client_open(client, content_length);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        DebugSerial::LogAlways("[EMAIL]", "relay open failed: %s", esp_err_to_name(err));
        return err;
    }

    bool ok = write_all(preamble.data(), preamble.size());
    if (ok && attach) {
        ok = write_all(file_header.data(), file_header.size());
        uint8_t chunk[2048];
        uint32_t off = 0;
        uint32_t remaining = wav_total;
        while (ok && remaining > 0) {
            const uint32_t n = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
            if (esp_partition_read(spool, off, chunk, n) != ESP_OK) {
                ok = false;
                break;
            }
            ok = write_all(reinterpret_cast<const char *>(chunk), n);
            off += n;
            remaining -= n;
        }
    }
    if (ok) {
        ok = write_all(epilogue.data(), epilogue.size());
    }

    int status = 0;
    if (ok) {
        esp_http_client_fetch_headers(client);
        status = esp_http_client_get_status_code(client);
        char rbuf[256];
        while (esp_http_client_read(client, rbuf, sizeof(rbuf)) > 0) {
            // drain the (small JSON) response
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    DebugSerial::LogAlways("[EMAIL]", "relay POST status=%d wav=%uB attach=%d", status,
                           static_cast<unsigned>(wav_total), attach ? 1 : 0);
    if (!ok) {
        return ESP_FAIL;
    }
    return (status >= 200 && status < 300) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

}  // namespace bisc8
