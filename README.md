# SPSCRingBuffer

Single producer, single consumer, lock-free queue

# Performance
## Naive lock-free approach:
```
Median: 262 ms
Mean: 261.00 ms
Standard Deviation: 6.89 ms
```
## SPSCRingBuffer:
```
Median: 20 ms
Mean: 20.21 ms
Standard Deviation: 0.83 ms
```