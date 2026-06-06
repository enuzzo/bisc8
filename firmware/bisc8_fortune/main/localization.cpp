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
    "connect to\n%s",
    "then open\n%s",
    "Listening",
    "Bisc8 is listening.\nMax 15s, no rambling!",
    "Thinking",
    "Reading the crumbs..",
    "Cooking",
    "Your question's in the oven.",
    "Speaking",
    "Fresh from the oven.",
    "Offline",
    "No known Wi-Fi.\nSetup portal is open.",
    "Press PWR\nto turn me on",
    "Error",
    "Consult",
    "Think your\nquestion",
    "hold to speak",
    "Status",
    "%s\n%s",
    "%s | %s",
    "No network\nSetup unavailable",
    "BOOT oracle",
    "Trying\n%s",
    "%d seconds",
    "15 seconds max",
    "voice answer",
    "Audio unavailable.",
    "Recording failed.",
    "Voice oracle is not configured yet.",
    "see serial",
    "No Wi-Fi.\nThe oracle\nmeditates offline.",
    "Low battery",
    "Nearly dead.\nEven the oracle\nnaps.",
    "Out of juice.\nPowering off\nto save the rest.",
    "Empty",
    "Zero answers loaded.\nAn oracle on\nan empty stomach.",
    "Resting",
    "Click to wake me up",
    "on your phone or pc",
    "couldn't reach\n%s",
    "press",
    "I didn't hear you.\nTry again.",
    "The oracle\nglitched.",
    "Connected",
    "Not connected",
};

constexpr LocalizedStrings kSpanish = {
    "es",
    "oráculo cargando",
    "respuestas\nen el grimorio",
    "Mantén BOOT para preguntar\nPWR energía",
    "Wi-Fi",
    "Configura Wi-Fi",
    "conéctate a\n%s",
    "luego abre\n%s",
    "Escuchando",
    "Bisc8 te escucha.\n¡Máx 15s, sin rollos!",
    "Pensando",
    "Leo las migas..",
    "Cocinando",
    "La pregunta está al fuego.",
    "Hablando",
    "Recién horneada.",
    "Sin red",
    "No hay Wi-Fi conocida.\nPortal abierto.",
    "Pulsa PWR\npara encenderme",
    "Error",
    "Consulta",
    "Piensa tu\npregunta",
    "mantén pulsado: habla",
    "Estado",
    "%s\n%s",
    "%s | %s",
    "Sin red\nPortal no listo",
    "BOOT oráculo",
    "Probando\n%s",
    "%d segundos",
    "15 segundos máx",
    "respuesta voz",
    "Audio no disponible.",
    "La grabación falló.",
    "El oráculo de voz aún no está configurado.",
    "ver serial",
    "Sin Wi-Fi.\nEl oráculo\nmedita offline.",
    "Batería baja",
    "Casi muerta.\nHasta el oráculo\nduerme.",
    "Sin batería.\nMe apago\npara ahorrar.",
    "Vacío",
    "Cero respuestas.\nVaya oráculo\nen ayunas.",
    "Reposo",
    "Pulsa para despertarme",
    "con el móvil o pc",
    "no alcanzo\n%s",
    "pulsa",
    "No te he oído.\nInténtalo otra vez.",
    "El oráculo\nse ha trabado.",
    "Conectado",
    "Sin conexión",
};

constexpr LocalizedStrings kItalian = {
    "it",
    "oracolo in carica",
    "risposte\nnel grimorio",
    "Tieni BOOT per chiedere\nPWR energia",
    "Wi-Fi",
    "Configura Wi-Fi",
    "collegati a\n%s",
    "poi apri\n%s",
    "Ascolto",
    "Bisc8 ti ascolta.\nMax 15s! Niente pipponi!",
    "Penso",
    "Consulto le briciole..",
    "Cucino",
    "La domanda è sul fuoco.",
    "Parlo",
    "Appena sfornata.",
    "Offline",
    "Nessuna Wi-Fi nota.\nPortale aperto.",
    "Premi PWR\nper accendermi",
    "Errore",
    "Interroga",
    "Pensa al tuo\nquesito",
    "tieni premuto: parla",
    "Stato",
    "%s\n%s",
    "%s | %s",
    "Senza rete\nPortale non pronto",
    "BOOT oracolo",
    "Provo\n%s",
    "%d secondi",
    "15 secondi max",
    "risposta vocale",
    "Audio non disponibile.",
    "Registrazione fallita.",
    "L'oracolo vocale non è ancora configurato.",
    "vedi seriale",
    "Niente Wi-Fi.\nL'oracolo\nmedita offline.",
    "Batteria bassa",
    "Quasi spenta.\nAnche l'oracolo\nha sonno.",
    "Batteria scarica.\nMi spengo\nper risparmiare.",
    "Vuoto",
    "Zero responsi caricati.\nChe oracolo\na digiuno.",
    "Riposo",
    "Premi per svegliarmi",
    "col telefono o pc",
    "non raggiungo\n%s",
    "premi",
    "Non ho sentito nulla.\nRiprova.",
    "L'oracolo si è\nincantato.",
    "Connesso",
    "Non connesso",
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
    if (strcmp(key, "intro_title") == 0) {
        return strings.intro_title;
    }
    if (strcmp(key, "intro_body") == 0) {
        return strings.intro_body;
    }
    if (strcmp(key, "intro_footer") == 0) {
        return strings.intro_footer;
    }
    if (strcmp(key, "status_title") == 0) {
        return strings.status_title;
    }
    if (strcmp(key, "status_online_body") == 0) {
        return strings.status_online_body;
    }
    if (strcmp(key, "status_setup_body") == 0) {
        return strings.status_setup_body;
    }
    if (strcmp(key, "status_footer") == 0) {
        return strings.status_footer;
    }
    if (strcmp(key, "voice_footer") == 0) {
        return strings.voice_footer;
    }
    if (strcmp(key, "recording_failed_body") == 0) {
        return strings.recording_failed_body;
    }
    if (strcmp(key, "voice_oracle_unconfigured_body") == 0) {
        return strings.voice_oracle_unconfigured_body;
    }
    if (strcmp(key, "voice_no_speech_body") == 0) {
        return strings.voice_no_speech_body;
    }
    if (strcmp(key, "voice_error_generic_body") == 0) {
        return strings.voice_error_generic_body;
    }
    if (strcmp(key, "error_footer") == 0) {
        return strings.error_footer;
    }
    return "";
}

}  // namespace bisc8
