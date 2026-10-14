#include "dispatcher.h"

// public

dispatcher_status dispatcher::initialize(void) noexcept {
    this->task_counter = 0;
    this->tasks_head = 0;
    this->tasks_tail = 0;
    this->running.store(1);

    for (u8 i = 0; i < static_cast<u8>(task_type::VOID); i++) {
        this->handlers[i] = {nullptr, nullptr};
    }

    for (u32 i = 0; i < MAX_THREAD_WORKERS; i++) {
        this->workers[i] = std::thread(&dispatcher::_worker_update, this);
    }

    return dispatcher_status::SUCCESS;
}

dispatcher_status dispatcher::register_handler(const task_type type, handler handler, void *context) noexcept {
    if (handler == nullptr) {
        return dispatcher_status::NO_HANDLER;
    }
    const u8 index = static_cast<u8>(type);
    if (index < static_cast<u8>(task_type::VOID)) {
        this->handlers[index] = {handler, context};
        return dispatcher_status::SUCCESS;
    }
    return dispatcher_status::NO_HANDLER;
}

dispatcher_status dispatcher::enqueue(const task_type type, const void *data, u32 data_size) noexcept {
    if (data == nullptr) {
        return dispatcher_status::DATA_EMPTY;
    } 
    if (data_size > MAX_TASK_DATA_SIZE) {
        return dispatcher_status::DATA_TOO_LARGE;
    }

    std::unique_lock<std::mutex> lock(this->lock);

    if (this->task_counter >= MAX_QUEUE_TASKS) {
        return dispatcher_status::QUEUE_FULL;
    }

    this->tasks[this->tasks_tail].type = type;
    std::memcpy(this->tasks[this->tasks_tail].data, data, data_size);

    this->task_counter++;
    this->tasks_tail = (this->tasks_tail + 1) % MAX_QUEUE_TASKS;

    this->lock.unlock();
    this->cv.notify_one();

    return dispatcher_status::SUCCESS;
}

void dispatcher::terminate(void) noexcept {
    this->running.store(0);
    this->cv.notify_all();

    for (u32 i = 0; i < MAX_THREAD_WORKERS; i++) {
        if (this->workers[i].joinable()) {
            this->workers[i].join();
        }
    }
}

// private

void dispatcher::_worker_update(void) noexcept {
    while (true) {
        task task;

        {
            std::unique_lock<std::mutex> lock(this->lock);
            this->cv.wait(lock, [this] {
                return this->task_counter > 0 || this->running.load() == 0;
            });

            if (this->task_counter == 0 || this->running.load() == 0) {
                break;
            }

            task = this->tasks[this->tasks_head];
            this->task_counter--;
            this->tasks_head = (this->tasks_head + 1) % MAX_QUEUE_TASKS;
        }

        u8 index = static_cast<u8>(task.type);
        if (index < static_cast<u8>(task_type::VOID)) {
            handler_entry handler = this->handlers[index];
            if (handler.func != nullptr) {
                handler.func(task.data, handler.context);
            }
        }
    }
}