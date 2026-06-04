#include "localization.h"

#include <string.h>

namespace bisc8 {

namespace {

constexpr LocalizedStrings kEnglish = {
    "en",
    "oracle loading",
    "answers\nin the grimoire",
    "Hold BOOT to ask\nPWR power",
    "Wi-Fi",
    "Setup Wi-Fi",
    "Join Bisc8-XXXX\nOpen 192.168.4.1",
    "Captive portal ready",
    "Listening",
    "Speak now.\nRelease BOOT to send.",
    "Thinking",
    "The oracle is reading your question.",
    "Cooking",
    "Keep the question warm.",
    "Speaking",
    "The answer is arriving.",
    "Offline",
    "No known Wi-Fi.\nSetup portal is open.",
    "Press PWR\nto turn me on",
    "Error",
};

constexpr LocalizedStrings kSpanish = {
    "es",
    "oráculo cargando",
    "respuestas\nen el grimorio",
    "Mantén BOOT para preguntar\nPWR energía",
    "Wi-Fi",
    "Configura Wi-Fi",
    "Únete a Bisc8-XXXX\nAbre 192.168.4.1",
    "Portal cautivo listo",
    "Escuchando",
    "Habla ahora.\nSuelta BOOT para enviar.",
    "Pensando",
    "El oráculo lee tu pregunta.",
    "Cooking",
    "La pregunta está al fuego.",
    "Hablando",
    "La respuesta está llegando.",
    "Sin red",
    "No hay Wi-Fi conocida.\nPortal abierto.",
    "Pulsa PWR\npara encenderme",
    "Error",
};

constexpr LocalizedStrings kItalian = {
    "it",
    "oracolo in carica",
    "risposte\nnel grimorio",
    "Tieni BOOT per chiedere\nPWR energia",
    "Wi-Fi",
    "Configura Wi-Fi",
    "Entra in Bisc8-XXXX\nApri 192.168.4.1",
    "Portale captive pronto",
    "Ascolto",
    "Parla ora.\nRilascia BOOT per inviare.",
    "Penso",
    "L'oracolo legge la domanda.",
    "Cooking",
    "La domanda è sul fuoco.",
    "Parlo",
    "La risposta sta arrivando.",
    "Offline",
    "Nessuna Wi-Fi nota.\nPortale aperto.",
    "Premi PWR\nper accendermi",
    "Errore",
};

}  // namespace

const char *DefaultLanguage() {
    return "en";
}

Language ParseLanguage(const char *code) {
    if (code != nullptr && strcmp(code, "es") == 0) {
        return Language::Spanish;
    }
    if (code != nullptr && strcmp(code, "it") == 0) {
        return Language::Italian;
    }
    return Language::English;
}

const LocalizedStrings &StringsFor(Language language) {
    switch (language) {
        case Language::Spanish:
            return kSpanish;
        case Language::Italian:
            return kItalian;
        case Language::English:
        default:
            return kEnglish;
    }
}

const char *LocalizedValue(Language language, const char *key) {
    const LocalizedStrings &strings = StringsFor(language);
    if (key == nullptr) {
        return "";
    }
    if (strcmp(key, "boot_status") == 0) {
        return strings.boot_status;
    }
    if (strcmp(key, "wifi_setup_title") == 0) {
        return strings.wifi_setup_title;
    }
    if (strcmp(key, "wifi_setup_body") == 0) {
        return strings.wifi_setup_body;
    }
    if (strcmp(key, "listening_title") == 0) {
        return strings.listening_title;
    }
    if (strcmp(key, "thinking_title") == 0) {
        return strings.thinking_title;
    }
    if (strcmp(key, "cooking_title") == 0) {
        return strings.cooking_title;
    }
    if (strcmp(key, "cooking_body") == 0) {
        return strings.cooking_body;
    }
    if (strcmp(key, "speaking_title") == 0) {
        return strings.speaking_title;
    }
    if (strcmp(key, "offline_title") == 0) {
        return strings.offline_title;
    }
    if (strcmp(key, "sleep_footer") == 0) {
        return strings.sleep_footer;
    }
    return "";
}

}  // namespace bisc8
