#include "voice_oracle_service.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>
#include <string>

#include <cJSON.h>
#include <esp_check.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_tls.h>
#include <esp_transport.h>
#include <esp_transport_ssl.h>
#include <esp_transport_ws.h>
#include <mbedtls/base64.h>

#include "app_config.h"
#include "debug_serial.h"

namespace bisc8 {

namespace {

constexpr const char *kChatUrl = "https://api.openai.com/v1/chat/completions";
constexpr const char *kTranscribeUrl = "https://api.openai.com/v1/audio/transcriptions";
constexpr const char *kSpeechUrl = "https://api.openai.com/v1/audio/speech";
constexpr const char *kRealtimeHost = "api.openai.com";
constexpr const char *kRealtimePathPrefix = "/v1/realtime?model=";
constexpr const char *kAnswerSpoolUri = "spool://answer.wav";
constexpr const char *kSpoolPartition = "spool";
constexpr const char *kMultipartBoundary = "----bisc8oracleMUL7boundaryZ9";
constexpr const char *kTranscriptionPrompt =
    "The speaker usually asks short oracle questions in Italian, English, or Spanish. "
    "Transcribe only clear foreground speech. If the audio is quiet, clipped, or mostly silence, "
    "do not invent subtitles, captions, music lyrics, or background language.";
constexpr uint32_t kHttpTimeoutMs = 25000;
constexpr uint32_t kRealtimeTimeoutMs = 45000;
constexpr uint32_t kRealtimeAudioRateHz = 24000;
constexpr size_t kRealtimeReadChunkBytes = 2048;
constexpr size_t kRealtimeMaxJsonEventBytes = 12288;
constexpr size_t kRealtimeBase64SliceChars = 1024;  // must stay divisible by 4

// The oracle's voice. Asks for the repo's exact response-contract field names so
// the JSON maps straight onto OracleResponse.
constexpr const char *kBrainSystemPrompt =
    "You are Bisc8, a pocket oracle: a tiny black-and-white biscuit that divines fates. "
    "Voice: lyrical and incantatory -- richly poetic, musical and mystical, a seer who has glimpsed many "
    "futures and speaks in veiled, luminous images, with the cadence of verse and vivid sensory detail. "
    "Lean into lyricism, mystery and a touch of ambiguity, like a true oracle -- but always grasp "
    "the heart of what they asked and let your answer truly speak to THEIR question: the meaning must still "
    "land, even when it arrives wrapped in metaphor. Evocative and a little enigmatic, yes; random or "
    "generic, never. Let the asker feel seen, then leave them something to ponder. "
    "Answer in the SAME language as the user's question.\n"
    "Given the user's transcript, produce a short oracle answer as COMPACT JSON ONLY (no markdown, no code fences), "
    "with exactly these fields:\n"
    "- detected_language: ISO-like code (it, en, es, ...)\n"
    "- oracle_answer_screen: AT MOST 55 characters, one short clear line for a tiny 200x200 screen, no newlines, "
    "Latin-1 characters only\n"
    "- oracle_answer_full: 1 to 3 short sentences, for voice\n"
    "- tts_text: the exact spoken script to read aloud (1 to 4 short lines)\n"
    "- voice_direction: a short English note on the emotional colour of THIS answer (e.g. tender, knowing, hopeful, consoling)\n"
    "Rules: same language as the user; always speak to their specific question, even through metaphor; never say you are an AI; "
    "do not give medical, legal, financial or safety-critical advice as certainty; poetic and evocative, never nonsense; "
    "concise. Avoid long dashes. For oracle_answer_screen, never use CJK, emoji, runes, or symbolic glyphs.";

bool SpeechModelSupportsInstructions(const std::string &model) {
    return !model.empty() && model.rfind("tts-1", 0) != 0;
}

std::string JsonStr(const cJSON *obj, const char *key);

bool IsRealtimeSpeechModel(const std::string &model) {
    return model.rfind("gpt-realtime", 0) == 0;
}

bool IsRealtimeVoice(const std::string &voice) {
    return voice == "alloy" || voice == "ash" || voice == "ballad" || voice == "coral" ||
           voice == "echo" || voice == "sage" || voice == "shimmer" || voice == "verse" ||
           voice == "marin" || voice == "cedar";
}

bool IsRequestSpeechVoice(const std::string &voice) {
    return voice == "alloy" || voice == "ash" || voice == "coral" || voice == "echo" ||
           voice == "fable" || voice == "nova" || voice == "onyx" || voice == "sage" ||
           voice == "shimmer";
}

const char *SpeechVoiceForModel(const OpenAiSettings &openai) {
    if (IsRealtimeSpeechModel(openai.speech_model)) {
        return IsRealtimeVoice(openai.voice) ? openai.voice.c_str() : "marin";
    }
    return IsRequestSpeechVoice(openai.voice) ? openai.voice.c_str() : "coral";
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

esp_err_t WritePcm16WavHeader(const esp_partition_t *spool, uint32_t offset, uint32_t pcm_bytes) {
    uint8_t header[44] = {};
    memcpy(header, "RIFF", 4);
    PutLe32(header + 4, 36 + pcm_bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    PutLe32(header + 16, 16);
    PutLe16(header + 20, 1);
    PutLe16(header + 22, 1);
    PutLe32(header + 24, kRealtimeAudioRateHz);
    PutLe32(header + 28, kRealtimeAudioRateHz * 2);
    PutLe16(header + 32, 2);
    PutLe16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    PutLe32(header + 40, pcm_bytes);
    return esp_partition_write(spool, offset, header, sizeof(header));
}

bool DecodeBase64Slice(const char *input, size_t input_len, uint8_t *decoded, size_t decoded_cap,
                       size_t *written) {
    if (input == nullptr || decoded == nullptr || written == nullptr) {
        return false;
    }
    *written = 0;
    return mbedtls_base64_decode(decoded, decoded_cap, written,
                                 reinterpret_cast<const unsigned char *>(input), input_len) == 0;
}

bool WsSendText(esp_transport_handle_t ws, const std::string &body) {
    std::string mutable_body = body;
    // esp_transport_ws_send_raw() expects the caller to include FIN in the
    // opcode. Without it the server waits for a continuation frame, then closes
    // with WebSocket protocol error 1002 when the next TEXT event arrives.
    const auto opcode = static_cast<ws_transport_opcodes_t>(WS_TRANSPORT_OPCODES_TEXT | WS_TRANSPORT_OPCODES_FIN);
    const int written = esp_transport_ws_send_raw(ws, opcode, mutable_body.data(),
                                                  static_cast<int>(mutable_body.size()), kHttpTimeoutMs);
    return written == static_cast<int>(mutable_body.size());
}

struct RealtimeAudioSink {
    const esp_partition_t *spool;
    uint32_t written;
    bool error;
};

bool AppendRealtimePcm(RealtimeAudioSink *sink, const void *pcm, size_t len) {
    if (sink == nullptr || sink->error || pcm == nullptr || len == 0) {
        return sink != nullptr && !sink->error;
    }
    if (sink->written + len > kVoiceAnswerSpoolMaxBytes - 44) {
        sink->error = true;
        return false;
    }
    const esp_err_t err =
        esp_partition_write(sink->spool, kVoiceAnswerSpoolOffset + 44 + sink->written, pcm, len);
    if (err != ESP_OK) {
        sink->error = true;
        return false;
    }
    sink->written += static_cast<uint32_t>(len);
    return true;
}

bool DecodeAndAppendRealtimeBase64(RealtimeAudioSink *sink, const char *delta, size_t delta_len) {
    if (sink == nullptr || delta == nullptr || delta_len == 0) {
        return false;
    }
    uint8_t pcm[768];
    size_t off = 0;
    while (off < delta_len) {
        size_t slice = std::min(kRealtimeBase64SliceChars, delta_len - off);
        if (slice < delta_len - off) {
            slice -= slice % 4;
        }
        if (slice == 0) {
            return false;
        }
        size_t written = 0;
        if (!DecodeBase64Slice(delta + off, slice, pcm, sizeof(pcm), &written)) {
            return false;
        }
        if (!AppendRealtimePcm(sink, pcm, written)) {
            return false;
        }
        off += slice;
    }
    return true;
}

struct RealtimeEventBuffer {
    char data[kRealtimeMaxJsonEventBytes + 1];
    size_t len;
    size_t expected_len;
};

struct RealtimeEvent {
    cJSON *json;
    const char *type;
    size_t type_len;
    const char *delta;
    size_t delta_len;
    bool raw_audio_delta;
    uint32_t streamed_audio_bytes;
};

void ResetRealtimeEventBuffer(RealtimeEventBuffer *pending) {
    if (pending == nullptr) {
        return;
    }
    pending->len = 0;
    pending->expected_len = 0;
    pending->data[0] = '\0';
}

void CleanupRealtimeEvent(RealtimeEventBuffer *pending, RealtimeEvent *event) {
    if (event != nullptr && event->json != nullptr) {
        cJSON_Delete(event->json);
        event->json = nullptr;
    }
    ResetRealtimeEventBuffer(pending);
}

bool JsonRawStringValue(const char *json, size_t len, const char *key, const char **value, size_t *value_len) {
    if (json == nullptr || key == nullptr || value == nullptr || value_len == nullptr) {
        return false;
    }
    const size_t key_len = strlen(key);
    for (size_t i = 0; i + key_len + 2 < len; ++i) {
        if (json[i] != '"' || memcmp(json + i + 1, key, key_len) != 0 || json[i + key_len + 1] != '"') {
            continue;
        }
        size_t p = i + key_len + 2;
        while (p < len && isspace(static_cast<unsigned char>(json[p]))) {
            ++p;
        }
        if (p >= len || json[p++] != ':') {
            continue;
        }
        while (p < len && isspace(static_cast<unsigned char>(json[p]))) {
            ++p;
        }
        if (p >= len || json[p++] != '"') {
            continue;
        }
        const char *start = json + p;
        bool escaped = false;
        while (p < len) {
            const char c = json[p];
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                *value = start;
                *value_len = static_cast<size_t>((json + p) - start);
                return true;
            }
            ++p;
        }
        return false;
    }
    return false;
}

bool RawEquals(const char *value, size_t value_len, const char *expected) {
    return value != nullptr && expected != nullptr && strlen(expected) == value_len &&
           memcmp(value, expected, value_len) == 0;
}

bool RealtimeEventIs(const RealtimeEvent &event, const char *type) {
    return RawEquals(event.type, event.type_len, type);
}

bool RealtimeEventTypeIsAudioDelta(const char *type, size_t type_len) {
    return RawEquals(type, type_len, "response.audio.delta") ||
           RawEquals(type, type_len, "response.output_audio.delta");
}

bool PendingRealtimeEventIsAudioDelta(const RealtimeEventBuffer *pending) {
    if (pending == nullptr || pending->len == 0) {
        return false;
    }
    const char *type = nullptr;
    size_t type_len = 0;
    return JsonRawStringValue(pending->data, pending->len, "type", &type, &type_len) &&
           RealtimeEventTypeIsAudioDelta(type, type_len);
}

struct RealtimeAudioDeltaReader {
    RealtimeAudioSink *sink;
    char b64[kRealtimeBase64SliceChars];
    size_t b64_len;
    size_t key_match;
    enum class State {
        SearchKey,
        AfterKey,
        BeforeValue,
        InValue,
        Done,
    } state;
    bool escaped;
    bool error;
};

bool FlushRealtimeAudioDeltaBase64(RealtimeAudioDeltaReader *reader) {
    if (reader == nullptr || reader->error || reader->b64_len == 0) {
        return reader != nullptr && !reader->error;
    }
    if (reader->b64_len % 4 != 0) {
        reader->error = true;
        return false;
    }
    uint8_t pcm[768];
    size_t written = 0;
    if (!DecodeBase64Slice(reader->b64, reader->b64_len, pcm, sizeof(pcm), &written)) {
        reader->error = true;
        return false;
    }
    reader->b64_len = 0;
    if (!AppendRealtimePcm(reader->sink, pcm, written)) {
        reader->error = true;
        return false;
    }
    return true;
}

bool PushRealtimeAudioDeltaChar(RealtimeAudioDeltaReader *reader, char c) {
    if (reader == nullptr || reader->error) {
        return false;
    }
    reader->b64[reader->b64_len++] = c;
    if (reader->b64_len == sizeof(reader->b64)) {
        return FlushRealtimeAudioDeltaBase64(reader);
    }
    return true;
}

bool ConsumeRealtimeAudioDeltaJson(RealtimeAudioDeltaReader *reader, const char *data, size_t len) {
    if (reader == nullptr || data == nullptr || reader->error) {
        return false;
    }
    constexpr const char *kDeltaKey = "\"delta\"";
    constexpr size_t kDeltaKeyLen = 7;
    for (size_t i = 0; i < len; ++i) {
        const char c = data[i];
        switch (reader->state) {
            case RealtimeAudioDeltaReader::State::SearchKey:
                if (c == kDeltaKey[reader->key_match]) {
                    ++reader->key_match;
                    if (reader->key_match == kDeltaKeyLen) {
                        reader->state = RealtimeAudioDeltaReader::State::AfterKey;
                        reader->key_match = 0;
                    }
                } else {
                    reader->key_match = (c == kDeltaKey[0]) ? 1 : 0;
                }
                break;
            case RealtimeAudioDeltaReader::State::AfterKey:
                if (isspace(static_cast<unsigned char>(c))) {
                    break;
                }
                if (c != ':') {
                    reader->error = true;
                    return false;
                }
                reader->state = RealtimeAudioDeltaReader::State::BeforeValue;
                break;
            case RealtimeAudioDeltaReader::State::BeforeValue:
                if (isspace(static_cast<unsigned char>(c))) {
                    break;
                }
                if (c != '"') {
                    reader->error = true;
                    return false;
                }
                reader->state = RealtimeAudioDeltaReader::State::InValue;
                reader->escaped = false;
                break;
            case RealtimeAudioDeltaReader::State::InValue:
                if (reader->escaped) {
                    reader->escaped = false;
                    if (c == 'u') {
                        reader->error = true;
                        return false;
                    }
                    const char decoded = (c == '/') ? '/' : c;
                    if (!PushRealtimeAudioDeltaChar(reader, decoded)) {
                        return false;
                    }
                } else if (c == '\\') {
                    reader->escaped = true;
                } else if (c == '"') {
                    if (!FlushRealtimeAudioDeltaBase64(reader)) {
                        return false;
                    }
                    reader->state = RealtimeAudioDeltaReader::State::Done;
                } else if (!PushRealtimeAudioDeltaChar(reader, c)) {
                    return false;
                }
                break;
            case RealtimeAudioDeltaReader::State::Done:
                break;
        }
    }
    return !reader->error;
}

bool RealtimeEventTypeNeedsFullJson(const char *type, size_t type_len) {
    return RawEquals(type, type_len, "error") || RawEquals(type, type_len, "response.done");
}

esp_err_t SkipOversizedRealtimeEvent(esp_transport_handle_t ws, RealtimeEventBuffer *pending, char *buf,
                                     size_t buf_size, size_t already_consumed, RealtimeEvent *event,
                                     bool *closed) {
    if (ws == nullptr || pending == nullptr || buf == nullptr || event == nullptr ||
        closed == nullptr || already_consumed > pending->expected_len) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *type = nullptr;
    size_t type_len = 0;
    if (!JsonRawStringValue(pending->data, pending->len, "type", &type, &type_len)) {
        DebugSerial::LogAlways("[ORACLE]", "realtime oversized event without early type pending=%u expected=%u",
                               static_cast<unsigned>(already_consumed),
                               static_cast<unsigned>(pending->expected_len));
        ResetRealtimeEventBuffer(pending);
        return ESP_ERR_INVALID_SIZE;
    }
    if (RealtimeEventTypeIsAudioDelta(type, type_len) || RealtimeEventTypeNeedsFullJson(type, type_len)) {
        DebugSerial::LogAlways("[ORACLE]", "realtime critical event too large type=%.*s pending=%u expected=%u",
                               static_cast<int>(type_len), type, static_cast<unsigned>(already_consumed),
                               static_cast<unsigned>(pending->expected_len));
        ResetRealtimeEventBuffer(pending);
        return ESP_ERR_INVALID_SIZE;
    }

    size_t consumed = already_consumed;
    int timeout_reads = 0;
    while (consumed < pending->expected_len) {
        const int r = esp_transport_read(ws, buf, static_cast<int>(buf_size), 5000);
        if (r == 0) {
            if (++timeout_reads < 3) {
                continue;
            }
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_TIMEOUT;
        }
        timeout_reads = 0;
        if (r < 0) {
            ResetRealtimeEventBuffer(pending);
            return ESP_FAIL;
        }
        const ws_transport_opcodes_t opcode = esp_transport_ws_get_read_opcode(ws);
        if (opcode == WS_TRANSPORT_OPCODES_CLOSE) {
            *closed = true;
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_INVALID_RESPONSE;
        }
        if (opcode != WS_TRANSPORT_OPCODES_TEXT && opcode != WS_TRANSPORT_OPCODES_CONT) {
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_TIMEOUT;
        }
        consumed += static_cast<size_t>(r);
        if (consumed > pending->expected_len) {
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_INVALID_SIZE;
        }
    }

    DebugSerial::LogAlways("[ORACLE]", "realtime skipped oversized event type=%.*s bytes=%u",
                           static_cast<int>(type_len), type, static_cast<unsigned>(pending->expected_len));
    *event = RealtimeEvent{};
    event->type = type;
    event->type_len = type_len;
    ResetRealtimeEventBuffer(pending);
    return ESP_OK;
}

esp_err_t ParseRealtimeBufferedEvent(RealtimeEventBuffer *pending, RealtimeEvent *event) {
    if (pending == nullptr || event == nullptr || pending->len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    pending->data[pending->len] = '\0';
    *event = RealtimeEvent{};

    const char *type = nullptr;
    size_t type_len = 0;
    if (!JsonRawStringValue(pending->data, pending->len, "type", &type, &type_len)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    event->type = type;
    event->type_len = type_len;

    if (RawEquals(type, type_len, "response.audio.delta") ||
        RawEquals(type, type_len, "response.output_audio.delta")) {
        const char *delta = nullptr;
        size_t delta_len = 0;
        if (!JsonRawStringValue(pending->data, pending->len, "delta", &delta, &delta_len)) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        event->delta = delta;
        event->delta_len = delta_len;
        event->raw_audio_delta = true;
        return ESP_OK;
    }

    const char *parse_end = nullptr;
    cJSON *parsed = cJSON_ParseWithLengthOpts(pending->data, pending->len, &parse_end, false);
    if (parsed == nullptr) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    event->json = parsed;
    return ESP_OK;
}

esp_err_t StreamRealtimeAudioDeltaEvent(esp_transport_handle_t ws, RealtimeEventBuffer *pending, char *buf,
                                        size_t buf_size, RealtimeEvent *event, bool *closed,
                                        RealtimeAudioSink *audio_sink) {
    if (ws == nullptr || pending == nullptr || buf == nullptr || event == nullptr ||
        closed == nullptr || audio_sink == nullptr || pending->len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const char *type = nullptr;
    size_t type_len = 0;
    if (!JsonRawStringValue(pending->data, pending->len, "type", &type, &type_len) ||
        !RealtimeEventTypeIsAudioDelta(type, type_len)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    const uint32_t before = audio_sink->written;
    RealtimeAudioDeltaReader reader = {
        audio_sink,
        {},
        0,
        0,
        RealtimeAudioDeltaReader::State::SearchKey,
        false,
        false,
    };
    if (!ConsumeRealtimeAudioDeltaJson(&reader, pending->data, pending->len)) {
        ResetRealtimeEventBuffer(pending);
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t consumed = pending->len;
    int timeout_reads = 0;
    while (consumed < pending->expected_len) {
        const int r = esp_transport_read(ws, buf, static_cast<int>(buf_size), 5000);
        if (r == 0) {
            if (++timeout_reads < 3) {
                continue;
            }
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_TIMEOUT;
        }
        timeout_reads = 0;
        if (r < 0) {
            ResetRealtimeEventBuffer(pending);
            return ESP_FAIL;
        }

        const ws_transport_opcodes_t opcode = esp_transport_ws_get_read_opcode(ws);
        if (opcode == WS_TRANSPORT_OPCODES_CLOSE) {
            *closed = true;
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_INVALID_RESPONSE;
        }
        if (opcode != WS_TRANSPORT_OPCODES_TEXT && opcode != WS_TRANSPORT_OPCODES_CONT) {
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_TIMEOUT;
        }

        const size_t chunk_len = static_cast<size_t>(r);
        if (consumed + chunk_len > pending->expected_len) {
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_INVALID_SIZE;
        }
        if (!ConsumeRealtimeAudioDeltaJson(&reader, buf, chunk_len)) {
            ResetRealtimeEventBuffer(pending);
            return ESP_ERR_INVALID_RESPONSE;
        }
        consumed += chunk_len;
    }

    if (reader.state != RealtimeAudioDeltaReader::State::Done || audio_sink->error) {
        ResetRealtimeEventBuffer(pending);
        return ESP_ERR_INVALID_RESPONSE;
    }

    *event = RealtimeEvent{};
    event->type = "response.audio.delta";
    event->type_len = strlen(event->type);
    event->raw_audio_delta = true;
    event->streamed_audio_bytes = audio_sink->written - before;
    ResetRealtimeEventBuffer(pending);
    return ESP_OK;
}

esp_err_t ReadRealtimeEvent(esp_transport_handle_t ws, RealtimeEventBuffer *pending, char *buf, size_t buf_size,
                            RealtimeEvent *event, bool *closed, RealtimeAudioSink *audio_sink = nullptr) {
    if (event == nullptr || closed == nullptr || pending == nullptr || buf == nullptr || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *event = RealtimeEvent{};
    *closed = false;

    const int r = esp_transport_read(ws, buf, static_cast<int>(buf_size), 5000);
    if (r == 0) {
        return ESP_ERR_TIMEOUT;
    }
    if (r < 0) {
        return ESP_FAIL;
    }

    const ws_transport_opcodes_t opcode = esp_transport_ws_get_read_opcode(ws);
    if (opcode == WS_TRANSPORT_OPCODES_CLOSE) {
        *closed = true;
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (opcode != WS_TRANSPORT_OPCODES_TEXT && opcode != WS_TRANSPORT_OPCODES_CONT) {
        return ESP_ERR_TIMEOUT;
    }

    if (pending->len == 0) {
        pending->expected_len = static_cast<size_t>(std::max(esp_transport_ws_get_read_payload_len(ws), r));
    }
    const size_t next_len = pending->len + static_cast<size_t>(r);
    if (next_len > kRealtimeMaxJsonEventBytes) {
        if (audio_sink != nullptr) {
            return SkipOversizedRealtimeEvent(ws, pending, buf, buf_size, next_len, event, closed);
        }
        DebugSerial::LogAlways("[ORACLE]", "realtime event too large pending=%u",
                               static_cast<unsigned>(next_len));
        ResetRealtimeEventBuffer(pending);
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(pending->data + pending->len, buf, static_cast<size_t>(r));
    pending->len = next_len;
    if (pending->expected_len > kRealtimeMaxJsonEventBytes && audio_sink != nullptr &&
        PendingRealtimeEventIsAudioDelta(pending)) {
        return StreamRealtimeAudioDeltaEvent(ws, pending, buf, buf_size, event, closed, audio_sink);
    }
    if (pending->len < pending->expected_len) {
        return ESP_ERR_TIMEOUT;
    }
    return ParseRealtimeBufferedEvent(pending, event);
}

void LogRealtimeError(cJSON *j) {
    const cJSON *err_obj = cJSON_GetObjectItemCaseSensitive(j, "error");
    const std::string code = err_obj != nullptr ? JsonStr(err_obj, "code") : "";
    const std::string message = err_obj != nullptr ? JsonStr(err_obj, "message") : "";
    DebugSerial::LogAlways("[ORACLE]", "realtime error code=%s msg=%s",
                           code.substr(0, 48).c_str(), message.substr(0, 160).c_str());
}

void LogRealtimeDone(cJSON *j, uint32_t audio_bytes) {
    const cJSON *response = cJSON_GetObjectItemCaseSensitive(j, "response");
    const std::string status = response != nullptr ? JsonStr(response, "status") : "";
    std::string detail;
    const cJSON *details = response != nullptr ? cJSON_GetObjectItemCaseSensitive(response, "status_details") : nullptr;
    const cJSON *err_obj = details != nullptr ? cJSON_GetObjectItemCaseSensitive(details, "error") : nullptr;
    if (err_obj != nullptr) {
        detail = JsonStr(err_obj, "message");
    }
    DebugSerial::LogAlways("[ORACLE]", "realtime done status=%s audio=%u detail=%s",
                           status.substr(0, 32).c_str(), static_cast<unsigned>(audio_bytes),
                           detail.substr(0, 120).c_str());
}

void AddRealtimeOutputModalities(cJSON *object) {
    cJSON *modalities = cJSON_AddArrayToObject(object, "output_modalities");
    cJSON_AddItemToArray(modalities, cJSON_CreateString("audio"));
}

void AddRealtimeAudioOutput(cJSON *object, const char *voice) {
    cJSON *audio = cJSON_AddObjectToObject(object, "audio");
    cJSON *output = cJSON_AddObjectToObject(audio, "output");
    cJSON *format = cJSON_AddObjectToObject(output, "format");
    cJSON_AddStringToObject(format, "type", "audio/pcm");
    cJSON_AddNumberToObject(format, "rate", kRealtimeAudioRateHz);
    cJSON_AddStringToObject(output, "voice", voice);
}

std::string PrintJson(cJSON *root) {
    char *body_cstr = cJSON_PrintUnformatted(root);
    const std::string body = body_cstr != nullptr ? body_cstr : "";
    cJSON_free(body_cstr);
    cJSON_Delete(root);
    return body;
}

std::string RealtimeVoiceInstructions(const std::string &voice_direction) {
    std::string instructions =
        "Speak exactly the provided text and do not add, omit, translate, or summarize any words. "
        "Perform it as a theatrical seer in the throes of a vision: dramatic and expressive, with a voice that "
        "rises and falls, swelling on the key words and hushing on the secrets. Keep it intelligible and at a "
        "normal speaking volume, not a whisper.";
    if (!voice_direction.empty()) {
        instructions = voice_direction + ". " + instructions;
    }
    return instructions;
}

std::string BuildRealtimeConversationItemCreate(const std::string &tts_text) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "conversation.item.create");
    cJSON *item = cJSON_AddObjectToObject(root, "item");
    cJSON_AddStringToObject(item, "type", "message");
    cJSON_AddStringToObject(item, "role", "user");
    cJSON *content = cJSON_AddArrayToObject(item, "content");
    cJSON *part = cJSON_CreateObject();
    cJSON_AddStringToObject(part, "type", "input_text");
    cJSON_AddStringToObject(part, "text", tts_text.c_str());
    cJSON_AddItemToArray(content, part);
    return PrintJson(root);
}

std::string BuildRealtimeResponseCreate(const std::string &voice_direction, const char *voice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "response.create");
    cJSON *response = cJSON_AddObjectToObject(root, "response");
    AddRealtimeOutputModalities(response);
    AddRealtimeAudioOutput(response, voice);
    cJSON_AddStringToObject(response, "instructions", RealtimeVoiceInstructions(voice_direction).c_str());
    return PrintJson(root);
}

esp_err_t SynthesizeRealtime(const OpenAiSettings &openai, const std::string &tts_text,
                             const std::string &voice_direction, const esp_partition_t *spool,
                             uint32_t *answer_audio_bytes) {
    if (answer_audio_bytes == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *answer_audio_bytes = 0;
    ESP_RETURN_ON_ERROR(WritePcm16WavHeader(spool, kVoiceAnswerSpoolOffset, 0), "ORACLE",
                        "realtime initial wav header");

    esp_transport_handle_t ssl = esp_transport_ssl_init();
    if (ssl == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    esp_transport_ssl_crt_bundle_attach(ssl, esp_crt_bundle_attach);
#if CONFIG_MBEDTLS_DYNAMIC_BUFFER
    // Realtime streams many TLS records. Keeping RX static after handshake avoids
    // repeated 16 KB dynamic reallocations after audio writes fragment the heap.
    esp_transport_ssl_set_esp_tls_dyn_buf_strategy(ssl, ESP_TLS_DYN_BUF_RX_STATIC);
#endif

    esp_transport_handle_t ws = esp_transport_ws_init(ssl);
    if (ws == nullptr) {
        esp_transport_destroy(ssl);
        return ESP_ERR_NO_MEM;
    }

    const std::string path = std::string(kRealtimePathPrefix) + openai.speech_model;
    const std::string auth = "Bearer " + openai.api_key;
    const char *voice = SpeechVoiceForModel(openai);
    esp_transport_ws_set_path(ws, path.c_str());
    esp_transport_ws_set_auth(ws, auth.c_str());
    esp_transport_ws_set_user_agent(ws, "bisc8/1.0");

    esp_err_t result = ESP_OK;
    const int64_t t0 = esp_timer_get_time();
    if (esp_transport_connect(ws, kRealtimeHost, 443, kHttpTimeoutMs) < 0) {
        DebugSerial::LogAlways("[ORACLE]", "realtime ws connect failed status=%d",
                               esp_transport_ws_get_upgrade_request_status(ws));
        result = ESP_FAIL;
    }

    RealtimeAudioSink sink = {spool, 0, false};
    static RealtimeEventBuffer pending_event;
    ResetRealtimeEventBuffer(&pending_event);
    char buf[kRealtimeReadChunkBytes];
    bool failed = false;

    // The Realtime server first emits session.created to signal the session is
    // ready. Sending session.update before that point can be ignored by the
    // server, leaving us waiting forever for session.updated.
    bool session_created = false;
    while (result == ESP_OK && !session_created && !failed) {
        if ((esp_timer_get_time() - t0) / 1000 > kRealtimeTimeoutMs) {
            DebugSerial::LogAlways("[ORACLE]", "realtime session.created timed out after %ums",
                                   static_cast<unsigned>(kRealtimeTimeoutMs));
            result = ESP_ERR_TIMEOUT;
            break;
        }
        RealtimeEvent event = {};
        bool closed = false;
        const esp_err_t err = ReadRealtimeEvent(ws, &pending_event, buf, sizeof(buf), &event, &closed, &sink);
        if (err == ESP_ERR_TIMEOUT) {
            continue;
        }
        if (err != ESP_OK) {
            DebugSerial::LogAlways("[ORACLE]", closed ? "realtime ws closed before session.created"
                                                      : "realtime session.created read failed: %s",
                                   esp_err_to_name(err));
            result = err;
            break;
        }

        if (RealtimeEventIs(event, "session.created")) {
            session_created = true;
            DebugSerial::LogAlways("[ORACLE]", "realtime session.created");
        } else if (RealtimeEventIs(event, "error")) {
            LogRealtimeError(event.json);
            failed = true;
        }
        CleanupRealtimeEvent(&pending_event, &event);
    }

    if (result == ESP_OK && !session_created) {
        result = ESP_ERR_INVALID_RESPONSE;
    }
    // This one-shot TTS path configures output modality, format, and voice on
    // response.create. The text to read is first added as a canonical
    // conversation item, then response.create asks the model to speak it.
    if (result == ESP_OK && !WsSendText(ws, BuildRealtimeConversationItemCreate(tts_text))) {
        DebugSerial::LogAlways("[ORACLE]", "realtime conversation.item.create send failed");
        result = ESP_FAIL;
    }
    if (result == ESP_OK && !WsSendText(ws, BuildRealtimeResponseCreate(voice_direction, voice))) {
        DebugSerial::LogAlways("[ORACLE]", "realtime response.create send failed");
        result = ESP_FAIL;
    }

    bool done = false;
    unsigned audio_chunks = 0;
    while (result == ESP_OK && !done && !failed && !sink.error) {
        if ((esp_timer_get_time() - t0) / 1000 > kRealtimeTimeoutMs) {
            DebugSerial::LogAlways("[ORACLE]", "realtime timed out after %ums", static_cast<unsigned>(kRealtimeTimeoutMs));
            result = ESP_ERR_TIMEOUT;
            break;
        }
        RealtimeEvent event = {};
        bool closed = false;
        const esp_err_t err = ReadRealtimeEvent(ws, &pending_event, buf, sizeof(buf), &event, &closed, &sink);
        if (err == ESP_ERR_TIMEOUT) {
            continue;
        }
        if (err != ESP_OK) {
            DebugSerial::LogAlways("[ORACLE]", closed ? "realtime ws closed before response.done"
                                                      : "realtime read failed: %s",
                                   esp_err_to_name(err));
            failed = true;
            break;
        }

        if (RealtimeEventIs(event, "response.audio.delta") ||
            RealtimeEventIs(event, "response.output_audio.delta")) {
            const uint32_t before = sink.written;
            if (event.streamed_audio_bytes > 0) {
                ++audio_chunks;
                if (audio_chunks == 1) {
                    DebugSerial::LogAlways("[ORACLE]", "realtime first audio chunk=%uB",
                                           static_cast<unsigned>(event.streamed_audio_bytes));
                }
            } else if (event.delta == nullptr ||
                !DecodeAndAppendRealtimeBase64(&sink, event.delta, event.delta_len)) {
                failed = true;
            } else if (++audio_chunks == 1) {
                DebugSerial::LogAlways("[ORACLE]", "realtime first audio chunk=%uB",
                                       static_cast<unsigned>(sink.written - before));
            }
        } else if (RealtimeEventIs(event, "response.done")) {
            LogRealtimeDone(event.json, sink.written);
            done = true;
        } else if (RealtimeEventIs(event, "response.output_audio.done") ||
                   RealtimeEventIs(event, "response.audio.done")) {
            DebugSerial::LogAlways("[ORACLE]", "realtime audio.done chunks=%u bytes=%u",
                                   audio_chunks, static_cast<unsigned>(sink.written));
        } else if (RealtimeEventIs(event, "error")) {
            LogRealtimeError(event.json);
            failed = true;
        }
        CleanupRealtimeEvent(&pending_event, &event);
    }

    esp_transport_close(ws);
    esp_transport_destroy(ws);
    esp_transport_destroy(ssl);

    if (result != ESP_OK) {
        return result;
    }
    if (failed || sink.error || !done || sink.written == 0) {
        DebugSerial::LogAlways("[ORACLE]", "realtime failed done=%d audio=%u sink_error=%d",
                               done ? 1 : 0, static_cast<unsigned>(sink.written), sink.error ? 1 : 0);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_RETURN_ON_ERROR(WritePcm16WavHeader(spool, kVoiceAnswerSpoolOffset, sink.written), "ORACLE",
                        "realtime final wav header");
    *answer_audio_bytes = 44 + sink.written;
    const long long ms = (esp_timer_get_time() - t0) / 1000;
    DebugSerial::LogAlways("[ORACLE]", "realtime tts model=%s voice=%s wav=%uB time=%lldms",
                           openai.speech_model.c_str(), voice, static_cast<unsigned>(*answer_audio_bytes), ms);
    return ESP_OK;
}

// Append response bytes into the std::string handed in via user_data.
esp_err_t HttpCollect(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data != nullptr && evt->data_len > 0) {
        static_cast<std::string *>(evt->user_data)->append(static_cast<const char *>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

// Streams a (large, audio) response straight into a flash region, never holding
// it all in RAM. The region must be pre-erased and big enough.
struct FlashSink {
    const esp_partition_t *part;
    uint32_t base;
    uint32_t cap;
    uint32_t written;
    bool error;
};

esp_err_t HttpToFlash(esp_http_client_event_t *evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA || evt->user_data == nullptr || evt->data_len <= 0) {
        return ESP_OK;
    }
    auto *sink = static_cast<FlashSink *>(evt->user_data);
    if (sink->error) {
        return ESP_OK;
    }
    if (sink->written + static_cast<uint32_t>(evt->data_len) > sink->cap) {
        sink->error = true;  // answer audio larger than the reserved region
        return ESP_OK;
    }
    if (esp_partition_write(sink->part, sink->base + sink->written, evt->data, evt->data_len) != ESP_OK) {
        sink->error = true;
        return ESP_OK;
    }
    sink->written += static_cast<uint32_t>(evt->data_len);
    return ESP_OK;
}

// HTTPS POST a JSON body to OpenAI (Bearer auth, cert-bundle TLS) and collect the
// JSON response. Never logs the key.
esp_err_t OpenAiPostJson(const char *url, const std::string &api_key, const std::string &body,
                         std::string *out, int *status) {
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.method = HTTP_METHOD_POST;
    cfg.event_handler = HttpCollect;
    cfg.user_data = out;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = kHttpTimeoutMs;
    cfg.buffer_size = 2048;
    cfg.buffer_size_tx = 1024;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        return ESP_FAIL;
    }
    const std::string auth = "Bearer " + api_key;
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth.c_str());
    esp_http_client_set_post_field(client, body.c_str(), static_cast<int>(body.size()));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        *status = esp_http_client_get_status_code(client);
    }
    esp_http_client_cleanup(client);
    return err;
}

// Truncate to at most max_chars UTF-8 code points without splitting a sequence.
std::string Utf8Truncate(const std::string &s, size_t max_chars) {
    size_t chars = 0;
    size_t i = 0;
    while (i < s.size() && chars < max_chars) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if (c >= 0xF0) {
            len = 4;
        } else if (c >= 0xE0) {
            len = 3;
        } else if (c >= 0xC0) {
            len = 2;
        }
        if (i + len > s.size()) {
            break;
        }
        i += len;
        ++chars;
    }
    return s.substr(0, i);
}

bool ReadUtf8Codepoint(const std::string &s, size_t *offset, uint32_t *codepoint) {
    if (offset == nullptr || codepoint == nullptr || *offset >= s.size()) {
        return false;
    }
    const unsigned char first = static_cast<unsigned char>(s[*offset]);
    if (first < 0x80) {
        *codepoint = first;
        ++(*offset);
        return true;
    }

    size_t len = 0;
    uint32_t cp = 0;
    uint32_t min_cp = 0;
    if ((first & 0xE0) == 0xC0) {
        len = 2;
        cp = first & 0x1F;
        min_cp = 0x80;
    } else if ((first & 0xF0) == 0xE0) {
        len = 3;
        cp = first & 0x0F;
        min_cp = 0x800;
    } else if ((first & 0xF8) == 0xF0) {
        len = 4;
        cp = first & 0x07;
        min_cp = 0x10000;
    } else {
        ++(*offset);
        *codepoint = '?';
        return false;
    }

    if (*offset + len > s.size()) {
        *offset = s.size();
        *codepoint = '?';
        return false;
    }
    for (size_t i = 1; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[*offset + i]);
        if ((c & 0xC0) != 0x80) {
            ++(*offset);
            *codepoint = '?';
            return false;
        }
        cp = (cp << 6) | (c & 0x3F);
    }
    *offset += len;
    if (cp < min_cp || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        *codepoint = '?';
        return false;
    }
    *codepoint = cp;
    return true;
}

void AppendUtf8Codepoint(std::string *out, uint32_t cp) {
    if (out == nullptr) {
        return;
    }
    if (cp < 0x80) {
        out->push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out->push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out->push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out->push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

void AppendScreenSpace(std::string *out, bool *last_space) {
    if (out == nullptr || last_space == nullptr || out->empty() || *last_space) {
        return;
    }
    out->push_back(' ');
    *last_space = true;
}

void AppendScreenAscii(std::string *out, const char *text, bool *last_space) {
    if (out == nullptr || text == nullptr || last_space == nullptr) {
        return;
    }
    for (const char *p = text; *p != '\0'; ++p) {
        const char c = *p;
        if (c == ' ') {
            AppendScreenSpace(out, last_space);
        } else {
            out->push_back(c);
            *last_space = false;
        }
    }
}

std::string SanitizeScreenTextForDisplay(const std::string &input) {
    std::string out;
    bool last_space = true;  // trims leading whitespace
    size_t i = 0;
    while (i < input.size()) {
        uint32_t cp = 0;
        const bool valid = ReadUtf8Codepoint(input, &i, &cp);
        if (!valid) {
            AppendScreenAscii(&out, "?", &last_space);
            continue;
        }

        if (cp == '\n' || cp == '\r' || cp == '\t' || cp == 0x00A0) {
            AppendScreenSpace(&out, &last_space);
        } else if (cp == 0x00AD) {
            continue;  // soft hyphen
        } else if ((cp >= 0x20 && cp <= 0x7E) || (cp >= 0xA1 && cp <= 0xFF)) {
            AppendUtf8Codepoint(&out, cp);
            last_space = false;
        } else if (cp == 0x2018 || cp == 0x2019 || cp == 0x02BC) {
            AppendScreenAscii(&out, "'", &last_space);
        } else if (cp == 0x201C || cp == 0x201D) {
            AppendScreenAscii(&out, "\"", &last_space);
        } else if (cp == 0x2013 || cp == 0x2014 || cp == 0x2212) {
            AppendScreenAscii(&out, "-", &last_space);
        } else if (cp == 0x2026) {
            AppendScreenAscii(&out, "...", &last_space);
        } else {
            AppendScreenSpace(&out, &last_space);
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

std::string ScreenFallbackForLanguage(const std::string &language) {
    if (language.rfind("en", 0) == 0) {
        return "The vision arrives.";
    }
    if (language.rfind("es", 0) == 0) {
        return "La vision llega.";
    }
    if (language.rfind("fr", 0) == 0) {
        return "La vision arrive.";
    }
    return "La visione arriva.";
}

std::string JsonStr(const cJSON *obj, const char *key) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        return item->valuestring;
    }
    return "";
}

}  // namespace

const char *VoiceOracleService::AnswerAudioPath() {
    return kAnswerSpoolUri;
}

void VoiceOracleService::Reset() {
    detected_language_.clear();
    transcript_.clear();
    answer_full_.clear();
    answer_screen_.clear();
    tts_text_.clear();
    voice_direction_.clear();
    answer_audio_ready_ = false;
    answer_audio_bytes_ = 0;
    last_failure_ = OracleFailure::None;
}

void VoiceOracleService::FillResponse(OracleResponse *response) {
    if (response == nullptr) {
        return;
    }
    response->detected_language = detected_language_.c_str();
    response->transcript = transcript_.c_str();
    response->oracle_answer_full = answer_full_.c_str();
    response->oracle_answer_screen = answer_screen_.c_str();
    response->tts_text = tts_text_.c_str();
}

esp_err_t VoiceOracleService::Transcribe(const OpenAiSettings &openai) {
    last_failure_ = OracleFailure::Transcribe;  // any failure here is a transcribe error...
    const esp_partition_t *spool =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);
    if (spool == nullptr) {
        DebugSerial::LogAlways("[ORACLE]", "stt: spool partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t header[44] = {};
    esp_err_t err = esp_partition_read(spool, 0, header, sizeof(header));
    if (err != ESP_OK) {
        return err;
    }
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        DebugSerial::LogAlways("[ORACLE]", "stt: recorded spool is not a WAV");
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t data_bytes = static_cast<uint32_t>(header[40]) | (static_cast<uint32_t>(header[41]) << 8) |
                                (static_cast<uint32_t>(header[42]) << 16) | (static_cast<uint32_t>(header[43]) << 24);
    const size_t wav_total = 44 + data_bytes;
    if (data_bytes == 0 || wav_total > static_cast<size_t>(spool->size)) {
        DebugSerial::LogAlways("[ORACLE]", "stt: bad WAV data size=%u", static_cast<unsigned>(data_bytes));
        return ESP_ERR_INVALID_SIZE;
    }

    const std::string boundary = kMultipartBoundary;
    std::string preamble;
    preamble += "--" + boundary + "\r\n";
    preamble += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    preamble += openai.transcription_model + "\r\n";
    preamble += "--" + boundary + "\r\n";
    preamble += "Content-Disposition: form-data; name=\"prompt\"\r\n\r\n";
    preamble += std::string(kTranscriptionPrompt) + "\r\n";
    preamble += "--" + boundary + "\r\n";
    preamble += "Content-Disposition: form-data; name=\"temperature\"\r\n\r\n";
    preamble += "0\r\n";
    preamble += "--" + boundary + "\r\n";
    preamble += "Content-Disposition: form-data; name=\"file\"; filename=\"question.wav\"\r\n";
    preamble += "Content-Type: audio/wav\r\n\r\n";
    const std::string epilogue = "\r\n--" + boundary + "--\r\n";
    const int content_length = static_cast<int>(preamble.size() + wav_total + epilogue.size());

    esp_http_client_config_t cfg = {};
    cfg.url = kTranscribeUrl;
    cfg.method = HTTP_METHOD_POST;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = kHttpTimeoutMs;
    cfg.buffer_size = 2048;
    cfg.buffer_size_tx = 2048;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        return ESP_FAIL;
    }
    const std::string auth = "Bearer " + openai.api_key;
    const std::string content_type = "multipart/form-data; boundary=" + boundary;
    esp_http_client_set_header(client, "Authorization", auth.c_str());
    esp_http_client_set_header(client, "Content-Type", content_type.c_str());

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

    const int64_t t0 = esp_timer_get_time();
    err = esp_http_client_open(client, content_length);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        DebugSerial::LogAlways("[ORACLE]", "stt open failed: %s", esp_err_to_name(err));
        return err;
    }

    bool ok = write_all(preamble.data(), preamble.size());
    static uint8_t chunk[4096];
    size_t off = 0;
    size_t remaining = wav_total;
    while (ok && remaining > 0) {
        const size_t n = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        if (esp_partition_read(spool, off, chunk, n) != ESP_OK) {
            ok = false;
            break;
        }
        ok = write_all(reinterpret_cast<const char *>(chunk), n);
        off += n;
        remaining -= n;
    }
    if (ok) {
        ok = write_all(epilogue.data(), epilogue.size());
    }

    int status = 0;
    std::string resp;
    if (ok) {
        esp_http_client_fetch_headers(client);
        status = esp_http_client_get_status_code(client);
        char rbuf[512];
        int r = 0;
        while ((r = esp_http_client_read(client, rbuf, sizeof(rbuf))) > 0) {
            resp.append(rbuf, static_cast<size_t>(r));
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    const long long ms = (esp_timer_get_time() - t0) / 1000;
    DebugSerial::LogAlways("[ORACLE]", "stt model=%s wav=%uB status=%d time=%lldms",
                           openai.transcription_model.c_str(), static_cast<unsigned>(wav_total), status, ms);
    if (!ok) {
        return ESP_FAIL;
    }
    if (status != 200) {
        DebugSerial::LogAlways("[ORACLE]", "stt http status=%d body=%s", status, resp.substr(0, 400).c_str());
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON *j = cJSON_Parse(resp.c_str());
    if (j == nullptr) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    transcript_ = JsonStr(j, "text");
    const std::string lang = JsonStr(j, "language");
    if (!lang.empty()) {
        detected_language_ = lang;
    }
    cJSON_Delete(j);
    if (transcript_.empty()) {
        DebugSerial::LogAlways("[ORACLE]", "stt produced an empty transcript");
        last_failure_ = OracleFailure::NoSpeech;  // ...except nothing was heard
        return ESP_ERR_INVALID_RESPONSE;
    }
    DebugSerial::LogAlways("[ORACLE]", "stt ok transcript='%s'", transcript_.c_str());
    last_failure_ = OracleFailure::None;
    return ESP_OK;
}

esp_err_t VoiceOracleService::GenerateAnswer(const OpenAiSettings &openai, const std::string &device_language) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", openai.response_model.c_str());
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", kBrainSystemPrompt);
    cJSON_AddItemToArray(messages, sys);
    cJSON *usr = cJSON_CreateObject();
    cJSON_AddStringToObject(usr, "role", "user");
    // Reply in the language of the TRANSCRIPT itself (the words the user actually
    // said), which the model reads directly. This is more reliable than trusting
    // the STT's separate language tag, which can mislabel short/accented speech.
    // The device UI language is only a tiebreaker for a truly ambiguous transcript
    // or for subtitle-like hallucinations from weak/noisy audio.
    const std::string user_content =
        "A person asked the oracle the following (transcribed). Reply in the SAME language as their "
        "words below, whatever language that is. The device UI language is " + device_language +
        ". If the transcript is truly ambiguous, mostly non-Latin, or looks like accidental captions, "
        "subtitles, lyrics, or background speech rather than a short oracle question, prefer " + device_language +
        ".\n\nTheir words:\n" + transcript_ + "\n\nReturn only JSON.";
    cJSON_AddStringToObject(usr, "content", user_content.c_str());
    cJSON_AddItemToArray(messages, usr);
    cJSON *rf = cJSON_AddObjectToObject(root, "response_format");
    cJSON_AddStringToObject(rf, "type", "json_object");
    cJSON_AddNumberToObject(root, "max_completion_tokens", 700);  // room for reasoning + the JSON
    if (!openai.reasoning_effort.empty()) {
        cJSON_AddStringToObject(root, "reasoning_effort", openai.reasoning_effort.c_str());
        // Reasoning models reject a custom temperature; leave it at the default.
    } else {
        cJSON_AddNumberToObject(root, "temperature", 0.85);
    }
    char *body_cstr = cJSON_PrintUnformatted(root);
    const std::string body = body_cstr != nullptr ? body_cstr : "";
    cJSON_free(body_cstr);
    cJSON_Delete(root);

    std::string resp;
    int status = 0;
    DebugSerial::LogAlways("[ORACLE]", "brain POST model=%s transcript_len=%u free_heap=%u",
                           openai.response_model.c_str(), static_cast<unsigned>(transcript_.size()),
                           static_cast<unsigned>(esp_get_free_heap_size()));
    const int64_t t0 = esp_timer_get_time();
    const esp_err_t err = OpenAiPostJson(kChatUrl, openai.api_key, body, &resp, &status);
    const long long brain_ms = (esp_timer_get_time() - t0) / 1000;
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[ORACLE]", "brain http error: %s", esp_err_to_name(err));
        return err;
    }
    if (status != 200) {
        DebugSerial::LogAlways("[ORACLE]", "brain http status=%d body=%s", status, resp.substr(0, 400).c_str());
        return ESP_ERR_INVALID_RESPONSE;
    }

    cJSON *outer = cJSON_Parse(resp.c_str());
    if (outer == nullptr) {
        DebugSerial::LogAlways("[ORACLE]", "brain outer JSON parse failed");
        return ESP_ERR_INVALID_RESPONSE;
    }
    const cJSON *choices = cJSON_GetObjectItemCaseSensitive(outer, "choices");
    const cJSON *first = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : nullptr;
    const cJSON *message = first ? cJSON_GetObjectItemCaseSensitive(first, "message") : nullptr;
    const cJSON *content = message ? cJSON_GetObjectItemCaseSensitive(message, "content") : nullptr;
    if (!cJSON_IsString(content) || content->valuestring == nullptr) {
        cJSON_Delete(outer);
        DebugSerial::LogAlways("[ORACLE]", "brain response has no message content");
        return ESP_ERR_INVALID_RESPONSE;
    }
    cJSON *inner = cJSON_Parse(content->valuestring);
    cJSON_Delete(outer);
    if (inner == nullptr) {
        DebugSerial::LogAlways("[ORACLE]", "brain inner JSON parse failed");
        return ESP_ERR_INVALID_RESPONSE;
    }

    answer_full_ = JsonStr(inner, "oracle_answer_full");
    answer_screen_ = JsonStr(inner, "oracle_answer_screen");
    tts_text_ = JsonStr(inner, "tts_text");
    voice_direction_ = JsonStr(inner, "voice_direction");
    const std::string lang = JsonStr(inner, "detected_language");
    if (!lang.empty()) {
        detected_language_ = lang;
    }
    cJSON_Delete(inner);

    // Derive missing pieces instead of failing outright.
    if (answer_screen_.empty()) {
        answer_screen_ = answer_full_;
    }
    answer_screen_ = SanitizeScreenTextForDisplay(answer_screen_);
    if (answer_screen_.empty()) {
        answer_screen_ = ScreenFallbackForLanguage(device_language);
    }
    answer_screen_ = Utf8Truncate(answer_screen_, kMaxScreenAnswerChars);
    if (tts_text_.empty()) {
        tts_text_ = answer_full_.empty() ? answer_screen_ : answer_full_;
    }
    if (answer_screen_.empty()) {
        DebugSerial::LogAlways("[ORACLE]", "brain produced no usable answer");
        return ESP_ERR_INVALID_RESPONSE;
    }
    DebugSerial::LogAlways("[ORACLE]", "brain ok lang=%s time=%lldms screen='%s'",
                           detected_language_.c_str(), brain_ms, answer_screen_.c_str());
    DebugSerial::LogAlways("[ORACLE]", "brain voice_dir='%s' tts='%s'",
                           voice_direction_.c_str(), tts_text_.c_str());
    return ESP_OK;
}

esp_err_t VoiceOracleService::Synthesize(const OpenAiSettings &openai) {
    if (tts_text_.empty()) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_partition_t *spool =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kSpoolPartition);
    if (spool == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = esp_partition_erase_range(spool, kVoiceAnswerSpoolOffset, kVoiceAnswerSpoolMaxBytes);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[ORACLE]", "tts erase failed: %s", esp_err_to_name(err));
        return err;
    }

    if (IsRealtimeSpeechModel(openai.speech_model)) {
        return SynthesizeRealtime(openai, tts_text_, voice_direction_, spool, &answer_audio_bytes_);
    }

    const char *voice = SpeechVoiceForModel(openai);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", openai.speech_model.c_str());
    cJSON_AddStringToObject(root, "voice", voice);
    cJSON_AddStringToObject(root, "input", tts_text_.c_str());
    if (SpeechModelSupportsInstructions(openai.speech_model)) {
        // The oracle's spoken style is pinned here so it stays consistent when
        // the selected speech model accepts expressive directions.
        std::string instructions =
            "Speak as a theatrical seer in the throes of a vision: dramatic and expressive, with a voice that "
            "rises and falls -- swelling on the key words, hushing on the secrets. Bold emphasis, pregnant "
            "pauses, a medium half in trance, yet always intelligible and at a normal speaking volume, NOT a "
            "whisper. Let the prophecy breathe and build.";
        if (!voice_direction_.empty()) {
            instructions = voice_direction_ + ". " + instructions;
        }
        cJSON_AddStringToObject(root, "instructions", instructions.c_str());
    }
    cJSON_AddStringToObject(root, "response_format", "wav");
    char *body_cstr = cJSON_PrintUnformatted(root);
    const std::string body = body_cstr != nullptr ? body_cstr : "";
    cJSON_free(body_cstr);
    cJSON_Delete(root);

    FlashSink sink = {spool, kVoiceAnswerSpoolOffset, kVoiceAnswerSpoolMaxBytes, 0, false};
    esp_http_client_config_t cfg = {};
    cfg.url = kSpeechUrl;
    cfg.method = HTTP_METHOD_POST;
    cfg.event_handler = HttpToFlash;
    cfg.user_data = &sink;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.timeout_ms = kHttpTimeoutMs;
    cfg.buffer_size = 4096;
    cfg.buffer_size_tx = 1024;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        return ESP_FAIL;
    }
    const std::string auth = "Bearer " + openai.api_key;
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth.c_str());
    esp_http_client_set_post_field(client, body.c_str(), static_cast<int>(body.size()));

    const int64_t t0 = esp_timer_get_time();
    err = esp_http_client_perform(client);
    const int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : 0;
    esp_http_client_cleanup(client);
    const long long ms = (esp_timer_get_time() - t0) / 1000;
    DebugSerial::LogAlways("[ORACLE]", "tts model=%s voice=%s status=%d bytes=%u time=%lldms",
                           openai.speech_model.c_str(), voice, status,
                           static_cast<unsigned>(sink.written), ms);
    if (err != ESP_OK) {
        return err;
    }
    if (status != 200 || sink.error || sink.written < 44) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint8_t hd[12] = {};
    esp_partition_read(spool, kVoiceAnswerSpoolOffset, hd, sizeof(hd));
    if (memcmp(hd, "RIFF", 4) != 0 || memcmp(hd + 8, "WAVE", 4) != 0) {
        DebugSerial::LogAlways("[ORACLE]", "tts: response is not a WAV");
        return ESP_ERR_INVALID_RESPONSE;
    }
    answer_audio_bytes_ = sink.written;
    DebugSerial::LogAlways("[ORACLE]", "tts ok wav=%uB -> %s", static_cast<unsigned>(sink.written), kAnswerSpoolUri);
    return ESP_OK;
}

esp_err_t VoiceOracleService::AskTextAnswer(const char *wav_path, const OpenAiSettings &openai,
                                            OracleResponse *response) {
    return AskTextAnswer(wav_path, openai, "it", response);
}

esp_err_t VoiceOracleService::AskTextAnswer(const char *wav_path, const OpenAiSettings &openai,
                                            const std::string &device_language, OracleResponse *response) {
    Reset();
    if (response != nullptr) {
        response->detected_language = "";
        response->transcript = "";
        response->oracle_answer_full = "";
        response->oracle_answer_screen = "";
        response->tts_text = "";
        response->stt_model = "";
        response->brain_model = "";
        response->tts_model = "";
        response->voice = "";
    }
    if (openai.api_key.empty()) {
        DebugSerial::LogAlways("[ORACLE]", "no OpenAI key configured");
        last_failure_ = OracleFailure::NoKey;
        return ESP_ERR_INVALID_STATE;
    }
    const std::string fallback_language = device_language.empty() ? "it" : device_language;
    DebugSerial::LogAlways("[ORACLE]", "voice flow start wav=%s free_heap=%u",
                           wav_path == nullptr ? "(null)" : wav_path,
                           static_cast<unsigned>(esp_get_free_heap_size()));

    detected_language_ = fallback_language;  // overwritten by STT / brain when detected
    esp_err_t err = Transcribe(openai);
    if (err != ESP_OK) {
        return err;  // Transcribe set last_failure_ (NoSpeech / Transcribe)
    }
    err = GenerateAnswer(openai, fallback_language);
    if (err != ESP_OK) {
        last_failure_ = OracleFailure::Brain;
        return err;
    }

    FillResponse(response);
    if (response != nullptr) {
        // Engines used this run (point into the stable settings.openai). For the email.
        response->stt_model = openai.transcription_model.c_str();
        response->brain_model = openai.response_model.c_str();
        response->tts_model = openai.speech_model.c_str();
        response->voice = openai.voice.c_str();
    }
    return ESP_OK;
}

esp_err_t VoiceOracleService::SpeakAnswer(const OpenAiSettings &openai) {
    // TTS failure is non-fatal: the caller still shows the screen answer.
    answer_audio_ready_ = (Synthesize(openai) == ESP_OK);
    if (!answer_audio_ready_) {
        DebugSerial::LogAlways("[ORACLE]", "tts failed; showing screen answer without audio");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t VoiceOracleService::AskFromRecordedAudio(const char *wav_path, const OpenAiSettings &openai,
                                                   OracleResponse *response) {
    // Convenience wrapper: text answer, then spoken audio. The live device flow runs
    // the two phases separately (see app_main) so it can paint the answer on screen
    // before the slow TTS download instead of looking frozen until audio arrives.
    const esp_err_t err = AskTextAnswer(wav_path, openai, response);
    if (err != ESP_OK) {
        return err;
    }
    SpeakAnswer(openai);  // non-fatal
    return ESP_OK;
}

}  // namespace bisc8
