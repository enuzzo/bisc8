#pragma once

#include <stddef.h>

#include "localization.h"

namespace bisc8 {

struct FortunePick {
    const char *text;
    size_t index;
    size_t count;
};

class FortuneService {
public:
    FortunePick PickRandom(Language language);
    size_t Count(Language language) const;

private:
    int last_index_ = -1;
};

}  // namespace bisc8
