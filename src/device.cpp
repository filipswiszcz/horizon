#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "device.h"

constexpr const char *DEVICE_STATE_FILE = "device.dat";

device_status device::initialize(void) noexcept {
    const char *token = std::getenv("TOKEN");
    if (token == nullptr || std::strlen(token) == 0) {
        std::cout << "HOR_ERR: Missing token." << std::endl;
        return device_status::INIT_ERROR;
    }

    std::strncpy(this->token, token, sizeof(this->token) - 1);
    this->token[sizeof(this->token) - 1] = '\0';

    this->is_running = 1;
    this->is_gw_online = 0;
    this->is_mess_online = 0;
    this->channels.counter = 0;

    dispatcher_status disp_init_status = this->dispatcher.initialize();
    if (disp_init_status != dispatcher_status::SUCCESS) {
        std::cout << "HOR_ERR: Dispatcher not initialized." << std::endl;
        return device_status::INIT_ERROR;
    }

    gateway_status gate_init_status = this->gateway.initialize(token);
    if (gate_init_status == gateway_status::SUCCESS) {
        this->is_gw_online = 1;
    } else {
        std::cout << "HOR_WARN: Gateway offline." << std::endl;
    }

    messenger_status mess_init_status = this->messenger.initialize(&this->dispatcher, &this->requester, token);
    if (mess_init_status == messenger_status::SUCCESS) {
        this->is_mess_online = 1;
    } else {
        std::cout << "HOR_WARN: Messenger offline." << std::endl;
    }

    device_status dev_load_status = this->_load_state();
    if (dev_load_status != device_status::SUCCESS) {
        this->last_commit_id = 0;
        std::cout << "HOR_WARN: Could not access or create state file. Defaulting commit ID to 0." << std::endl;
    }

    this->last_commits_check = std::chrono::steady_clock::now();
    this->last_gateway_check = std::chrono::steady_clock::now();

    return device_status::SUCCESS;
}

void device::tick(void) noexcept {
    while (this->is_running) {
        // this->gateway.update(); // i need to think it through

        auto clock_now = std::chrono::steady_clock::now();

        auto gw_clock_elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock_now - this->last_gateway_check).count();
        if (this->is_gw_online && gw_clock_elapsed >= 1) {
            gateway_status gw_update_status = this->gateway.update();
            if (gw_update_status != gateway_status::SUCCESS) {
                std::cout << "HOR_WARN: Gateway connection dropped." << std::endl;
                this->is_gw_online = 0;
            }
            this->last_gateway_check = clock_now;
        }

        auto p4_clock_elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock_now - this->last_commits_check).count();
        if (p4_clock_elapsed >= 120) {
            // this->_check_commits("//depot/...", "changelog");
            // messenger_status mess_send_status = messenger.send(1556246990831095870, "wake the fuck up samurai");
            this->last_commits_check = clock_now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // const requester_status req_send_status = requester.send(this->url, this->msg, std::strlen(this->msg));
        // std::cout << static_cast<u32>(req_send_status) << std::endl;

        // handle responses from the server (eg /status)
    }
}

void device::terminate(void) noexcept {
    this->is_running = 0;
    this->gateway.terminate();
    this->dispatcher.terminate();
}

u64 device::_get_channel_id(const char *name) noexcept {
    if (name == nullptr) {
        return 0;
    }
    for (u32 i = 0; i < this->channels.counter; i++) {
        if (std::strcmp(this->channels.names[i], name) == 0) {
            return this->channels.ids[i];
        }
    }
    return 0;
}

