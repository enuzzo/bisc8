#include "web_portal.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <esp_system.h>
#include <esp_wifi.h>

#include "app_config.h"
#include "connectivity_service.h"
#include "debug_serial.h"

namespace bisc8 {

namespace {

constexpr size_t kMaxFormBodyBytes = 4096;
constexpr uint16_t kMaxScanRecords = 20;
using FormFields = std::vector<std::pair<std::string, std::string>>;

const char *const kPortalRoutes[] = {
    "/",
    "/api/status",
    "/api/wifi/scan",
    "/api/wifi/credentials",
    "/api/settings",
    "/api/openai",
    "/api/email",
    "/api/smtp",
    "/api/reset",
};

const char *const kCaptiveProbePaths[] = {
    "/generate_204",
    "/hotspot-detect.html",
    "/ncsi.txt",
    "/connecttest.txt",
};

const char *const kIndexHtml = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bisc8 Setup</title>
<style>
:root{color-scheme:light;--ink:#17130f;--muted:#675f55;--line:#d8d0c4;--paper:#fbfaf7;--panel:#ffffff;--soft:#f1eee7;--accent:#0f766e;--accent2:#8a4f12;--bad:#9f2a2a;--ok:#17633a}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 18% 0,#eef8f5 0 220px,transparent 221px),linear-gradient(180deg,#fff,#f5f1e9);color:var(--ink);font:15px/1.42 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}
main{width:min(980px,100%);margin:0 auto;padding:22px 14px 34px}.top{display:grid;grid-template-columns:1fr;gap:14px;margin-bottom:14px}.brand{display:flex;align-items:center;gap:12px}.mark{width:48px;height:48px;border:2px solid var(--ink);display:grid;place-items:center;font:700 17px/1 Georgia,serif;background:#fff;box-shadow:4px 4px 0 var(--ink)}h1{margin:0;font:700 28px/1.05 Georgia,serif;letter-spacing:0}.sub{margin:3px 0 0;color:var(--muted)}
.status{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}.pill{border:1px solid var(--line);background:rgba(255,255,255,.82);padding:9px 10px}.pill b{display:block;font-size:11px;text-transform:uppercase;color:var(--muted);letter-spacing:.04em}.pill span{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.grid{display:grid;grid-template-columns:1fr;gap:12px}.panel{background:rgba(255,255,255,.88);border:1px solid var(--line);box-shadow:0 12px 28px rgba(40,31,20,.07);padding:15px}.panel h2{margin:0 0 12px;font:700 19px/1.15 Georgia,serif}.row{display:grid;grid-template-columns:1fr;gap:9px;margin-bottom:10px}label{font-size:12px;font-weight:700;color:var(--muted);text-transform:uppercase;letter-spacing:.035em}input,select{width:100%;min-height:42px;border:1px solid var(--line);background:#fff;color:var(--ink);padding:10px 11px;font:15px/1.2 inherit;border-radius:0}input:focus,select:focus{outline:2px solid rgba(15,118,110,.28);border-color:var(--accent)}
.actions{display:flex;flex-wrap:wrap;gap:8px;align-items:center}.btn{min-height:40px;border:1px solid var(--ink);background:var(--ink);color:#fff;padding:9px 13px;font:700 14px/1 inherit;cursor:pointer}.btn.secondary{background:#fff;color:var(--ink)}.btn.warn{background:var(--bad);border-color:var(--bad)}.btn:disabled{opacity:.58;cursor:wait}.hint{margin:8px 0 0;color:var(--muted);font-size:13px}.scan{border:1px solid var(--line);background:var(--soft);padding:10px;margin:8px 0;max-height:156px;overflow:auto}.scan button{display:block;width:100%;text-align:left;border:0;border-bottom:1px solid var(--line);background:transparent;padding:9px 4px;color:var(--ink)}.scan button:last-child{border-bottom:0}.toast{position:sticky;bottom:10px;margin-top:12px;background:var(--ink);color:#fff;padding:10px 12px;display:none}.toast.show{display:block}.ok{color:var(--ok)}.danger{color:var(--bad)}
@media (min-width:760px){main{padding:30px 18px 44px}.top{grid-template-columns:1.15fr .85fr;align-items:end}.grid{grid-template-columns:repeat(2,minmax(0,1fr))}.panel.wide{grid-column:1/-1}.row.two{grid-template-columns:1fr 1fr}.status{grid-template-columns:repeat(2,minmax(0,1fr))}}
</style>
</head>
<body>
<main>
<section class="top">
<div class="brand"><div class="mark">B8</div><div><h1 data-i18n="appTitle">Bisc8 Setup</h1><p class="sub" data-i18n="subtitle">Local configuration for the small oracle.</p></div></div>
<div class="status">
<div class="pill"><b data-i18n="mode">Mode</b><span data-bind="wifi_mode">setup mode</span></div>
<div class="pill"><b data-i18n="address">Address</b><span data-bind="device_address">http://192.168.4.1</span></div>
<div class="pill"><b data-i18n="network">Network</b><span data-bind="network_label">setup mode</span></div>
<div class="pill"><b data-i18n="language">Language</b><span data-bind="language">en</span></div>
</div>
</section>
<section class="grid">
<article class="panel">
<h2 data-i18n="wifiTitle">Wi-Fi</h2>
<div class="actions"><button class="btn secondary" type="button" id="scan" data-i18n="scan">Scan</button><span class="hint" id="scanState" data-i18n="scanState">Saved networks stay on the device.</span></div>
<div class="scan" id="scanList"></div>
<form data-api="/api/wifi/credentials">
<div class="row"><label for="ssid" data-i18n="ssid">SSID</label><input id="ssid" name="ssid" autocomplete="off" required></div>
<div class="row"><label for="password" data-i18n="password">Password</label><input id="password" name="password" type="password" autocomplete="new-password"></div>
<div class="actions"><button class="btn" type="submit" data-i18n="saveWifi">Save Wi-Fi</button></div>
</form>
<form data-api="/api/wifi/credentials" class="remove">
<input type="hidden" name="action" value="remove">
<div class="row"><label for="remove_index" data-i18n="removeSlot">Remove saved slot</label><input id="remove_index" name="index" inputmode="numeric" placeholder="0"></div>
<div class="actions"><button class="btn secondary" type="submit" data-i18n="remove">Remove</button></div>
</form>
</article>
<article class="panel">
<h2 data-i18n="languageTitle">Language</h2>
<form data-api="/api/settings">
<div class="row"><label for="language" data-i18n="displayLanguage">Display language</label><select id="language" name="language"><option value="en">English</option><option value="es">Español</option><option value="it">Italiano</option></select></div>
<div class="actions"><button class="btn" type="submit" data-i18n="saveLanguage">Save language</button></div>
</form>
<p class="hint" data-i18n="firstBoot">First boot starts in English.</p>
</article>
<article class="panel">
<h2 data-i18n="openaiTitle">OpenAI</h2>
<form data-api="/api/openai">
<div class="row"><label for="api_key" data-i18n="apiKey">API key</label><input id="api_key" name="api_key" type="password" placeholder="Leave blank to keep current key" data-i18n-placeholder="keepKey"></div>
<div class="row two"><div><label for="transcription_model" data-i18n="stt">Speech to text</label><input id="transcription_model" name="transcription_model" placeholder="gpt-4o-mini-transcribe"></div><div><label for="response_model" data-i18n="oracleModel">Oracle model</label><input id="response_model" name="response_model" placeholder="gpt-4o-mini"></div></div>
<div class="row two"><div><label for="speech_model" data-i18n="tts">Text to speech</label><input id="speech_model" name="speech_model" placeholder="gpt-4o-mini-tts"></div><div><label for="voice" data-i18n="voice">Voice</label><input id="voice" name="voice" placeholder="alloy"></div></div>
<div class="actions"><button class="btn" type="submit" data-i18n="saveOpenai">Save OpenAI</button></div>
</form>
<p class="hint"><span data-i18n="storedKey">Stored key</span>: <span data-bind="openai_key">missing</span></p>
</article>
<article class="panel">
<h2 data-i18n="emailTitle">Email</h2>
<form data-api="/api/email">
<div class="row"><label for="recipient" data-i18n="recipient">Recipient</label><input id="recipient" name="recipient" type="email" placeholder="you@example.com"></div>
<div class="row"><label for="email_enabled" data-i18n="sendEmails">Send oracle emails</label><select id="email_enabled" name="enabled"><option value="1" data-i18n="yes">Yes</option><option value="0" data-i18n="no">No</option></select></div>
<details>
<summary data-i18n="advancedRelay">Advanced relay settings</summary>
<div class="row"><label for="relay_url" data-i18n="relayUrl">Relay URL</label><input id="relay_url" name="relay_url" inputmode="url" placeholder="https://relay.example.com/v1/bisc8/email"></div>
<div class="row"><label for="relay_token" data-i18n="relayToken">Relay token</label><input id="relay_token" name="relay_token" type="password" placeholder="Leave blank to keep current token" data-i18n-placeholder="keepToken"></div>
</details>
<div class="actions"><button class="btn" type="submit" data-i18n="saveEmail">Save email</button></div>
</form>
<p class="hint"><span data-i18n="recipientStatus">Recipient</span>: <span data-bind="email_recipient">missing</span>. <span data-i18n="relayStatus">Relay</span>: <span data-bind="email_relay">missing</span></p>
</article>
<article class="panel wide">
<h2 data-i18n="resetTitle">Reset</h2>
<div class="actions"><button class="btn warn" id="reset" type="button" data-i18n="resetButton">Full config reset</button><span class="hint" data-i18n="resetHint">This clears Wi-Fi, OpenAI, email, and language.</span></div>
<p class="hint" data-i18n="secretWarning">Secrets are stored on this device. Enable flash encryption before production use.</p>
</article>
</section>
<div class="toast" id="toast"></div>
</main>
<script>
const I18N={
en:{appTitle:'Bisc8 Setup',subtitle:'Local configuration for the small oracle.',mode:'Mode',address:'Address',network:'Network',language:'Language',wifiTitle:'Wi-Fi',scan:'Scan',scanState:'Saved networks stay on the device.',ssid:'SSID',password:'Password',saveWifi:'Save Wi-Fi',removeSlot:'Remove saved slot',remove:'Remove',languageTitle:'Language',displayLanguage:'Display language',saveLanguage:'Save language',firstBoot:'First boot starts in English.',openaiTitle:'OpenAI',apiKey:'API key',keepKey:'Leave blank to keep current key',stt:'Speech to text',oracleModel:'Oracle model',tts:'Text to speech',voice:'Voice',saveOpenai:'Save OpenAI',storedKey:'Stored key',emailTitle:'Email',recipient:'Recipient',sendEmails:'Send oracle emails',yes:'Yes',no:'No',advancedRelay:'Advanced relay settings',relayUrl:'Relay URL',relayToken:'Relay token',keepToken:'Leave blank to keep current token',saveEmail:'Save email',recipientStatus:'Recipient',relayStatus:'Relay',resetTitle:'Reset',resetButton:'Full config reset',resetHint:'This clears Wi-Fi, OpenAI, email, and language.',secretWarning:'Secrets are stored on this device. Enable flash encryption before production use.',missing:'missing',setupMode:'setup mode',onlineMode:'online',offlineMode:'offline',saved:'Saved',scanning:'Scanning...',networksFound:'networks found',resetConfirm:'Clear all local configuration?',resetDone:'Configuration cleared'},
es:{appTitle:'Configuración Bisc8',subtitle:'Configuración local del pequeño oráculo.',mode:'Modo',address:'Dirección',network:'Red',language:'Idioma',wifiTitle:'Wi-Fi',scan:'Escanear',scanState:'Las redes guardadas permanecen en el dispositivo.',ssid:'SSID',password:'Contraseña',saveWifi:'Guardar Wi-Fi',removeSlot:'Eliminar ranura guardada',remove:'Eliminar',languageTitle:'Idioma',displayLanguage:'Idioma de pantalla',saveLanguage:'Guardar idioma',firstBoot:'El primer arranque empieza en inglés.',openaiTitle:'OpenAI',apiKey:'Clave API',keepKey:'Déjalo vacío para conservar la clave actual',stt:'Voz a texto',oracleModel:'Modelo del oráculo',tts:'Texto a voz',voice:'Voz',saveOpenai:'Guardar OpenAI',storedKey:'Clave guardada',emailTitle:'Email',recipient:'Destinatario',sendEmails:'Enviar emails del oráculo',yes:'Sí',no:'No',advancedRelay:'Ajustes avanzados del relay',relayUrl:'URL del relay',relayToken:'Token del relay',keepToken:'Déjalo vacío para conservar el token actual',saveEmail:'Guardar email',recipientStatus:'Destinatario',relayStatus:'Relay',resetTitle:'Reset',resetButton:'Reset total',resetHint:'Borra Wi-Fi, OpenAI, email e idioma.',secretWarning:'Los secretos se guardan en este dispositivo. Activa flash encryption antes de producción.',missing:'falta',setupMode:'modo setup',onlineMode:'conectado',offlineMode:'sin red',saved:'Guardado',scanning:'Escaneando...',networksFound:'redes encontradas',resetConfirm:'¿Borrar toda la configuración local?',resetDone:'Configuración borrada'},
it:{appTitle:'Configurazione Bisc8',subtitle:'Configurazione locale del piccolo oracolo.',mode:'Modalità',address:'Indirizzo',network:'Rete',language:'Lingua',wifiTitle:'Wi-Fi',scan:'Scansiona',scanState:'Le reti salvate restano sul dispositivo.',ssid:'SSID',password:'Password',saveWifi:'Salva Wi-Fi',removeSlot:'Rimuovi slot salvato',remove:'Rimuovi',languageTitle:'Lingua',displayLanguage:'Lingua display',saveLanguage:'Salva lingua',firstBoot:'Il primo avvio parte in inglese.',openaiTitle:'OpenAI',apiKey:'Chiave API',keepKey:'Lascia vuoto per tenere la chiave attuale',stt:'Voce in testo',oracleModel:'Modello oracolo',tts:'Testo in voce',voice:'Voce',saveOpenai:'Salva OpenAI',storedKey:'Chiave salvata',emailTitle:'Email',recipient:'Destinatario',sendEmails:'Invia email oracolo',yes:'Sì',no:'No',advancedRelay:'Impostazioni relay avanzate',relayUrl:'URL relay',relayToken:'Token relay',keepToken:'Lascia vuoto per tenere il token attuale',saveEmail:'Salva email',recipientStatus:'Destinatario',relayStatus:'Relay',resetTitle:'Reset',resetButton:'Reset completo',resetHint:'Cancella Wi-Fi, OpenAI, email e lingua.',secretWarning:'I segreti sono salvati su questo dispositivo. Abilita la flash encryption prima della produzione.',missing:'manca',setupMode:'modalità setup',onlineMode:'connesso',offlineMode:'offline',saved:'Salvato',scanning:'Scansiono...',networksFound:'reti trovate',resetConfirm:'Cancellare tutta la configurazione locale?',resetDone:'Configurazione cancellata'}
};
let currentLanguage='en';
const toast=document.getElementById('toast');
function tr(key){return (I18N[currentLanguage]&&I18N[currentLanguage][key])||I18N.en[key]||key}
function note(keyOrText){toast.textContent=tr(keyOrText)||keyOrText;toast.classList.add('show');setTimeout(()=>toast.classList.remove('show'),2800)}
async function api(url,opts){const r=await fetch(url,opts);const j=await r.json();if(!r.ok)throw new Error(j.error||'Request failed');return j}
function applyLanguage(language){currentLanguage=I18N[language]?language:'en';document.documentElement.lang=currentLanguage;document.querySelectorAll('[data-i18n]').forEach(el=>{el.textContent=tr(el.dataset.i18n)});document.querySelectorAll('[data-i18n-placeholder]').forEach(el=>{el.placeholder=tr(el.dataset.i18nPlaceholder)})}
function valueText(key,value){if(!value||value==='missing')return tr('missing');if(value==='setup mode')return tr('setupMode');if(value==='online')return tr('onlineMode');if(value==='offline')return tr('offlineMode');return value}
function fill(s){if(s.language)applyLanguage(s.language);for(const k in s){document.querySelectorAll('[data-bind="'+k+'"]').forEach(el=>{el.textContent=valueText(k,s[k])})}if(s.language)document.getElementById('language').value=s.language}
async function refresh(){try{fill(await api('/api/status'))}catch(e){note(e.message)}}
function body(form){return new URLSearchParams(new FormData(form)).toString()}
document.querySelectorAll('form[data-api]').forEach(form=>form.addEventListener('submit',async e=>{e.preventDefault();const b=form.querySelector('button');b.disabled=true;try{fill(await api(form.dataset.api,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body(form)}));note('saved')}catch(err){note(err.message)}finally{b.disabled=false}}));
document.getElementById('scan').addEventListener('click',async()=>{const state=document.getElementById('scanState');const list=document.getElementById('scanList');state.textContent=tr('scanning');list.textContent='';try{const j=await api('/api/wifi/scan');state.textContent=j.networks.length+' '+tr('networksFound');j.networks.forEach(n=>{const btn=document.createElement('button');btn.type='button';btn.textContent=n.ssid+' · '+n.band+' · RSSI '+n.rssi;btn.onclick=()=>{document.getElementById('ssid').value=n.ssid};list.appendChild(btn)})}catch(e){state.textContent=e.message}});
document.getElementById('reset').addEventListener('click',async()=>{if(!confirm(tr('resetConfirm')))return;try{fill(await api('/api/reset',{method:'POST'}));note('resetDone')}catch(e){note(e.message)}});
applyLanguage('en');
refresh();
</script>
</body>
</html>)HTML";

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
        } else if (ch != '\r') {
            out.push_back(ch);
        }
    }
    out.push_back('"');
    return out;
}

const char *WifiBandLabel(uint8_t channel) {
    if (channel >= 1 && channel <= 14) {
        return "2.4 GHz";
    }
    if (channel >= 36) {
        return "5 GHz";
    }
    return "unknown";
}

int HexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + ch - 'a';
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + ch - 'A';
    }
    return -1;
}

