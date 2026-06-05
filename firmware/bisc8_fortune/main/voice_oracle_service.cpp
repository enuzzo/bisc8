#include "voice_oracle_service.h"

#include <string.h>

#include <string>

#include <cJSON.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_partition.h>
#include <esp_timer.h>

#include "app_config.h"
#include "debug_serial.h"

namespace bisc8 {

namespace {

constexpr const char *kChatUrl = "https://api.openai.com/v1/chat/completions";
constexpr const char *kTranscribeUrl = "https://api.openai.com/v1/audio/transcriptions";
constexpr const char *kSpeechUrl = "https://api.openai.com/v1/audio/speech";
constexpr const char *kAnswerSpoolUri = "spool://answer.wav";
constexpr const char *kSpoolPartition = "spool";
constexpr const char *kMultipartBoundary = "----bisc8oracleMUL7boundaryZ9";
constexpr uint32_t kHttpTimeoutMs = 25000;

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
    "- oracle_answer_screen: AT MOST 55 characters, one short clear line for a tiny 200x200 screen, no newlines\n"
    "- oracle_answer_full: 1 to 3 short sentences, for voice\n"
    "- tts_text: the exact spoken script to read aloud (1 to 4 short lines)\n"
    "- voice_direction: a short English note on the emotional colour of THIS answer (e.g. tender, knowing, hopeful, consoling)\n"
    "Rules: same language as the user; always speak to their specific question, even through metaphor; never say you are an AI; "
    "do not give medical, legal, financial or safety-critical advice as certainty; poetic and evocative, never nonsense; "
    "concise. Avoid long dashes.";

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
    // The device UI language is only a tiebreaker for a truly ambiguous transcript.
    const std::string user_content =
        "A person asked the oracle the following (transcribed). Reply in the SAME language as their "
        "words below, whatever language that is. If it is truly ambiguous, prefer " + device_language +
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
    DebugSerial::LogAlways("[ORACLE]", "brain POST model=%s transcript_len=%u",
                           openai.response_model.c_str(), static_cast<unsigned>(transcript_.size()));
    const esp_err_t err = OpenAiPostJson(kChatUrl, openai.api_key, body, &resp, &status);
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
    answer_screen_ = Utf8Truncate(answer_screen_, kMaxScreenAnswerChars);
    if (tts_text_.empty()) {
        tts_text_ = answer_full_.empty() ? answer_screen_ : answer_full_;
    }
    if (answer_screen_.empty()) {
        DebugSerial::LogAlways("[ORACLE]", "brain produced no usable answer");
        return ESP_ERR_INVALID_RESPONSE;
    }
    DebugSerial::LogAlways("[ORACLE]", "brain ok lang=%s screen='%s'",
                           detected_language_.c_str(), answer_screen_.c_str());
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

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", openai.speech_model.c_str());
    cJSON_AddStringToObject(root, "voice", openai.voice.empty() ? "coral" : openai.voice.c_str());
    cJSON_AddStringToObject(root, "input", tts_text_.c_str());
    // The oracle's spoken style is pinned here so it stays consistent: a hushed,
    // mystical diviner revealing a vision. voice_direction_ adds the per-answer
    // emotional colour on top.
    std::string instructions =
        "Speak as a theatrical seer in the throes of a vision: dramatic and expressive, with a voice that "
        "rises and falls -- swelling on the key words, hushing on the secrets. Bold emphasis, pregnant "
        "pauses, a medium half in trance, yet always intelligible and at a normal speaking volume, NOT a "
        "whisper. Let the prophecy breathe and build.";
    if (!voice_direction_.empty()) {
        instructions = voice_direction_ + ". " + instructions;
    }
    cJSON_AddStringToObject(root, "instructions", instructions.c_str());
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
                           openai.speech_model.c_str(), openai.voice.c_str(), status,
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

esp_err_t VoiceOracleService::AskFromRecordedAudio(const char *wav_path, const OpenAiSettings &openai,
                                                   OracleResponse *response) {
    Reset();
    if (response != nullptr) {
        response->detected_language = "";
        response->transcript = "";
        response->oracle_answer_full = "";
        response->oracle_answer_screen = "";
        response->tts_text = "";
    }
    if (openai.api_key.empty()) {
        DebugSerial::LogAlways("[ORACLE]", "no OpenAI key configured");
        last_failure_ = OracleFailure::NoKey;
        return ESP_ERR_INVALID_STATE;
    }
    DebugSerial::LogAlways("[ORACLE]", "voice flow start wav=%s", wav_path == nullptr ? "(null)" : wav_path);

    detected_language_ = "it";  // overwritten by STT / brain when detected
    esp_err_t err = Transcribe(openai);
    if (err != ESP_OK) {
        return err;  // Transcribe set last_failure_ (NoSpeech / Transcribe)
    }
    err = GenerateAnswer(openai, detected_language_);
    if (err != ESP_OK) {
        last_failure_ = OracleFailure::Brain;
        return err;
    }
    // TTS failure is non-fatal: still show the screen answer.
    answer_audio_ready_ = (Synthesize(openai) == ESP_OK);
    if (!answer_audio_ready_) {
        DebugSerial::LogAlways("[ORACLE]", "tts failed; showing screen answer without audio");
    }

    FillResponse(response);
    return ESP_OK;
}

}  // namespace bisc8
