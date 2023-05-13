#include "util.h"

static u32 random_number(const u32 seed) {
    static constexpr u32 m = (1 << 16) + 1;
    static constexpr u32 a = 75;
    static constexpr u32 c = 74;

    return (a * seed + c) % m;
}

static u32 copy_bytes(const i8* source, u32 count, i8* destination) {
    const u32 bytes_written = count;
    while (count-- != 0) {
        *destination++ = *source++;
    }

    return bytes_written;
}

static u32 compare_bytes(const i8* lhs, const i8* rhs, u32 count) {
    u32 difference = 0;
    while (count-- != 0) {
        difference += static_cast<u32>(*lhs++ != *rhs++);
    }

    return difference;
}
