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
    [[nodiscard]] requester_status send(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
private:
#ifdef _WIN32
    [[nodiscard]] requester_status _send_win32(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
#else
    [[nodiscard]] requester_status _send_linux(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
#endif
};

#endif // !HOR_REQUESTER_H