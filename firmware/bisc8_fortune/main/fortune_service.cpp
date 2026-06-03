#include "fortune_service.h"

#include <esp_random.h>

#include "debug_serial.h"
#include "generated/fortune_data.h"

namespace bisc8 {

FortunePick FortuneService::PickRandom() {
    if (kFortuneCount == 0) {
        return {"Nessuna risposta caricata.", 0, 0};
    }

    size_t next = 0;
    if (kFortuneCount == 1) {
        next = 0;
    } else {
        do {
            next = esp_random() % kFortuneCount;
        } while (static_cast<int>(next) == last_index_);
    }
    last_index_ = static_cast<int>(next);
    DebugSerial::Log("[FORTUNE]", "selected index=%u count=%u", static_cast<unsigned>(next), static_cast<unsigned>(kFortuneCount));
    return {kFortunes[next], next, kFortuneCount};
}

size_t FortuneService::Count() const {
    return kFortuneCount;
}

}  // namespace bisc8