std::string UrlDecode(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            out.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = HexValue(value[i + 1]);
            const int lo = HexValue(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            }
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

FormFields ParseForm(const std::string &body) {
    FormFields fields;
    size_t start = 0;
    while (start <= body.size()) {
        const size_t end = body.find('&', start);
        const std::string part = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const size_t eq = part.find('=');
        if (eq != std::string::npos) {
            fields.emplace_back(UrlDecode(part.substr(0, eq)), UrlDecode(part.substr(eq + 1)));
        } else if (!part.empty()) {
            fields.emplace_back(UrlDecode(part), "");
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return fields;
}

std::string FormValue(const FormFields &fields, const char *key, const std::string &fallback = "") {
    for (const auto &field : fields) {
        if (field.first == key) {
            return field.second;
        }
    }
    return fallback;
}

esp_err_t ReadRequestBody(httpd_req_t *req, std::string *body) {
    if (req == nullptr || body == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (req->content_len > kMaxFormBodyBytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    body->clear();
    body->resize(req->content_len);
    size_t received = 0;
    while (received < req->content_len) {
        const int ret = httpd_req_recv(req, &(*body)[received], req->content_len - received);
        if (ret <= 0) {
            return ESP_FAIL;
        }
        received += static_cast<size_t>(ret);
    }
    return ESP_OK;
}

esp_err_t SendJson(httpd_req_t *req, const std::string &json) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.size());
}

esp_err_t SendError(httpd_req_t *req, const char *status, const char *message) {
    httpd_resp_set_status(req, status);
    return SendJson(req, std::string("{\"error\":") + JsonString(message) + "}");
}

void ApplyDefaultSettings(DeviceSettings *settings) {
    if (settings == nullptr) {
        return;
    }
    *settings = DeviceSettings{};
    settings->language = "en";
    settings->openai = DefaultOpenAiSettings();
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

void WebPortal::BindStatus(const WifiStatus *wifi) {
    wifi_ = wifi;
}

void WebPortal::BindConfig(ConfigStore *config_store, DeviceSettings *settings) {
    config_store_ = config_store;
    settings_ = settings;
}

esp_err_t WebPortal::RegisterRoute(const char *uri, httpd_method_t method, HttpHandler handler) {
    httpd_uri_t route = {};
    route.uri = uri;
    route.method = method;
    route.handler = handler;
    route.user_ctx = this;
    return httpd_register_uri_handler(server_, &route);
}

esp_err_t WebPortal::HandleIndex(httpd_req_t *req) {
    DebugSerial::Log("[WEB]", "GET / free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebPortal::HandleStatus(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return SendError(req, "500 Internal Server Error", "Portal is not ready");
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleWifiScan(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return SendError(req, "500 Internal Server Error", "Portal is not ready");
    }
    return portal->SendWifiScanJson(req);
}

esp_err_t WebPortal::HandleWifiCredentials(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }

    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    const std::string action = FormValue(form, "action");

    if (action == "remove") {
        const size_t index = static_cast<size_t>(std::strtoul(FormValue(form, "index", "999").c_str(), nullptr, 10));
        if (index >= portal->settings_->wifi_count) {
            return SendError(req, "400 Bad Request", "Saved Wi-Fi slot not found");
        }
        for (size_t i = index; i + 1 < portal->settings_->wifi_count; ++i) {
            portal->settings_->wifi[i] = portal->settings_->wifi[i + 1];
        }
        portal->settings_->wifi[portal->settings_->wifi_count - 1] = WifiCredential{};
        portal->settings_->wifi_count--;
    } else {
        const std::string ssid = FormValue(form, "ssid");
        const std::string password = FormValue(form, "password");
        if (ssid.empty()) {
            return SendError(req, "400 Bad Request", "SSID is required");
        }
        bool updated = false;
        for (size_t i = 0; i < portal->settings_->wifi_count; ++i) {
            if (portal->settings_->wifi[i].ssid == ssid) {
                portal->settings_->wifi[i].password = password;
                updated = true;
                break;
            }
        }
        if (!updated) {
            if (portal->settings_->wifi_count >= kMaxWifiCredentials) {
                return SendError(req, "400 Bad Request", "Wi-Fi list is full");
            }
            portal->settings_->wifi[portal->settings_->wifi_count++] = WifiCredential{ssid, password};
        }
    }

    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleSettings(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const std::string language = FormValue(ParseForm(body), "language", "en");
    if (language != "en" && language != "es" && language != "it") {
        return SendError(req, "400 Bad Request", "Unsupported language");
    }
    portal->settings_->language = language;
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleOpenAi(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    const std::string api_key = FormValue(form, "api_key");
    if (!api_key.empty()) {
        portal->settings_->openai.api_key = api_key;
    }
    const std::string transcription_model = FormValue(form, "transcription_model");
    const std::string response_model = FormValue(form, "response_model");
    const std::string speech_model = FormValue(form, "speech_model");
    const std::string voice = FormValue(form, "voice");
    if (!transcription_model.empty()) {
        portal->settings_->openai.transcription_model = transcription_model;
    }
    if (!response_model.empty()) {
        portal->settings_->openai.response_model = response_model;
    }
    if (!speech_model.empty()) {
        portal->settings_->openai.speech_model = speech_model;
    }
    if (!voice.empty()) {
        portal->settings_->openai.voice = voice;
    }
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleEmail(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    portal->settings_->email.enabled = FormValue(form, "enabled", "0") == "1";
    portal->settings_->email.recipient = FormValue(form, "recipient");
    const std::string relay_url = FormValue(form, "relay_url");
    if (!relay_url.empty()) {
        portal->settings_->email.relay_url = relay_url;
    }
    const std::string relay_token = FormValue(form, "relay_token");
    if (!relay_token.empty()) {
        portal->settings_->email.relay_token = relay_token;
    }
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleReset(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->config_store_ == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    const esp_err_t err = portal->config_store_->Reset();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    ApplyDefaultSettings(portal->settings_);
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleCaptiveRedirect(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, nullptr, 0);
}

esp_err_t WebPortal::SendStatusJson(httpd_req_t *req) const {
    DebugSerial::Log("[WEB]", "GET /api/status free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    std::string setup_ssid;
    std::string setup_url = "http://192.168.4.1";
    std::string connected_ssid;
    std::string connected_ip;
    std::string ssid_attempt;
    bool online = false;
    bool setup_active = running_;

    if (wifi_ != nullptr) {
        setup_ssid = wifi_->setup_ssid;
        setup_url = wifi_->setup_url.empty() ? setup_url : wifi_->setup_url;
        connected_ssid = wifi_->connected_ssid;
        connected_ip = wifi_->connected_ip;
        ssid_attempt = wifi_->ssid_attempt;
        online = wifi_->online;
        setup_active = wifi_->setup_active || running_;
    }

    std::string language = "en";
    std::string openai_key;
    std::string email_recipient;
    std::string email_relay;
    bool email_enabled = false;
    size_t wifi_count = 0;
    if (settings_ != nullptr) {
        language = settings_->language;
        openai_key = settings_->openai.api_key.empty() ? "missing" : MaskSecret(settings_->openai.api_key);
        email_recipient = settings_->email.recipient.empty() ? "missing" : MaskSecret(settings_->email.recipient);
        email_relay = settings_->email.relay_url.empty() ? "missing" : "configured";
        email_enabled = settings_->email.enabled;
        wifi_count = settings_->wifi_count;
    }

    const std::string wifi_mode = online ? "online" : (setup_active ? "setup mode" : "offline");
    const std::string device_address = online && !connected_ip.empty() ? connected_ip : setup_url;
    const std::string network_label = online ? connected_ssid : wifi_mode;

    std::string json = "{";
    json += "\"online\":" + std::string(online ? "true" : "false");
    json += ",\"setup_active\":" + std::string(setup_active ? "true" : "false");
    json += ",\"setup_ssid\":" + JsonString(setup_ssid);
    json += ",\"setup_url\":" + JsonString(setup_url);
    json += ",\"connected_ip\":" + JsonString(connected_ip.empty() ? "missing" : connected_ip);
    json += ",\"device_address\":" + JsonString(device_address);
    json += ",\"wifi_mode\":" + JsonString(wifi_mode);
    json += ",\"network_label\":" + JsonString(network_label.empty() ? wifi_mode : network_label);
    json += ",\"connected_ssid\":" + JsonString(connected_ssid.empty() ? "setup mode" : connected_ssid);
    json += ",\"ssid_attempt\":" + JsonString(ssid_attempt);
    json += ",\"language\":" + JsonString(language);
    json += ",\"wifi_count\":" + std::to_string(wifi_count);
    json += ",\"openai_key\":" + JsonString(openai_key);
    json += ",\"email_enabled\":" + std::string(email_enabled ? "true" : "false");
    json += ",\"email_recipient\":" + JsonString(email_recipient);
    json += ",\"email_relay\":" + JsonString(email_relay);
    json += ",\"warning\":\"secrets are stored on this device\"";
    json += "}";
    return SendJson(req, json);
}

esp_err_t WebPortal::SendWifiScanJson(httpd_req_t *req) const {
    DebugSerial::LogAlways("[WEB]", "GET /api/wifi/scan start free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    ap_count = std::min(ap_count, kMaxScanRecords);
    wifi_ap_record_t records[kMaxScanRecords] = {};
    err = esp_wifi_scan_get_ap_records(&ap_count, records);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }

    std::string json = "{\"networks\":[";
    for (uint16_t i = 0; i < ap_count; ++i) {
        if (i > 0) {
            json += ",";
        }
        json += "{\"ssid\":";
        json += JsonString(reinterpret_cast<const char *>(records[i].ssid));
        json += ",\"rssi\":";
        json += std::to_string(records[i].rssi);
        json += ",\"channel\":";
        json += std::to_string(records[i].primary);
        json += ",\"band\":";
        json += JsonString(WifiBandLabel(records[i].primary));
        json += "}";
    }
    json += "]}";
    DebugSerial::LogAlways("[WEB]",
                           "GET /api/wifi/scan done count=%u free_heap=%u",
                           static_cast<unsigned>(ap_count),
                           static_cast<unsigned>(esp_get_free_heap_size()));
    return SendJson(req, json);
}

esp_err_t WebPortal::SaveCurrentSettings() {
    if (config_store_ == nullptr || settings_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return config_store_->Save(*settings_);
}

esp_err_t WebPortal::Start() {
    if (running_) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;
    config.max_open_sockets = 3;
    config.backlog_conn = 2;
    config.recv_wait_timeout = 8;
    config.send_wait_timeout = 8;
    config.max_uri_handlers = sizeof(kPortalRoutes) / sizeof(kPortalRoutes[0]) +
                              sizeof(kCaptiveProbePaths) / sizeof(kCaptiveProbePaths[0]);

    esp_err_t err = httpd_start(&server_, &config);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WEB]", "setup portal failed to start: %s", esp_err_to_name(err));
        return err;
    }

    err = RegisterRoute("/", HTTP_GET, &WebPortal::HandleIndex);
    if (err == ESP_OK) {
        err = RegisterRoute("/api/status", HTTP_GET, &WebPortal::HandleStatus);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/wifi/scan", HTTP_GET, &WebPortal::HandleWifiScan);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/wifi/credentials", HTTP_POST, &WebPortal::HandleWifiCredentials);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/settings", HTTP_POST, &WebPortal::HandleSettings);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/openai", HTTP_POST, &WebPortal::HandleOpenAi);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/email", HTTP_POST, &WebPortal::HandleEmail);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/smtp", HTTP_POST, &WebPortal::HandleEmail);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/reset", HTTP_POST, &WebPortal::HandleReset);
    }
    for (const char *probe : kCaptiveProbePaths) {
        if (err != ESP_OK) {
            break;
        }
        err = RegisterRoute(probe, HTTP_GET, &WebPortal::HandleCaptiveRedirect);
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
