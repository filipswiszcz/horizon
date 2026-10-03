#include "dispatcher.h"

// public

dispatcher_status dispatcher::initialize(void) {
    this->task_counter = 0;
    this->tasks_head = 0;
    this->tasks_tail = 0;
    this->running.store(1);

    for (u32 i = 0; i < MAX_THREAD_WORKERS; i++) {
        this->workers[i] = std::thread(&dispatcher::_worker_update, this);
    }

    return dispatcher_status::SUCCESS;
}

dispatcher_status dispatcher::enqueue(task &task) {
    std::unique_lock<std::mutex> lock(this->lock);

    if (this->task_counter >= MAX_QUEUE_TASKS) {
        return dispatcher_status::QUEUE_FULL;
    }

    this->tasks[this->tasks_tail] = task;
    this->task_counter++;
    this->tasks_tail = (this->tasks_tail + 1) % MAX_QUEUE_TASKS;

    this->lock.unlock();
    this->cv.notify_one();

    return dispatcher_status::SUCCESS;
}

void dispatcher::terminate(void) {
    this->running.store(0);
    this->cv.notify_all();

    for (u32 i = 0; i < MAX_THREAD_WORKERS; i++) {
        if (this->workers[i].joinable()) {
            this->workers[i].join();
        }
    }
}

// private

void dispatcher::_worker_update(void) {
    while (true) {
        task task = {nullptr, nullptr};

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

        if (task.execute != nullptr) {
            task.execute(task.context);
        }
    }
}