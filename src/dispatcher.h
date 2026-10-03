#ifndef HOR_DISPATCHER_H
#define HOR_DISPATCHER_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "common.h"

struct task {
    void (*execute) (void *context);
    void *context;
};

enum class dispatcher_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    QUEUE_FULL
};

class dispatcher {
public:
    [[nodiscard]] dispatcher_status initialize(void);
    [[nodiscard]] dispatcher_status enqueue(task &task);
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