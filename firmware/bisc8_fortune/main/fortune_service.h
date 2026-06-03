#pragma once

#include <stddef.h>

namespace bisc8 {

struct FortunePick {
    const char *text;
    size_t index;
    size_t count;
};

class FortuneService {
public:
    FortunePick PickRandom();
    size_t Count() const;

private:
    int last_index_ = -1;
};

}  // namespace bisc8
