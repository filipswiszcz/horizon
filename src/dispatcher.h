#ifndef HOR_DISPATCHER_H
#define HOR_DISPATCHER_H

#include <mutex>
#include <condition_variable>
#include <thread>

#include "common.h"

enum class task_type : u8 {
    SEND_MESSAGE = 0,
    VOID
};

typedef void (*handler) (const u8 *data, void *context);

constexpr u32 MAX_TASK_DATA_SIZE = 2560;

struct task {
    task_type type;
    u8 data[MAX_TASK_DATA_SIZE];
    // alignas(8) u8 context[TASK_CONTEXT_SIZE];
};

enum class dispatcher_status : u8 {
    SUCCESS = 0,
    INIT_ERROR,
    QUEUE_FULL,
    DATA_TOO_LARGE,
    DATA_EMPTY,
    NO_HANDLER
};

class dispatcher {
public:
    [[nodiscard]] dispatcher_status initialize(void) noexcept;
    [[nodiscard]] dispatcher_status register_handler(const task_type type, handler handler, void *context = nullptr) noexcept;
    [[nodiscard]] dispatcher_status enqueue(const task_type type, const void *data, u32 data_size) noexcept;
    void terminate(void) noexcept;
private:
    static constexpr u32 MAX_THREAD_WORKERS = 2;
    static constexpr u32 MAX_QUEUE_TASKS = 16;

    struct handler_entry {
        handler func;
        void *context;
    };

    handler_entry handlers[static_cast<u8>(task_type::VOID)];

    std::thread workers[MAX_THREAD_WORKERS];
    std::mutex lock;
    std::condition_variable cv;

    task tasks[MAX_QUEUE_TASKS];
    u32 task_counter, tasks_head, tasks_tail;

    std::atomic<u8> running;

    void _worker_update(void) noexcept;
};

#endif //!HOR_DISPATCHER_H