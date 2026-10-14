#ifndef HOR_DEVICE_H
#define HOR_DEVICE_H

#include <chrono>

#include "dispatcher.h"
#include "messenger.h"
#include "requester.h"

enum class device_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    READ_ERROR,
    WRITE_ERROR,
    NETWORK_ERROR
};

class device {
public:
    [[nodiscard]] device_status initialize(void) noexcept;
    void update(void) noexcept;
    void terminate(void) noexcept;
private:
    dispatcher dispatcher;
    messenger messenger;
    requester requester;

    u8 running;

    i32 last_commit_id;
    std::chrono::steady_clock::time_point last_commits_check;

    [[nodiscard]] device_status _load_state(void) noexcept;
    [[nodiscard]] device_status _save_state(void) noexcept;
    [[nodiscard]] device_status _check_commits(void) noexcept;
};

#endif // !HOR_DEVICE_H