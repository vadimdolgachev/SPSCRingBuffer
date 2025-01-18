#ifndef SPSCRINGBUFFER_H
#define SPSCRINGBUFFER_H

#include <atomic>
#include <vector>

// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html
// Cache line alignment
#if defined(__i386__) || defined(__x86_64__)
#define CACHELINE_SIZE 64
#elif defined(__powerpc64__)
// TODO(dougkwan) This is the L1 D-cache line size of our Power7 machines.
// Need to check if this is appropriate for other PowerPC64 systems.
#define CACHELINE_SIZE 128
#elif defined(__arm__)
// Cache line sizes for ARM: These values are not strictly correct since
// cache line sizes depend on implementations, not architectures.  There
// are even implementations with cache line sizes configurable at boot
// time.
#if defined(__ARM_ARCH_5T__)
#define CACHELINE_SIZE 32
#elif defined(__ARM_ARCH_7A__)
#define CACHELINE_SIZE 64
#endif
#endif

#ifndef CACHELINE_SIZE
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
#define CACHELINE_SIZE 64
#endif

#define CACHELINE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))

enum class SPSCRingBufferStatus {
    Empty,
    Full,
    Success
};

template<typename T, typename Allocator = std::allocator<T>>
class SPSCRingBuffer final {
public:
    explicit SPSCRingBuffer(const size_t capacity, const Allocator& alloc = Allocator()):
        storage(capacity, alloc) {
        if (capacity == 0) {
            throw std::invalid_argument("Buffer capacity must be greater than 0");
        }
    }

    SPSCRingBufferStatus push(T value) {
        // Uses memory_order_relaxed for loading since only push tail changes
        const size_t tailPos = position.tail.load(std::memory_order_relaxed);

        // Uses local head position since there is no need to load actual position each time
        if (getNextPos(tailPos) == localHeadPos) {
            // Load actual head position
            localHeadPos = position.head.load(std::memory_order_acquire);
            if (getNextPos(tailPos) == localHeadPos) {
                return SPSCRingBufferStatus::Full;
            }
        }

        storage[tailPos] = std::move(value);
        position.tail.store(getNextPos(tailPos), std::memory_order_release);
        return SPSCRingBufferStatus::Success;
    }

    SPSCRingBufferStatus pop(T &value) {
        const size_t headPos = position.head.load(std::memory_order_relaxed);

        if (headPos == localTailPos) {
            localTailPos = position.tail.load(std::memory_order_acquire);
            if (headPos == localTailPos) {
                return SPSCRingBufferStatus::Empty;
            }
        }

        value = std::move(storage[headPos]);
        position.head.store(getNextPos(headPos), std::memory_order_release);
        return SPSCRingBufferStatus::Success;
    }

private:
    // Eliminates cache coherency of CPU by using CACHELINE_ALIGNED
    struct Position {
        CACHELINE_ALIGNED std::atomic<size_t> head = {0};
        CACHELINE_ALIGNED std::atomic<size_t> tail = {0};
    };

    constexpr size_t getNextPos(const size_t position) {
        return position + 1 == storage.size() ? 0 : position + 1;
    }

    std::vector<T, Allocator> storage;
    Position position;
    CACHELINE_ALIGNED size_t localTailPos = 0;
    CACHELINE_ALIGNED size_t localHeadPos = 0;
};

#endif //SPSCRINGBUFFER_H
