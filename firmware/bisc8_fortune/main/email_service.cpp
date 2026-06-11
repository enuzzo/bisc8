#include "email_service.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_config.h"
#include "debug_serial.h"
#include "voice_oracle_service.h"

namespace bisc8 {

namespace {
constexpr const char *kSpoolPartition = "spool";  // question WAV lives at offset 0
constexpr const char *kEmailBoundary = "----bisc8emailMUL7boundaryZ9";
constexpr uint32_t kEmailTimeoutMs = 60000;
constexpr int kEmailMaxAttempts = 3;
constexpr uint32_t kEmailRetryDelayMs = 750;

std::string Str(const char *s) { return s == nullptr ? std::string() : std::string(s); }

void AddField(std::string &out, const std::string &boundary, const char *name, const std::string &value) {
    out += "--" + boundary + "\r\n";
    out += "Content-Disposition: form-data; name=\"" + std::string(name) + "\"\r\n\r\n";
    out += value + "\r\n";
}
}  // namespace

esp_err_t EmailService::SendOracleEmail(const EmailSettings &settings, const OracleResponse &response,
                                        const char *audio_path, uint32_t answer_audio_bytes) {
    if (!settings.enabled || settings.recipient.empty() || settings.relay_url.empty()) {
        DebugSerial::LogAlways("[EMAIL]", "relay skipped; settings incomplete");
        return ESP_ERR_INVALID_STATE;
    }

    const esp_partition_t *spool =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);

    auto le32 = [](const uint8_t *p) {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    };
    auto put_le32 = [](uint8_t *p, uint32_t v) {
        p[0] = static_cast<uint8_t>(v & 0xff);
        p[1] = static_cast<uint8_t>((v >> 8) & 0xff);
        p[2] = static_cast<uint8_t>((v >> 16) & 0xff);
        p[3] = static_cast<uint8_t>((v >> 24) & 0xff);
    };

    // Question recording (spool offset 0): trust its header for the exact length.
    const bool want_question = (audio_path != nullptr && audio_path[0] != '\0');
    uint32_t wav_total = 0;
    if (want_question && spool != nullptr) {
        uint8_t header[44] = {};
        if (esp_partition_read(spool, 0, header, sizeof(header)) == ESP_OK &&
            memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVE", 4) == 0) {
            const uint32_t data_bytes = le32(header + 40);
            const uint32_t total = 44 + data_bytes;
            if (data_bytes > 0 && total <= static_cast<uint32_t>(spool->size)) {
                wav_total = total;
            }
        }
    }
    const bool attach_question = (wav_total > 0);

    // Answer audio (TTS) from the reserved answer region. The oracle may build
    // this WAV around raw PCM, and older builds accepted streamed WAVs with an
    // unknown data size. Always send a patched copy with the true byte count.
    uint8_t ans_hdr[128] = {};
    uint32_t ans_hdr_len = 0;      // patched header bytes sent before the payload
    uint32_t ans_payload_off = 0;  // where audio data begins inside the answer WAV
    uint32_t ans_total = 0;        // whole answer WAV size relayed
    if (answer_audio_bytes >= 44 && spool != nullptr &&
        static_cast<uint64_t>(kVoiceAnswerSpoolOffset) + answer_audio_bytes <= spool->size) {
        uint8_t probe[128] = {};
        if (esp_partition_read(spool, kVoiceAnswerSpoolOffset, probe, sizeof(probe)) == ESP_OK &&
            memcmp(probe, "RIFF", 4) == 0 && memcmp(probe + 8, "WAVE", 4) == 0) {
            size_t p = 12;
            uint32_t data_size_field_pos = 0;
            while (p + 8 <= sizeof(probe)) {
                const uint32_t csize = le32(probe + p + 4);
                if (memcmp(probe + p, "data", 4) == 0) {
                    data_size_field_pos = static_cast<uint32_t>(p + 4);
                    ans_payload_off = static_cast<uint32_t>(p + 8);
                    break;
                }
                p += 8 + csize + (csize & 1);
            }
            if (ans_payload_off >= 44 && ans_payload_off <= sizeof(probe) && answer_audio_bytes > ans_payload_off) {
                ans_total = answer_audio_bytes;
                ans_hdr_len = ans_payload_off;
                memcpy(ans_hdr, probe, ans_hdr_len);
                put_le32(ans_hdr + 4, ans_total - 8);                                  // RIFF chunk size
                put_le32(ans_hdr + data_size_field_pos, ans_total - ans_payload_off);  // data chunk size
            }
        }
    }
    const bool attach_answer = (ans_total > 0);

    const std::string boundary = kEmailBoundary;
    std::string preamble;
    AddField(preamble, boundary, "token", settings.relay_token);
    AddField(preamble, boundary, "to", settings.recipient);
    AddField(preamble, boundary, "transcript", Str(response.transcript));
    AddField(preamble, boundary, "answer", Str(response.oracle_answer_full));
    AddField(preamble, boundary, "lang", Str(response.detected_language));
    // Engines used per phase (rendered as a "made with" line in the email).
    AddField(preamble, boundary, "stt_model", Str(response.stt_model));
    AddField(preamble, boundary, "brain_model", Str(response.brain_model));
    AddField(preamble, boundary, "tts_model", Str(response.tts_model));
    AddField(preamble, boundary, "voice", Str(response.voice));

