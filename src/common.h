#ifndef HOR_COMMON_H
#define HOR_COMMON_H

#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>

#define DISCORD_API_URL "https://discord.com/api/v10/"
#define DISCORD_TOKEN ""

typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static_assert(sizeof(f32) == 4, "Float must be exactly 4 bytes");
static_assert(sizeof(f64) == 8, "Double must be exactly 8 bytes");

typedef u64 dcID;

// dictionary with channel_name -> channel_id relations?

#endif //!HOR_COMMON_H