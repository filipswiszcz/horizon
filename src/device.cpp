#include "device.h"

constexpr const char *DEVICE_STATE_FILE = "device.dat";
constexpr u64 TEMP_CHANGELOG_CHANNEL_ID = 1;

device_status device::initialize(void) noexcept {
    dispatcher_status disp_init_status = this->dispatcher.initialize();
    if (disp_init_status != dispatcher_status::SUCCESS) {
        return device_status::INIT_ERROR;
    }

    messenger_status mess_init_status = this->messenger.initialize(&this->dispatcher, &this->requester);
    if (mess_init_status != messenger_status::SUCCESS) {
        // std::cout << mess_init_status << std::endl;
        this->dispatcher.terminate();
        return device_status::INIT_ERROR;
    }

    this->running = 1;
    this->last_commit_id = 0;

    device_status dev_load_status = this->_load_state();
    if (dev_load_status != device_status::SUCCESS) {
        this->dispatcher.terminate();
        return device_status::INIT_ERROR;
    }

    this->last_commits_check = std::chrono::steady_clock::now();

    return device_status::SUCCESS;
}

void device::update(void) noexcept {
    while (this->running) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // const requester_status req_send_status = requester.send(this->url, this->msg, std::strlen(this->msg));
        // std::cout << static_cast<u32>(req_send_status) << std::endl;

        // handle responses from the server (eg /status)
    }
}

void device::terminate(void) noexcept {
    this->running = 0;
    this->dispatcher.terminate();
}

device_status device::_load_state(void) noexcept {
    FILE *file = std::fopen(DEVICE_STATE_FILE, "w");
    if (file == nullptr) {
        return device_status::READ_ERROR;
    }
    std::fscanf(file, "last_p4_commit_id=%d", this->last_commit_id);
    std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_save_state(void) noexcept {
    FILE *file = std::fopen(DEVICE_STATE_FILE, "r");
    if (file == nullptr) {
        return device_status::WRITE_ERROR;
    }
    std::fprintf(file, "last_p4_commit_id=%d", this->last_commit_id);
    std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_check_commits(void) noexcept {
    return device_status::SUCCESS;
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