#include <cstdio>
#include <cstring>

#include "dispatcher.h"
#include "messenger.h"
#include "requester.h"

messenger_status messenger::initialize(class dispatcher *dispatcher, class requester *requester, const char *token) noexcept {
    if (dispatcher == nullptr || requester == nullptr) {
        return messenger_status::INIT_ERROR;
    }

    this->dispatcher = dispatcher;
    this->requester = requester;

    std::snprintf(this->auth_header, sizeof(this->auth_header), "Authorization: Bot %s\r\n", token);

    dispatcher_status disp_reg_status = this->dispatcher->register_handler(
        task_type::SEND_MESSAGE,
        &messenger::_worker_execute_send,
        this
    );
    if (disp_reg_status != dispatcher_status::SUCCESS) {
        return messenger_status::INIT_ERROR;
    }

    return messenger_status::SUCCESS;
}

messenger_status messenger::send(const u64 channel_id, const char *content) noexcept {
    struct message message;
    std::snprintf(message.url, sizeof(message.url), "%s/channels/%lu/messages", DISCORD_API_URL, channel_id);
    std::snprintf(message.headers, sizeof(message.headers), "Content-Type: application/json\r\n%s", this->auth_header);
    std::snprintf(message.data, sizeof(message.data), "{\"content\": \"%s\"}", content);
    message.data_size = static_cast<u32>(std::strlen(message.data));

    dispatcher_status disp_enqueue_status = this->dispatcher->enqueue(
        task_type::SEND_MESSAGE,
        &message,
        sizeof(message)
    );
    if (disp_enqueue_status != dispatcher_status::SUCCESS) {
        return messenger_status::DISPATCH_ERROR;
    }

    return messenger_status::SUCCESS;
}

void messenger::_worker_execute_send(const u8 *data, void *context) noexcept {
    const message *message = reinterpret_cast<const struct message*>(data);
    auto *instance = static_cast<class messenger*>(context);
    instance->requester->post(message->url, message->headers, message->data, message->data_size);
}