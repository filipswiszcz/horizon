#ifndef HOR_DEVICE_H
#define HOR_DEVICE_H

#include <chrono>

#include "dispatcher.h"
#include "gateway.h"
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
    gateway gateway;
    messenger messenger;
    requester requester;

    struct {
        char names[DISCORD_MAX_CHANNELS][101];
        u64 ids[DISCORD_MAX_CHANNELS];
        u32 counter;
    } channels;

    u8 running;

    i32 last_commit_id;
    std::chrono::steady_clock::time_point last_commits_check;

    [[nodiscard]] u64 _get_channel_id(const char *name) noexcept;

    [[nodiscard]] device_status _load_state(void) noexcept;
    [[nodiscard]] device_status _save_state(void) noexcept;

    [[nodiscard]] device_status _keep_alive(void) noexcept;

    [[nodiscard]] device_status _check_commits(const char *depot, const char *channel_name) noexcept;
};

#endif // !HOR_DEVICE_H