    // Each file part is self-terminating (body followed by CRLF), so question +
    // answer concatenate cleanly and a missing question still yields valid MIME.
    auto file_part_header = [&](const char *field, const char *filename) {
        std::string h = "--" + boundary + "\r\n";
        h += "Content-Disposition: form-data; name=\"" + std::string(field) + "\"; filename=\"" +
             std::string(filename) + "\"\r\n";
        h += "Content-Type: audio/wav\r\n\r\n";
        return h;
    };
    const std::string fh_q = attach_question ? file_part_header("audio", "question.wav") : std::string();
    // "answer_audio", not "answer": "answer" is already the answer-TEXT form field.
    const std::string fh_a = attach_answer ? file_part_header("answer_audio", "answer.wav") : std::string();
    const std::string closing = "--" + boundary + "--\r\n";
    const int content_length = static_cast<int>(
        preamble.size() +
        (attach_question ? fh_q.size() + wav_total + 2 : 0) +  // +2: CRLF terminating the part body
        (attach_answer ? fh_a.size() + ans_total + 2 : 0) +
        closing.size());

    const std::string content_type = "multipart/form-data; boundary=" + boundary;
    const std::string auth = "Bearer " + settings.relay_token;
    esp_err_t last_err = ESP_FAIL;
    for (int attempt = 1; attempt <= kEmailMaxAttempts; ++attempt) {
        esp_http_client_config_t cfg = {};
        cfg.url = settings.relay_url.c_str();
        cfg.method = HTTP_METHOD_POST;
        cfg.crt_bundle_attach = esp_crt_bundle_attach;
        cfg.timeout_ms = kEmailTimeoutMs;
        cfg.buffer_size = 2048;
        cfg.buffer_size_tx = 2048;
        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (client == nullptr) {
            last_err = ESP_ERR_NO_MEM;
        } else {
            esp_http_client_set_header(client, "Content-Type", content_type.c_str());
            if (!settings.relay_token.empty()) {  // some hosts strip Authorization; the token is also a form field
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
                DebugSerial::LogAlways("[EMAIL]", "relay open failed: %s attempt=%d", esp_err_to_name(err),
                                       attempt);
                last_err = err;
            } else {
                // Stream a [base, base+len) spool region to the client in flash-friendly chunks.
                uint8_t chunk[2048];
                auto stream_region = [&](uint32_t base, uint32_t len) -> bool {
                    uint32_t off = base;
                    uint32_t remaining = len;
                    while (remaining > 0) {
                        const uint32_t n = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
                        if (esp_partition_read(spool, off, chunk, n) != ESP_OK) {
                            return false;
                        }
                        if (!write_all(reinterpret_cast<const char *>(chunk), n)) {
                            return false;
                        }
                        off += n;
                        remaining -= n;
                    }
                    return true;
                };

                bool ok = write_all(preamble.data(), preamble.size());
                if (ok && attach_question) {
                    ok = write_all(fh_q.data(), fh_q.size()) && stream_region(0, wav_total) &&
                         write_all("\r\n", 2);
                }
                if (ok && attach_answer) {
                    // Patched header first, then the audio payload that follows it in flash.
                    ok = write_all(fh_a.data(), fh_a.size()) &&
                         write_all(reinterpret_cast<const char *>(ans_hdr), ans_hdr_len) &&
                         stream_region(kVoiceAnswerSpoolOffset + ans_payload_off, ans_total - ans_payload_off) &&
                         write_all("\r\n", 2);
                }
                if (ok) {
                    ok = write_all(closing.data(), closing.size());
                }

                int status = 0;
                if (ok) {
                    const int header_len = esp_http_client_fetch_headers(client);
                    if (header_len < 0) {
                        ok = false;
                        err = ESP_FAIL;
                        DebugSerial::LogAlways("[EMAIL]", "relay headers failed: %d", header_len);
                    } else {
                        status = esp_http_client_get_status_code(client);
                        char rbuf[256];
                        int r = 0;
                        while ((r = esp_http_client_read(client, rbuf, sizeof(rbuf))) > 0) {
                            // drain the (small JSON) response
                        }
                        if (r < 0) {
                            ok = false;
                            err = ESP_FAIL;
                            DebugSerial::LogAlways("[EMAIL]", "relay read failed: %d", r);
                        }
                    }
                }
                esp_http_client_close(client);
                esp_http_client_cleanup(client);

                DebugSerial::LogAlways("[EMAIL]", "relay POST status=%d question=%uB answer=%uB attempt=%d",
                                       status, static_cast<unsigned>(attach_question ? wav_total : 0),
                                       static_cast<unsigned>(attach_answer ? ans_total : 0), attempt);
                if (!ok) {
                    last_err = (err != ESP_OK) ? err : ESP_FAIL;
                } else if (status >= 200 && status < 300) {
                    return ESP_OK;
                } else {
                    last_err = ESP_ERR_INVALID_RESPONSE;
                }
            }
        }
        if (attempt < kEmailMaxAttempts) {
            DebugSerial::LogAlways("[EMAIL]", "relay retry attempt=%d err=%s", attempt + 1,
                                   esp_err_to_name(last_err));
            vTaskDelay(pdMS_TO_TICKS(kEmailRetryDelayMs));
        }
    }
    return last_err;
}

}  // namespace bisc8
