#include "device.h"

device_status device::initialize(void) {
    dispatcher_status disp_init_status = this->task_dispatcher.initialize();
    if (disp_init_status != dispatcher_status::SUCCESS) {
        return device_status::INIT_ERROR;
    }

    messenger_status mess_init_status = this->messenger.initialize(&this->task_dispatcher);
    if (mess_init_status != messenger_status::SUCCESS) {
        // std::cout << mess_init_status << std::endl;
        this->task_dispatcher.terminate();
        return device_status::INIT_ERROR;
    }

    this->running = 1;

    return device_status::SUCCESS;
}

void device::update(void) {
    while (this->running) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // handle responses from the server (eg /status)
    }
}

void device::terminate(void) {
    this->running = 0;
    this->task_dispatcher.terminate();
}

i32 main(void) {
    device device;
    device_status dev_init_status = device.initialize();
    if (dev_init_status != device_status::SUCCESS) {
        return 1;
    }
    device.update();
    device.terminate();
    return 0;
}