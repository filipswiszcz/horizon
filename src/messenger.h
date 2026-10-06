#ifndef HOR_MESSENGER_H
#define HOR_MESSENGER_H

#include "common.h"
#include "dispatcher.h"
#include "memory.h"

struct message {
    u64 channel_id;
    const char *content;
    u32 cont_length;
    // embed?? (for url?)
};

// class message {}; // or messages (dod)?

enum class messenger_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    MSG_NOT_SENT,
    DISPATCH_ERROR,
    OUT_OF_MEMORY
};

class messenger {
public:
    [[nodiscard]] messenger_status initialize(dispatcher *task_dispatcher) noexcept; // needs access to requester or tasks dispatcher
    [[nodiscard]] messenger_status send(const u64 channel_id, const char *text) noexcept; // more args
private:
    dispatcher *task_dispatcher;
    lf_pool_allocator<message, 16> msg_pool;

    static void _worker_execute_send(const void *payload) noexcept;
};

#endif //!HOR_MESSENGER_H