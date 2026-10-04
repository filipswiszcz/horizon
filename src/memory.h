#ifndef HOR_MEMORY_H
#define HOR_MEMORY_H

#include "common.h"

enum class allocator_status : u8 {
    SUCCESS = 0,
    OUT_OF_MEMORY,
    INVALID_ARGUMENT
};

template<typename T>
struct allocator_result {
    allocator_status status;
    T *ptr;
};

template<typename T, u32 capacity>
class lf_pool_allocator {
public:
    lf_pool_allocator() noexcept {
        for (u32 i = 0; i < capacity; i++) {
            this->used[i].store(0);
        }
    }

    [[nodiscard]] allocator_result<T> allocate(void) noexcept {
        for (u32 i = 0; i < capacity; i++) {
            u8 expected = 0;
            if (this->used[i].compare_exchange_strong(expected, 1)) {
                return {allocator_status::SUCCESS, &this->pool[i]};
            }
        }
        return {allocator_status::OUT_OF_MEMORY, nullptr};
    }

    [[nodiscard]] allocator_status free(const T *ptr) noexcept {
        if (ptr == nullptr) {
            return allocator_status::INVALID_ARGUMENT;
        }
        u32 index = static_cast<u32>(ptr - this->pool);
        if (index < capacity) {
            this->used[index].store(0);
            return allocator_status::SUCCESS;
        }
        return allocator_status::INVALID_ARGUMENT;
    }
private:
    T pool[capacity];
    std::atomic<u8> used[capacity];
};

#endif // !HOR_MEMORY_H