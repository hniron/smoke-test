#ifndef AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_
#define AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_

#include <stdint.h>

constexpr const char *kAicpuNoopOpName = "AivAicpuNoop";
constexpr const char *kAicpuPollFlagsOpName = "AivAicpuPollFlags";
constexpr const char *kAicpuStampFlagOpName = "AivAicpuStampFlag";
constexpr const char *kAicpuStampOnlyOpName = "AivAicpuStampOnly";
constexpr const char *kAicpuReadBenchOpName = "AivAicpuReadBench";
constexpr const char *kAicpuReadChaseOpName = "AivAicpuReadChase";

constexpr uint32_t kFlagPadCount = 8;
constexpr uint32_t kAicpuSuccess = 0;
constexpr uint32_t kAicpuInvalidParam = 1;
constexpr uint32_t kAicpuTimeout = 2;

#endif // AIV_AICPU_BENCH_AICPU_POLL_KERNEL_H_
