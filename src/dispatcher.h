#ifndef HOR_DISPATCHER_H
#define HOR_DISPATCHER_H

#include <mutex>
#include <condition_variable>
#include <thread>

#include "common.h"

constexpr u32 TASK_CONTEXT_SIZE = 32;

struct task {
    void (*execute) (void *context);
    alignas(8) u8 context[TASK_CONTEXT_SIZE];

    template<typename T>
    void bind(void (*func) (void*), const T &data) noexcept {
        this->execute = func;
        std::memcpy(this->context, &data, sizeof(T));
    }
};

enum class dispatcher_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    QUEUE_FULL
};

class dispatcher {
public:
    [[nodiscard]] dispatcher_status initialize(void);
    [[nodiscard]] dispatcher_status enqueue(const task &task);
    void terminate(void);
private:
    static constexpr u32 MAX_THREAD_WORKERS = 2;
    static constexpr u32 MAX_QUEUE_TASKS = 16;

    std::thread workers[MAX_THREAD_WORKERS];
    std::mutex lock;
    std::condition_variable cv;

    task tasks[MAX_QUEUE_TASKS];
    u32 task_counter, tasks_head, tasks_tail;

    std::atomic<u8> running;

    void _worker_update(void);
};

#endif //!HOR_DISPATCHER_H