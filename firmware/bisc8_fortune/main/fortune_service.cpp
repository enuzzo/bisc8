#include "fortune_service.h"

#include <esp_random.h>

#include "debug_serial.h"
#include "generated/fortune_data.h"

namespace bisc8 {

namespace {

struct FortuneTable {
    const char *const *fortunes;
    size_t count;
    const char *language;
};

FortuneTable FortuneTableFor(Language language) {
    switch (language) {
        case Language::Spanish:
            return {kFortunes_es, kFortuneCount_es, kFortuneLanguage_es};
        case Language::Italian:
            return {kFortunes, kFortuneCount, kFortuneLanguage};
        case Language::English:
        default:
            return {kFortunes_en, kFortuneCount_en, kFortuneLanguage_en};
    }
}

}  // namespace

FortunePick FortuneService::PickRandom(Language language) {
    const FortuneTable table = FortuneTableFor(language);
    if (table.count == 0) {
        return {"No answer loaded.", 0, 0};
    }

    size_t next = 0;
    if (table.count == 1) {
        next = 0;
    } else {
        do {
            next = esp_random() % table.count;
        } while (static_cast<int>(next) == last_index_);
    }
    last_index_ = static_cast<int>(next);
    DebugSerial::Log("[FORTUNE]", "selected language=%s index=%u count=%u",
                     table.language,
                     static_cast<unsigned>(next),
                     static_cast<unsigned>(table.count));
    return {table.fortunes[next], next, table.count};
}

size_t FortuneService::Count(Language language) const {
    return FortuneTableFor(language).count;
}

}  // namespace bisc8
