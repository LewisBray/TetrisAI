#ifndef UTIL_H
#define UTIL_H

#include "types.h"

static u32 random_number(u32 seed);

static u32 copy_bytes(const i8* source, u32 count, i8* destination);
static u32 compare_bytes(const i8* lhs, const i8* rhs, u32 count);

#endif
