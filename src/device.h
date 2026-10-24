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

// get map of channels
// make device/messenger configurable/restartable
// p4 login inject
// read depots and changelog channels from device.dat

class device {
public:
    [[nodiscard]] device_status initialize(void) noexcept;
    void tick(void) noexcept;
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

    char token[128];

    u8 is_running;

    u8 is_gw_online;
    u8 is_mess_online;

    i32 last_commit_id;
    std::chrono::steady_clock::time_point last_commits_check;
    std::chrono::steady_clock::time_point last_gateway_check;

    [[nodiscard]] u64 _get_channel_id(const char *name) noexcept;

    [[nodiscard]] device_status _load_state(void) noexcept;
    [[nodiscard]] device_status _save_state(void) noexcept;

    [[nodiscard]] device_status _restore_gateway_conn(void) noexcept;
    [[nodiscard]] device_status _restart_messenger(void) noexcept;

    [[nodiscard]] device_status _check_commits(const char *depot_name, const char *channel_name) noexcept;
};

#endif // !HOR_DEVICE_H