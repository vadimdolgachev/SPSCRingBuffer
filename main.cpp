#include <cassert>
#include <thread>

#include "SPSCRingBuffer.h"

int main() {
    constexpr int bufferCapacity = 4096;
    SPSCRingBuffer<int> ringBuffer(bufferCapacity);

    std::thread producer([&ringBuffer] {
        for (int i = 0; i < bufferCapacity; ++i) {
            while (!ringBuffer.push(i + 1)) {
                std::this_thread::yield();
            }
        }
    });
    uint64_t sum = 0;
    std::thread consumer([&ringBuffer, &sum] {
        for (int i = 0; i < bufferCapacity; ++i) {
            int value = 0;
            while (!ringBuffer.pop(value)) {
                std::this_thread::yield();
            }
            sum += value;
        }
    });

    producer.join();
    consumer.join();

    assert(sum == bufferCapacity * (bufferCapacity + 1) / 2);
    return 0;
}
