#ifndef HOR_DEVICE_H
#define HOR_DEVICE_H

#include "dispatcher.h"
#include "messenger.h"

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
    dispatcher task_dispatcher;
    messenger messenger;

    u8 running;
};

#endif // !HOR_DEVICE_H