device_status device::_load_state(void) noexcept {
    FILE *file = std::fopen(DEVICE_STATE_FILE, "r");
    if (file == nullptr) {
        if (errno != ENOENT) {
            return device_status::READ_ERROR;
        }
        this->last_commit_id = 0;
        return this->_save_state();
    }
    if (std::fscanf(file, "last_commit_id=%d", &this->last_commit_id) != 1) {
        this->last_commit_id = 0;
    }
    (void) std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_save_state(void) noexcept {
    FILE *file = std::fopen(DEVICE_STATE_FILE, "w");
    if (file == nullptr) {
        return device_status::WRITE_ERROR;
    }
    if (std::fprintf(file, "last_commit_id=%d", this->last_commit_id) == 0) {
        return device_status::WRITE_ERROR;
    }
    (void) std::fclose(file);
    return device_status::SUCCESS;
}

device_status device::_check_commits(const char *depot_name, const char *channel_name) noexcept {
    if (depot_name == nullptr || channel_name == nullptr) {
        return device_status::READ_ERROR;
    }

    char cmd_p4_changes[256]; // find the best size
    std::snprintf(cmd_p4_changes, sizeof(cmd_p4_changes), "p4 changes -m 1 -s submitted %s", depot_name);

    FILE *cmd_p4_changes_pipe;
#ifdef _WIN32
    cmd_p4_changes_pipe = _popen(cmd_p4_changes, "r");
#else
    cmd_p4_changes_pipe = popen(cmd_p4_changes, "r");
#endif    
    if (cmd_p4_changes_pipe == nullptr) {
        return device_status::READ_ERROR;
    }

    char cmd_p4_changes_out[128] = {0};
    if (std::fgets(cmd_p4_changes_out, sizeof(cmd_p4_changes_out), cmd_p4_changes_pipe) == nullptr) {
#ifdef _WIN32
        _pclose(cmd_p4_changes_pipe);
#else
        pclose(cmd_p4_changes_pipe);
#endif
        return device_status::READ_ERROR;
    }

    i32 curr_commit_id = 0;
    if (std::sscanf(cmd_p4_changes_out, "Change %d on", &curr_commit_id) != 1) {
#ifdef _WIN32
        _pclose(cmd_p4_changes_pipe);
#else
        pclose(cmd_p4_changes_pipe);
#endif
        return device_status::READ_ERROR;
    }

    if (curr_commit_id <= this->last_commit_id) {
#ifdef _WIN32
        _pclose(cmd_p4_changes_pipe);
#else
        pclose(cmd_p4_changes_pipe);
#endif
        return device_status::READ_ERROR;
    }

#ifdef _WIN32
        _pclose(cmd_p4_changes_pipe);
#else
        pclose(cmd_p4_changes_pipe);
#endif

    char cmd_p4_desc[128];
    std::snprintf(cmd_p4_desc, sizeof(cmd_p4_desc), "p4 -Ztag -F \"%%desc%%\" describe -s %d", curr_commit_id);
    
    FILE *cmd_p4_desc_pipe;
#ifdef _WIN32
        cmd_p4_desc_pipe = _popen(cmd_p4_desc, "r");
#else
        cmd_p4_desc_pipe = popen(cmd_p4_desc, "r");
#endif
    if (cmd_p4_desc_pipe == nullptr) {
        return device_status::READ_ERROR;
    }

    char content[1024] = {0}, line[256];
    u32 position = 0;
    while (std::fgets(line, sizeof(line), cmd_p4_desc_pipe) != nullptr) {
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
            position += std::snprintf(content + position, sizeof(content) - position, "- %s\\n", line_start);
        } else {
            position += std::snprintf(content + position, sizeof(content) - position, "%s\\n", line_start);
        }
    }

#ifdef _WIN32
        _pclose(cmd_p4_desc_pipe);
#else
        pclose(cmd_p4_desc_pipe);
#endif

    char message[1536];
    std::snprintf(message, sizeof(message), "CHANGELOG #%d\\n%s", curr_commit_id, content);

    if (this->last_commit_id != 0) {
        u64 channel_id = this->_get_channel_id(channel_name);
        if (channel_id != 0) {
            this->messenger.send(channel_id, message);
        }
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
    device.tick();
    device.terminate();
    return 0;
}