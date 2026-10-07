#ifndef HOR_REQUESTER_H
#define HOR_REQUESTER_H

#include "common.h"

enum class requester_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    HTTP_ERROR,
    INVALID_ARG
};

class requester {
public:
    [[nodiscard]] requester_status send(const char *url, const char *data, const u32 data_size) noexcept;
private:
    [[nodiscard]] requester_status handle_win32(const char *url, const char *data, const u32 data_size) noexcept;
    [[nodiscard]] requester_status handle_linux(const char *url, const char *data, const u32 data_size) noexcept;
};

#endif // !HOR_REQUESTER_H