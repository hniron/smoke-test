#include "aicpu_poll_kernel.h"

#include <time.h>

namespace {
constexpr uint32_t kSuccess = 0;
constexpr uint32_t kInvalidParam = 1;
constexpr uint32_t kTimeout = 2;

uint64_t NowNs()
{
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    (void)clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}
} // namespace

extern "C" uint32_t PollFlags(PollFlagsParam *param)
{
    if (param == nullptr || param->flags == nullptr || param->seenNs == nullptr ||
        param->pollIters == nullptr || param->status == nullptr || param->taskCount == 0 ||
        param->flagStride == 0 || param->timeoutNs == 0) {
        return kInvalidParam;
    }

    param->status[0] = kSuccess;
    const uint64_t startNs = NowNs();
    for (uint32_t i = 0; i < param->taskCount; ++i) {
        const uint32_t expected = i + 1U;
        uint64_t iters = 0;
        for (;;) {
            const uint32_t value = param->flags[i * param->flagStride];
            ++iters;
            if (value >= expected) {
                param->seenNs[i] = NowNs();
                param->pollIters[i] = iters;
                break;
            }
            if ((NowNs() - startNs) > param->timeoutNs) {
                param->status[0] = kTimeout;
                param->seenNs[i] = NowNs();
                param->pollIters[i] = iters;
                return kTimeout;
            }
        }
    }
    return kSuccess;
}

extern "C" uint32_t StampFlag(StampFlagParam *param)
{
    if (param == nullptr || param->flags == nullptr || param->seenNs == nullptr ||
        param->status == nullptr || param->flagStride == 0) {
        return kInvalidParam;
    }

    const uint32_t expected = param->taskIndex + 1U;
    const uint32_t value = param->flags[param->taskIndex * param->flagStride];
    param->seenNs[param->taskIndex] = NowNs();
    if (value < expected) {
        param->status[0] = kInvalidParam;
        return kInvalidParam;
    }
    return kSuccess;
}

extern "C" uint32_t StampOnly(StampOnlyParam *param)
{
    if (param == nullptr || param->seenNs == nullptr || param->status == nullptr) {
        return kInvalidParam;
    }

    param->seenNs[param->taskIndex] = NowNs();
    param->status[0] = kSuccess;
    return kSuccess;
}


extern "C" uint32_t ReadBench(ReadBenchParam *param)
{
    if (param == nullptr || param->data == nullptr || param->elapsedNs == nullptr ||
        param->checksum == nullptr || param->status == nullptr || param->dataWords == 0 ||
        param->readsPerRepeat == 0 || param->repeat == 0 || param->strideWords == 0) {
        return kInvalidParam;
    }

    param->status[0] = kSuccess;
    uint64_t sum = 0;
    uint64_t idx = 0;
    uint64_t stride = param->strideWords % param->dataWords;
    if (stride == 0) {
        stride = param->dataWords;
    }
    const uint64_t startNs = NowNs();
    for (uint64_t r = 0; r < param->repeat; ++r) {
        for (uint64_t i = 0; i < param->readsPerRepeat; ++i) {
            sum += static_cast<uint64_t>(param->data[idx]);
            idx += stride;
            if (idx >= param->dataWords) {
                idx -= param->dataWords;
            }
        }
    }
    const uint64_t endNs = NowNs();
    param->elapsedNs[0] = endNs - startNs;
    param->checksum[0] = sum;
    return kSuccess;
}

extern "C" uint32_t ReadChase(ReadChaseParam *param)
{
    if (param == nullptr || param->next == nullptr || param->elapsedNs == nullptr ||
        param->checksum == nullptr || param->status == nullptr || param->dataWords == 0 ||
        param->steps == 0 || param->startIndex >= param->dataWords) {
        return kInvalidParam;
    }

    param->status[0] = kSuccess;
    uint32_t idx = param->startIndex;
    uint64_t sum = 0;
    const uint64_t startNs = NowNs();
    for (uint64_t i = 0; i < param->steps; ++i) {
        idx = param->next[idx];
        if (idx >= param->dataWords) {
            param->status[0] = kInvalidParam;
            return kInvalidParam;
        }
        sum += static_cast<uint64_t>(idx);
    }
    const uint64_t endNs = NowNs();
    param->elapsedNs[0] = endNs - startNs;
    param->checksum[0] = (sum << 32U) ^ static_cast<uint64_t>(idx);
    return kSuccess;
}

extern "C" uint32_t Noop(void *param)
{
    (void)param;
    return kSuccess;
}
