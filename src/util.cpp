#include "util.h"

static u32 random_number(const u32 seed) {
    static constexpr u32 m = (1 << 16) + 1;
    static constexpr u32 a = 75;
    static constexpr u32 c = 74;

    return (a * seed + c) % m;
}
