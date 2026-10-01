#ifndef HOR_DEVICE_H
#define HOR_DEVICE_H

#include "common.h"

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
};

#endif // !HOR_DEVICE_H