#ifndef HOR_MESSENGER_H
#define HOR_MESSENGER_H

#include "common.h"
#include "dispatcher.h"
#include "memory.h"
#include "requester.h"

struct message {
    char url[128];
    char data[2048];
    u32 data_size;
};

enum class messenger_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    MSG_NOT_SENT,
    DISPATCH_ERROR
};

class messenger {
public:
    [[nodiscard]] messenger_status initialize(dispatcher *dispatcher, requester *requester) noexcept; // needs access to requester or tasks dispatcher
    [[nodiscard]] messenger_status send(const u64 channel_id, const char *content) noexcept; // more args
private:
    dispatcher *dispatcher;
    requester *requester;

    static void _worker_execute_send(const u8 *data, void *context) noexcept;
};

#endif //!HOR_MESSENGER_H