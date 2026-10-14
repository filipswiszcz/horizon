#ifndef HOR_DEVICE_H
#define HOR_DEVICE_H

#include "dispatcher.h"
#include "messenger.h"
#include "requester.h"

enum class device_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    NETWORK_ERROR
};

class device {
public:
    [[nodiscard]] device_status initialize(void);
    void update(void);
    void terminate(void);
private:
    dispatcher dispatcher;
    messenger messenger;
    requester requester;

    // const char *url = "https://discord.com/api/webhooks/1557495551623303219/fXp3HgGgln4RtXZCnMBzNu1JJg9kp07UmpfKWpQJKkkpclxv3I4aPML9YtkOGD0DoVMO";
    // const char *msg = R"({"content":"ignore alien orders"})";

    u8 running;
};

#endif // !HOR_DEVICE_H