#ifndef AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_
#define AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_

#include <stdint.h>

constexpr uint32_t kFlagPadCount = 8;

struct PollFlagsParam {
    volatile uint32_t *flags;
    uint64_t *seenNs;
    uint64_t *pollIters;
    uint32_t *status;
    uint32_t taskCount;
    uint32_t flagStride;
    uint64_t timeoutNs;
};

struct StampFlagParam {
    volatile uint32_t *flags;
    uint64_t *seenNs;
    uint32_t *status;
    uint32_t taskIndex;
    uint32_t flagStride;
};

struct StampOnlyParam {
    uint64_t *seenNs;
    uint32_t *status;
    uint32_t taskIndex;
};

struct ReadBenchParam {
    volatile uint32_t *data;
    uint64_t *elapsedNs;
    uint64_t *checksum;
    uint32_t *status;
    uint64_t dataWords;
    uint64_t readsPerRepeat;
    uint64_t repeat;
    uint64_t strideWords;
};

struct ReadChaseParam {
    volatile uint32_t *next;
    uint64_t *elapsedNs;
    uint64_t *checksum;
    uint32_t *status;
    uint64_t dataWords;
    uint64_t steps;
    uint32_t startIndex;
};

#ifdef __cplusplus
extern "C" {
#endif

uint32_t PollFlags(PollFlagsParam *param);
uint32_t StampFlag(StampFlagParam *param);
uint32_t StampOnly(StampOnlyParam *param);
uint32_t ReadBench(ReadBenchParam *param);
uint32_t ReadChase(ReadChaseParam *param);
uint32_t Noop(void *param);

#ifdef __cplusplus
}
#endif

#endif // AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_
