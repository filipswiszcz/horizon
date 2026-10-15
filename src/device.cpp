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

        auto clock_now = std::chrono::steady_clock::now();
        auto clock_elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock_now - this->last_commits_check).count();

        if (clock_elapsed >= 10) {
            this->_check_commits();
            this->last_commits_check = std::chrono::steady_clock::now();
        }

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
    FILE *file = std::fopen(DEVICE_STATE_FILE, "r");
    if (file == nullptr) {
        return device_status::READ_ERROR;
    }
    std::fscanf(file, "last_commit_id=%d", &this->last_commit_id);
    std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_save_state(void) noexcept {
    FILE *file = std::fopen(DEVICE_STATE_FILE, "w");
    if (file == nullptr) {
        return device_status::WRITE_ERROR;
    }
    std::fprintf(file, "last_commit_id=%d", this->last_commit_id);
    std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_check_commits(void) noexcept {
    FILE *file;
#ifdef _WIN32
    file = _popen("p4 changes -m 1 -s submitted", "r");
#else
    file = popen("p4 changes -m 1 -s submitted", "r");
#endif    
    if (file == nullptr) {
        return device_status::READ_ERROR;
    }

    char cmd_output[128] = {0};
    if (std::fgets(cmd_output, sizeof(cmd_output), file) == nullptr) {
#ifdef _WIN32
        _pclose(file);
#else
        pclose(file);
#endif
        return device_status::READ_ERROR;
    }

    i32 curr_commit_id = 0;
    if (std::sscanf(cmd_output, "Change %d on", &curr_commit_id) != 1) {
#ifdef _WIN32
        _pclose(file);
#else
        pclose(file);
#endif
        return device_status::READ_ERROR;
    }
    if (curr_commit_id <= this->last_commit_id) {
#ifdef _WIN32
        _pclose(file);
#else
        pclose(file);
#endif
        return device_status::READ_ERROR;
    }

#ifdef _WIN32
        _pclose(file);
#else
        pclose(file);
#endif

    char cmd_desc_output[128];
    std::snprintf(cmd_desc_output, sizeof(cmd_desc_output), "p4 -Ztag -F \"%%desc%%\" describe -s %d", curr_commit_id);
    
    FILE *process;
#ifdef _WIN32
        process = _popen(cmd_desc_output, "r");
#else
        process = popen(cmd_desc_output, "r");
#endif
    if (process == nullptr) {
        return device_status::READ_ERROR;
    }

    char raw_message[1024] = {0}, line[256];
    u32 pos = 0;
    while (std::fgets(line, sizeof(line), process) != nullptr) {
        size_t line_len = std::strlen(line);
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
            line[line_len - 1] = '\0';
            line_len--;
        }

        if (line_len == 0) {
            continue;
        }

        char *line_start = line;
        while (*line_start == ' ' || *line_start == '\t') {
            line_start++;
        }

        if (*line_start != '-') {
            pos += std::snprintf(raw_message + pos, sizeof(raw_message) - pos, "- %s\\n", line_start);
        } else {
            pos += std::snprintf(raw_message + pos, sizeof(raw_message) - pos, "%s\\n", line_start);
        }
    }

#ifdef _WIN32
        _pclose(process);
#else
        pclose(process);
#endif

    char message[1536];
    std::snprintf(message, sizeof(message), "CHANGELOG #%d\\n%s", curr_commit_id, raw_message);

    if (this->last_commit_id != 0) {
        this->messenger.send(TEMP_CHANGELOG_CHANNEL_ID, message);
    }

    this->last_commit_id = curr_commit_id;
    this->_save_state();

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