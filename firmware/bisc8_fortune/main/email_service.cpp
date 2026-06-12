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
constexpr uint32_t kEmailTimeoutMs = 30000;
constexpr int kEmailMaxAttempts = 3;
constexpr uint32_t kEmailRetryDelayMs = 750;
// Email review copies: 8 kHz mono PCM16, downsampled with an averaging (box)
// low-pass. The earlier 4 kHz every-Nth decimation had no anti-alias filter,
// so 2-8 kHz speech energy folded back into the passband and the attachments
// sounded garbled on top of muffled. 8 kHz averaged is telephone-grade speech
// and the multipart body stays shared-host friendly: question <=8 s (128 KB)
// + answer <=12 s (192 KB), still ~3x below the old full-WAV payloads that
// made the relay upload flaky.
constexpr uint32_t kEmailAudioTargetRateHz = 8000;
constexpr uint32_t kEmailMaxQuestionPcmBytes = kEmailAudioTargetRateHz * 2 * 8;
constexpr uint32_t kEmailMaxAnswerPcmBytes = kEmailAudioTargetRateHz * 2 * 12;

enum class EmailPayloadMode { kCompact8k, kQuestionOnly8k, kTextOnly };

std::string Str(const char *s) { return s == nullptr ? std::string() : std::string(s); }

void AddField(std::string &out, const std::string &boundary, const char *name, const std::string &value) {
    out += "--" + boundary + "\r\n";
    out += "Content-Disposition: form-data; name=\"" + std::string(name) + "\"\r\n\r\n";
    out += value + "\r\n";
}

uint32_t ReadLe32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t ReadLe16(const uint8_t *p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

void PutLe16(uint8_t *p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xff);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xff);
}

void PutLe32(uint8_t *p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xff);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xff);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xff);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xff);
}

