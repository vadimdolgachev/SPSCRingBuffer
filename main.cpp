#include <cassert>
#include <thread>

#include "SPSCRingBuffer.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>

template<typename Unit = std::chrono::microseconds>
class ExecutionTimer final {
public:
    using Clock = std::chrono::steady_clock;

    explicit ExecutionTimer(const size_t count) : count(count), times(count) {
        if (count == 0) {
            throw std::invalid_argument("Count must be greater than 0");
        }
    }

    ExecutionTimer(const ExecutionTimer &other) = delete;
    ExecutionTimer &operator=(const ExecutionTimer &other) = delete;
    ExecutionTimer(ExecutionTimer &&other) noexcept = default;
    ExecutionTimer &operator=(ExecutionTimer &&other) noexcept = default;

    template<typename F>
    void measure(F &&f) {
        for (size_t i = 0; i < count; ++i) {
            const auto begin = Clock::now();
            std::invoke(std::forward<F>(f));
            times[i] = std::chrono::duration_cast<Unit>(Clock::now() - begin).count();
        }
    }

    [[nodiscard]] typename Unit::rep median() const {
        return times[times.size() / 2];
    }

    [[nodiscard]] double mean() const {
        return std::accumulate(times.begin(), times.end(), 0.0,
                               [](const double acc, const auto &val) {
                                   return acc + static_cast<double>(val);
                               }) /
            static_cast<double>(times.size());
    }

    [[nodiscard]] double standardDeviation() const {
        const auto avg = mean();
        const auto variance = std::accumulate(times.begin(), times.end(), 0.0,
                                              [avg](const double acc, const auto &val) {
                                                  const double diff = static_cast<double>(val) - avg;
                                                  return acc + diff * diff;
                                              });
        return sqrt(variance / static_cast<double>(times.size()));
    }

    void report() const {
        std::sort(times.begin(), times.end());
        std::cout << std::format("Median: {} {}\n", median(), unitName());
        std::cout << std::format("Mean: {:.2f} {}\n", mean(), unitName());
        std::cout << std::format("Standard Deviation: {:.2f} {}\n", standardDeviation(), unitName());
    }

    static constexpr const char * unitName() {
        if constexpr (std::is_same_v<Unit, std::chrono::nanoseconds>) {
            return "ns";
        } else if constexpr (std::is_same_v<Unit, std::chrono::microseconds>) {
            return "μs";
        } else if constexpr (std::is_same_v<Unit, std::chrono::milliseconds>) {
            return "ms";
        } else if constexpr (std::is_same_v<Unit, std::chrono::seconds>) {
            return "s";
        } else {
            return "unknown";
        }
    }

private:
    const size_t count;
    mutable std::vector<typename Unit::rep> times;
};

int main() {
    ExecutionTimer<std::chrono::milliseconds> measurer(1000);
    constexpr int64_t opCount = 10'000'000;
    measurer.measure([] {
        constexpr int bufferCapacity = 4096;
        SPSCRingBuffer<int> ringBuffer(bufferCapacity);
        std::thread producer([&ringBuffer] {
            for (int i = 0; i < opCount; ++i) {
                while (ringBuffer.push(i + 1) == SPSCRingBufferStatus::Full) {
                    std::this_thread::yield();
                }
            }
        });
        uint64_t sum = 0;
        std::thread consumer([&ringBuffer, &sum] {
            for (int i = 0; i < opCount; ++i) {
                int value = 0;
                while (ringBuffer.pop(value) == SPSCRingBufferStatus::Empty) {
                    std::this_thread::yield();
                }
                sum += value;
            }
        });

        producer.join();
        consumer.join();

        assert(sum == opCount * (opCount + 1) / 2 && "Producer and consumer do not have the same value");
    });

    measurer.report();
    return 0;
}
