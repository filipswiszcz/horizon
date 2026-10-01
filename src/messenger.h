#ifndef HOR_MESSENGER_H
#define HOR_MESSENGER_H

#include "common.h"

class message {} // or messages (dod)?

enum class messenger_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    MSG_NOT_SENT
};

class messenger {
public:
    [[nodiscard]] messenger_status initialize(void); // needs access to requester or tasks dispatcher
    [[nodiscard]] messenger_status send(const u32 channel_id); // more args
};

#endif //!HOR_MESSENGER_H