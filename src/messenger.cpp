#include "messenger.h"

messenger_status messenger::initialize(class dispatcher *task_dispatcher) noexcept {
    if (task_dispatcher == nullptr) {
        return messenger_status::INIT_ERROR;
    }
    this->task_dispatcher = task_dispatcher;
    return messenger_status::SUCCESS;
}

messenger_status messenger::send(const u64 channel_id, const char *text) noexcept {
    allocator_result<message> msg_alloc_result = this->msg_pool.allocate();
    if (msg_alloc_result.status != allocator_status::SUCCESS) {
        return messenger_status::OUT_OF_MEMORY;
    }

    msg_alloc_result.ptr->channel_id = channel_id;
    // msg_alloc_result.ptr->cont_length = static_cast<u32>(content.length());

    // dispatcher_status disp_enqueue_status = this->task_dispatcher.enqueue(task);
    // if (disp_enqueue_status != SUCCESS) {
    //     // delete
    //     return messenger_status::DISPATCH_ERROR;
    // }

    struct context {
        messenger *messenger;
        message *message;
    };
    context context = {this, msg_alloc_result.ptr};

    task task = {nullptr};
    // task.bind(&messenger::_worker_execute_send, context);

    if (this->task_dispatcher->enqueue(task) != dispatcher_status::SUCCESS) {
        this->msg_pool.free(msg_alloc_result.ptr);
        return messenger_status::DISPATCH_ERROR;
    }

    return messenger_status::SUCCESS;
}

void messenger::_worker_execute_send(const void *payload) noexcept {
    struct context {
        messenger *messenger;
        message *message;
    };
    // auto *context = reinterpret_cast<struct context*>(payload);
}