void BuildPcm16WavHeader(uint8_t *header, uint32_t sample_rate, uint32_t pcm_bytes) {
    memset(header, 0, 44);
    memcpy(header, "RIFF", 4);
    PutLe32(header + 4, 36 + pcm_bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    PutLe32(header + 16, 16);
    PutLe16(header + 20, 1);
    PutLe16(header + 22, 1);
    PutLe32(header + 24, sample_rate);
    PutLe32(header + 28, sample_rate * 2);
    PutLe16(header + 32, 2);
    PutLe16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    PutLe32(header + 40, pcm_bytes);
}

uint32_t DownsampleRatio(uint32_t source_rate, uint32_t target_rate) {
    if (source_rate == 0 || target_rate == 0) {
        return 1;
    }
    const uint32_t ratio = source_rate / target_rate;
    return ratio < 1 ? 1 : ratio;
}

uint32_t DownsampledPcmBytes(uint32_t source_pcm_bytes, uint32_t ratio, uint32_t max_pcm_bytes) {
    if (source_pcm_bytes < 2 || ratio == 0 || max_pcm_bytes < 2) {
        return 0;
    }
    const uint32_t source_samples = source_pcm_bytes / 2;
    const uint32_t out_samples = (source_samples + ratio - 1) / ratio;  // trailing partial group still emits one sample
    uint32_t bytes = out_samples * 2;
    const uint32_t cap = max_pcm_bytes & ~1U;
    if (bytes > cap) {
        bytes = cap;
    }
    return bytes;
}

const char *EmailPayloadModeName(EmailPayloadMode mode) {
    switch (mode) {
        case EmailPayloadMode::kCompact8k:
            return "compact8k";
        case EmailPayloadMode::kQuestionOnly8k:
            return "question8k";
        case EmailPayloadMode::kTextOnly:
        default:
            return "text";
    }
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

    // Question recording (spool offset 0): trust its header for the exact length.
    const bool want_question = (audio_path != nullptr && audio_path[0] != '\0');
    uint32_t wav_total = 0;
    uint32_t question_payload_off = 44;
    uint32_t question_pcm_bytes = 0;
    uint32_t question_sample_rate = 16000;
    if (want_question && spool != nullptr) {
        uint8_t header[44] = {};
        if (esp_partition_read(spool, 0, header, sizeof(header)) == ESP_OK &&
            memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVE", 4) == 0) {
            const uint32_t data_bytes = ReadLe32(header + 40);
            const uint16_t channels = ReadLe16(header + 22);
            const uint16_t bits = ReadLe16(header + 34);
            const uint32_t total = 44 + data_bytes;
            if (data_bytes > 0 && total <= static_cast<uint32_t>(spool->size) &&
                channels == 1 && bits == 16) {
                wav_total = total;
                question_pcm_bytes = data_bytes & ~1U;
                question_sample_rate = ReadLe32(header + 24);
            }
        }
    }
    const bool attach_question = (wav_total > 0);
    const uint32_t question_ratio = DownsampleRatio(question_sample_rate, kEmailAudioTargetRateHz);
    // The header must carry the TRUE output rate (source/ratio), not the nominal
    // target: a 22.05 kHz source with ratio 2 yields 11.025 kHz, and a wrong
    // rate plays at the wrong pitch.
    const uint32_t compact_question_rate = question_sample_rate / question_ratio;
    const uint32_t compact_question_pcm_bytes =
        attach_question ? DownsampledPcmBytes(question_pcm_bytes, question_ratio, kEmailMaxQuestionPcmBytes)
                        : 0;
    const uint32_t compact_question_total =
        compact_question_pcm_bytes > 0 ? 44 + compact_question_pcm_bytes : 0;
    uint8_t compact_question_hdr[44] = {};
    if (compact_question_total > 0) {
        BuildPcm16WavHeader(compact_question_hdr, compact_question_rate, compact_question_pcm_bytes);
    }

    // Answer audio (TTS) from the reserved answer region. Keep the full WAV for
    // local playback, but derive a small review copy for the email attachment.
    uint32_t ans_payload_off = 0;  // where audio data begins inside the answer WAV
    uint32_t ans_total = 0;        // whole answer WAV size relayed
    uint32_t ans_sample_rate = 24000;
    if (answer_audio_bytes >= 44 && spool != nullptr &&
        static_cast<uint64_t>(kVoiceAnswerSpoolOffset) + answer_audio_bytes <= spool->size) {
        uint8_t probe[128] = {};
        if (esp_partition_read(spool, kVoiceAnswerSpoolOffset, probe, sizeof(probe)) == ESP_OK &&
            memcmp(probe, "RIFF", 4) == 0 && memcmp(probe + 8, "WAVE", 4) == 0) {
            size_t p = 12;
            uint16_t ans_channels = 1;
            uint16_t ans_bits = 16;
            while (p + 8 <= sizeof(probe)) {
                const uint32_t csize = ReadLe32(probe + p + 4);
                if (memcmp(probe + p, "fmt ", 4) == 0 && csize >= 16 && p + 24 <= sizeof(probe)) {
                    ans_channels = ReadLe16(probe + p + 8 + 2);
                    ans_sample_rate = ReadLe32(probe + p + 8 + 4);
                    ans_bits = ReadLe16(probe + p + 8 + 14);
                } else if (memcmp(probe + p, "data", 4) == 0) {
                    ans_payload_off = static_cast<uint32_t>(p + 8);
                    break;
                }
                p += 8 + csize + (csize & 1);
            }
            if (ans_payload_off >= 44 && ans_payload_off <= sizeof(probe) &&
                answer_audio_bytes > ans_payload_off && ans_channels == 1 && ans_bits == 16) {
                ans_total = answer_audio_bytes;
            }
        }
    }
    const bool attach_answer = (ans_total > 0);
    const uint32_t ans_pcm_bytes = attach_answer ? ((ans_total - ans_payload_off) & ~1U) : 0;
    const uint32_t answer_ratio = DownsampleRatio(ans_sample_rate, kEmailAudioTargetRateHz);
    const uint32_t compact_answer_rate = ans_sample_rate / answer_ratio;
    const uint32_t compact_answer_pcm_bytes =
        attach_answer ? DownsampledPcmBytes(ans_pcm_bytes, answer_ratio, kEmailMaxAnswerPcmBytes)
                      : 0;
    const uint32_t compact_answer_total = compact_answer_pcm_bytes > 0 ? 44 + compact_answer_pcm_bytes : 0;
    uint8_t compact_ans_hdr[44] = {};
    if (compact_answer_total > 0) {
        BuildPcm16WavHeader(compact_ans_hdr, compact_answer_rate, compact_answer_pcm_bytes);
    }

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
    const std::string fh_q = compact_question_total > 0 ? file_part_header("audio", "question.wav") : std::string();
    // "answer_audio", not "answer": "answer" is already the answer-TEXT form field.
    const std::string fh_a = compact_answer_total > 0 ? file_part_header("answer_audio", "answer.wav") : std::string();
    const std::string closing = "--" + boundary + "--\r\n";
    const std::string content_type = "multipart/form-data; boundary=" + boundary;
    const std::string auth = "Bearer " + settings.relay_token;
    auto send_payload = [&](EmailPayloadMode payload_mode, int max_attempts) -> esp_err_t {
        const char *mode = EmailPayloadModeName(payload_mode);
        const bool include_question =
            (payload_mode != EmailPayloadMode::kTextOnly && compact_question_total > 0);
        const bool include_answer =
            (payload_mode == EmailPayloadMode::kCompact8k && compact_answer_total > 0);
        const uint32_t question_part_bytes = include_question ? compact_question_total : 0;
        const uint32_t answer_part_bytes = include_answer ? compact_answer_total : 0;
        const int content_length = static_cast<int>(
            preamble.size() +
            (question_part_bytes > 0 ? fh_q.size() + question_part_bytes + 2 : 0) +
            (answer_part_bytes > 0 ? fh_a.size() + answer_part_bytes + 2 : 0) +
            closing.size());

        DebugSerial::LogAlways("[EMAIL]", "relay payload mode=%s length=%d question=%uB answer=%uB", mode,
                               content_length, static_cast<unsigned>(question_part_bytes),
                               static_cast<unsigned>(answer_part_bytes));

        esp_err_t last_err = ESP_FAIL;
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
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
                    DebugSerial::LogAlways("[EMAIL]", "relay open failed: %s mode=%s attempt=%d",
                                           esp_err_to_name(err), mode, attempt);
                    last_err = err;
                } else {
                    // Stream box-filtered mono PCM into a small WAV attachment. Each group of
                    // `ratio` input samples is AVERAGED into one output sample (cheap low-pass),
                    // not just picked: plain every-Nth decimation aliases 2-8 kHz speech energy
                    // back into the passband and sounds garbled. The full-resolution files stay
                    // in flash for STT/playback; email only needs a reviewable copy.
                    uint8_t chunk[2048];
                    auto stream_downsampled_pcm = [&](uint32_t base, uint32_t len, uint32_t ratio,
                                                      uint32_t expected_bytes) -> bool {
                        if (ratio < 1) {
                            ratio = 1;
                        }
                        uint32_t off = base;
                        uint32_t remaining = len & ~1U;
                        uint32_t total_written = 0;
                        int32_t acc = 0;          // running sum of the current group, carried across chunks
                        uint32_t acc_count = 0;   // input samples accumulated in the group so far
                        while (remaining > 0 && total_written < expected_bytes) {
                            uint32_t n = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
                            n &= ~1U;
                            if (n == 0) {
                                break;
                            }
                            if (esp_partition_read(spool, off, chunk, n) != ESP_OK) {
                                return false;
                            }
                            // In-place compaction: the write cursor can never pass the read
                            // cursor (one output sample per `ratio` inputs, emitted after the
                            // group's last input byte pair was already consumed).
                            uint32_t out_len = 0;
                            for (uint32_t i = 0; i + 1 < n; i += 2) {
                                acc += static_cast<int16_t>(chunk[i] | (chunk[i + 1] << 8));
                                if (++acc_count < ratio) {
                                    continue;
                                }
                                const int16_t avg = static_cast<int16_t>(acc / static_cast<int32_t>(ratio));
                                acc = 0;
                                acc_count = 0;
                                if (total_written + out_len + 2 > expected_bytes) {
                                    break;
                                }
                                chunk[out_len++] = static_cast<uint8_t>(avg & 0xff);
                                chunk[out_len++] = static_cast<uint8_t>((avg >> 8) & 0xff);
                            }
                            if (out_len > 0) {
                                if (!write_all(reinterpret_cast<const char *>(chunk), out_len)) {
                                    return false;
                                }
                                total_written += out_len;
                            }
                            off += n;
                            remaining -= n;
                        }
                        // The byte budget counts a trailing partial group as one sample
                        // (ceil division in DownsampledPcmBytes); flush it.
                        if (total_written < expected_bytes && acc_count > 0) {
                            const int16_t avg = static_cast<int16_t>(acc / static_cast<int32_t>(acc_count));
                            uint8_t tail[2] = {static_cast<uint8_t>(avg & 0xff),
                                               static_cast<uint8_t>((avg >> 8) & 0xff)};
                            if (!write_all(reinterpret_cast<const char *>(tail), sizeof(tail))) {
                                return false;
                            }
                            total_written += sizeof(tail);
                        }
                        return total_written == expected_bytes;
                    };
                    auto stream_compact_question = [&]() -> bool {
                        return stream_downsampled_pcm(question_payload_off, question_pcm_bytes, question_ratio,
                                                      compact_question_pcm_bytes);
                    };
                    auto stream_compact_answer = [&]() -> bool {
                        return stream_downsampled_pcm(kVoiceAnswerSpoolOffset + ans_payload_off, ans_pcm_bytes,
                                                      answer_ratio, compact_answer_pcm_bytes);
                    };

                    bool ok = write_all(preamble.data(), preamble.size());
                    if (ok && include_question) {
                        ok = write_all(fh_q.data(), fh_q.size()) &&
                             write_all(reinterpret_cast<const char *>(compact_question_hdr),
                                       sizeof(compact_question_hdr)) &&
                             stream_compact_question() &&
                             write_all("\r\n", 2);
                    }
                    if (ok && include_answer) {
                        ok = write_all(fh_a.data(), fh_a.size()) &&
                             write_all(reinterpret_cast<const char *>(compact_ans_hdr), sizeof(compact_ans_hdr)) &&
                             stream_compact_answer() &&
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
                            DebugSerial::LogAlways("[EMAIL]", "relay headers failed: %d mode=%s", header_len,
                                                   mode);
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
                                DebugSerial::LogAlways("[EMAIL]", "relay read failed: %d mode=%s", r, mode);
                            }
                        }
                    }
                    esp_http_client_close(client);
                    esp_http_client_cleanup(client);

                    DebugSerial::LogAlways(
                        "[EMAIL]", "relay POST status=%d question=%uB answer=%uB mode=%s attempt=%d", status,
                        static_cast<unsigned>(question_part_bytes),
                        static_cast<unsigned>(answer_part_bytes), mode, attempt);
                    if (!ok) {
                        last_err = (err != ESP_OK) ? err : ESP_FAIL;
                    } else if (status >= 200 && status < 300) {
                        return ESP_OK;
                    } else {
                        last_err = ESP_ERR_INVALID_RESPONSE;
                    }
                }
            }
            if (attempt < max_attempts) {
                DebugSerial::LogAlways("[EMAIL]", "relay retry attempt=%d mode=%s err=%s", attempt + 1, mode,
                                       esp_err_to_name(last_err));
                vTaskDelay(pdMS_TO_TICKS(kEmailRetryDelayMs));
            }
        }
        return last_err;
    };

    if (compact_question_total > 0 || compact_answer_total > 0) {
        const esp_err_t compact_err = send_payload(EmailPayloadMode::kCompact8k, kEmailMaxAttempts);
        if (compact_err == ESP_OK) {
            return ESP_OK;
        }
        DebugSerial::LogAlways("[EMAIL]", "compact answer payload failed; retrying without answer audio err=%s",
                               esp_err_to_name(compact_err));
    }
    if (compact_question_total > 0) {
        const esp_err_t question_err = send_payload(EmailPayloadMode::kQuestionOnly8k, kEmailMaxAttempts);
        if (question_err == ESP_OK) {
            return ESP_OK;
        }
        DebugSerial::LogAlways("[EMAIL]", "question audio payload failed; retrying text-only err=%s",
                               esp_err_to_name(question_err));
    }
    return send_payload(EmailPayloadMode::kTextOnly, kEmailMaxAttempts);
}

}  // namespace bisc8
