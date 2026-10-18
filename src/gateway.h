#ifndef HOR_GATEWAY_H
#define HOR_GATEWAY_H

#include <chrono>

#include "common.h"

enum class gateway_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    CONNECT_ERROR,
    SEND_ERROR,
};

class gateway {
public:
    [[nodiscard]] gateway_status initialize(const char *token) noexcept;
    void update(void) noexcept;
    void terminate(void) noexcept;
private:
    char token[128];

    u32 heartbeat;
    std::chrono::steady_clock::time_point last_heartbeat;

    char rec_buff[4096];

    [[nodiscard]] gateway_status send_identify(void) noexcept;
    [[nodiscard]] gateway_status send_heartbeat(void) noexcept;

    void* session;
    void* connection;
    void* request;
    void* websocket;

#ifdef _WIN32
    [[nodiscard]] gateway_status _connect_win32(void) noexcept;
    [[nodiscard]] gateway_status _send_frame_data_win32(const char *data) noexcept;
#else
    [[nodiscard]] gateway_status _connect_linux(void) noexcept;
    [[nodiscard]] gateway_status _send_frame_data_linux(const char *data) noexcept;
#endif
};

#endif // !HOR_GATEWAY_H