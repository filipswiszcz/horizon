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
    [[nodiscard]] requester_status post(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
    [[nodiscard]] requester_status get(const char *url, const char *headers, char *response, u32 response_size) noexcept;
private:
#ifdef _WIN32
    [[nodiscard]] requester_status _post_win32(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
    [[nodiscard]] requester_status _get_win32(const char *url, const char *headers, char *response, u32 response_size) noexcept;
#else
    [[nodiscard]] requester_status _post_linux(const char *url, const char *headers, const char *data, const u32 data_size) noexcept;
    [[nodiscard]] requester_status _get_linux(const char *url, const char *headers, char *response, u32 response_size) noexcept;
#endif
};

#endif // !HOR_REQUESTER_H