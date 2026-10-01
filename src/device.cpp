#include "device.h"

device_status device::initialize(void) {

}

void device::update(void) {}

void device::terminate(void) {}

i32 main(void) {
    device device;
    device_status init_status = device.initialize();
    if (init_status != SUCCESS) {
        return 1;
    }
    device.update();
    device.terminate();
    return 0;
}