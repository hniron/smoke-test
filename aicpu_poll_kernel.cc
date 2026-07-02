#include "aicpu_poll_kernel.h"

#include "cpu_kernel.h"

#include <stdint.h>
#include <time.h>

#ifndef KERNEL_STATUS_OK
#define KERNEL_STATUS_OK 0
#endif
#ifndef KERNEL_STATUS_PARAM_INVALID
#define KERNEL_STATUS_PARAM_INVALID 1
#endif

namespace aicpu {
namespace {

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

bool HasData(Tensor *tensor)
{
    return tensor != nullptr && tensor->GetData() != nullptr;
}

bool GetConfig(CpuKernelContext &ctx, uint32_t index, const int64_t **config)
{
    Tensor *tensor = ctx.Input(index);
    if (!HasData(tensor)) {
        return false;
    }
    *config = reinterpret_cast<const int64_t *>(tensor->GetData());
    return true;
}

uint32_t SetStatus(Tensor *statusTensor, uint32_t status)
{
    if (!HasData(statusTensor)) {
        return KERNEL_STATUS_PARAM_INVALID;
    }
    uint32_t *statusPtr = reinterpret_cast<uint32_t *>(statusTensor->GetData());
    statusPtr[0] = status;
    return KERNEL_STATUS_OK;
}

class AivAicpuNoopKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        return SetStatus(ctx.Output(0), kAicpuSuccess);
    }
};

class AivAicpuPollFlagsKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        Tensor *flagsTensor = ctx.Input(0);
        const int64_t *config = nullptr;
        Tensor *seenNsTensor = ctx.Output(0);
        Tensor *pollItersTensor = ctx.Output(1);
        Tensor *statusTensor = ctx.Output(2);
        if (!HasData(flagsTensor) || !GetConfig(ctx, 1, &config) || !HasData(seenNsTensor) ||
            !HasData(pollItersTensor) || !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }

        const int64_t taskCount64 = config[0];
        const int64_t flagStride64 = config[1];
        const int64_t timeoutNs64 = config[2];
        if (taskCount64 <= 0 || flagStride64 <= 0 || timeoutNs64 <= 0 ||
            taskCount64 > static_cast<int64_t>(UINT32_MAX) ||
            flagStride64 > static_cast<int64_t>(UINT32_MAX)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }

        const uint32_t taskCount = static_cast<uint32_t>(taskCount64);
        const uint32_t flagStride = static_cast<uint32_t>(flagStride64);
        const uint64_t timeoutNs = static_cast<uint64_t>(timeoutNs64);
        volatile uint32_t *flags = reinterpret_cast<volatile uint32_t *>(flagsTensor->GetData());
        uint64_t *seenNs = reinterpret_cast<uint64_t *>(seenNsTensor->GetData());
        uint64_t *pollIters = reinterpret_cast<uint64_t *>(pollItersTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());

        status[0] = kAicpuSuccess;
        const uint64_t startNs = NowNs();
        for (uint32_t i = 0; i < taskCount; ++i) {
            const uint32_t expected = i + 1U;
            uint64_t iters = 0;
            for (;;) {
                const uint32_t value = flags[i * flagStride];
                ++iters;
                if (value >= expected) {
                    seenNs[i] = NowNs();
                    pollIters[i] = iters;
                    break;
                }
                if ((NowNs() - startNs) > timeoutNs) {
                    status[0] = kAicpuTimeout;
                    seenNs[i] = NowNs();
                    pollIters[i] = iters;
                    return KERNEL_STATUS_OK;
                }
            }
        }
        return KERNEL_STATUS_OK;
    }
};

class AivAicpuPollFlagsScanKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        Tensor *flagsTensor = ctx.Input(0);
        const int64_t *config = nullptr;
        Tensor *seenNsTensor = ctx.Output(0);
        Tensor *pollItersTensor = ctx.Output(1);
        Tensor *statusTensor = ctx.Output(2);
        if (!HasData(flagsTensor) || !GetConfig(ctx, 1, &config) || !HasData(seenNsTensor) ||
            !HasData(pollItersTensor) || !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }

        const int64_t taskCount64 = config[0];
        const int64_t flagStride64 = config[1];
        const int64_t timeoutNs64 = config[2];
        if (taskCount64 <= 0 || flagStride64 <= 0 || timeoutNs64 <= 0 ||
            taskCount64 > static_cast<int64_t>(UINT32_MAX) ||
            flagStride64 > static_cast<int64_t>(UINT32_MAX)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }

        const uint32_t taskCount = static_cast<uint32_t>(taskCount64);
        const uint32_t flagStride = static_cast<uint32_t>(flagStride64);
        const uint64_t timeoutNs = static_cast<uint64_t>(timeoutNs64);
        volatile uint32_t *flags = reinterpret_cast<volatile uint32_t *>(flagsTensor->GetData());
        uint64_t *seenNs = reinterpret_cast<uint64_t *>(seenNsTensor->GetData());
        uint64_t *pollIters = reinterpret_cast<uint64_t *>(pollItersTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());

        status[0] = kAicpuSuccess;
        for (uint32_t i = 0; i < taskCount; ++i) {
            seenNs[i] = 0;
            pollIters[i] = 0;
        }

        uint32_t remaining = taskCount;
        const uint64_t startNs = NowNs();
        while (remaining > 0) {
            for (uint32_t i = 0; i < taskCount; ++i) {
                if (seenNs[i] != 0) {
                    continue;
                }
                ++pollIters[i];
                const uint32_t value = flags[i * flagStride];
                if (value >= i + 1U) {
                    seenNs[i] = NowNs();
                    --remaining;
                }
            }
            if ((NowNs() - startNs) > timeoutNs) {
                status[0] = kAicpuTimeout;
                return KERNEL_STATUS_OK;
            }
        }
        return KERNEL_STATUS_OK;
    }
};

class AivAicpuStampFlagKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        Tensor *flagsTensor = ctx.Input(0);
        const int64_t *config = nullptr;
        Tensor *seenNsTensor = ctx.Output(0);
        Tensor *statusTensor = ctx.Output(1);
        if (!HasData(flagsTensor) || !GetConfig(ctx, 1, &config) || !HasData(seenNsTensor) ||
            !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const int64_t taskIndex64 = config[0];
        const int64_t flagStride64 = config[1];
        if (taskIndex64 < 0 || flagStride64 <= 0 || taskIndex64 > static_cast<int64_t>(UINT32_MAX) ||
            flagStride64 > static_cast<int64_t>(UINT32_MAX)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const uint32_t taskIndex = static_cast<uint32_t>(taskIndex64);
        const uint32_t flagStride = static_cast<uint32_t>(flagStride64);
        volatile uint32_t *flags = reinterpret_cast<volatile uint32_t *>(flagsTensor->GetData());
        uint64_t *seenNs = reinterpret_cast<uint64_t *>(seenNsTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());

        const uint32_t expected = taskIndex + 1U;
        const uint32_t value = flags[taskIndex * flagStride];
        seenNs[taskIndex] = NowNs();
        if (value < expected) {
            status[0] = kAicpuInvalidParam;
            return KERNEL_STATUS_PARAM_INVALID;
        }
        status[0] = kAicpuSuccess;
        return KERNEL_STATUS_OK;
    }
};

class AivAicpuStampOnlyKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        const int64_t *config = nullptr;
        Tensor *seenNsTensor = ctx.Output(0);
        Tensor *statusTensor = ctx.Output(1);
        if (!GetConfig(ctx, 1, &config) || !HasData(seenNsTensor) || !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const int64_t taskIndex64 = config[0];
        if (taskIndex64 < 0 || taskIndex64 > static_cast<int64_t>(UINT32_MAX)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        uint64_t *seenNs = reinterpret_cast<uint64_t *>(seenNsTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());
        seenNs[static_cast<uint32_t>(taskIndex64)] = NowNs();
        status[0] = kAicpuSuccess;
        return KERNEL_STATUS_OK;
    }
};

class AivAicpuReadBenchKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        Tensor *dataTensor = ctx.Input(0);
        const int64_t *config = nullptr;
        Tensor *elapsedNsTensor = ctx.Output(0);
        Tensor *checksumTensor = ctx.Output(1);
        Tensor *statusTensor = ctx.Output(2);
        if (!HasData(dataTensor) || !GetConfig(ctx, 1, &config) || !HasData(elapsedNsTensor) ||
            !HasData(checksumTensor) || !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const int64_t dataWords64 = config[0];
        const int64_t readsPerRepeat64 = config[1];
        const int64_t repeat64 = config[2];
        const int64_t strideWords64 = config[3];
        if (dataWords64 <= 0 || readsPerRepeat64 <= 0 || repeat64 <= 0 || strideWords64 <= 0) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const uint64_t dataWords = static_cast<uint64_t>(dataWords64);
        const uint64_t readsPerRepeat = static_cast<uint64_t>(readsPerRepeat64);
        const uint64_t repeat = static_cast<uint64_t>(repeat64);
        uint64_t stride = static_cast<uint64_t>(strideWords64) % dataWords;
        if (stride == 0) {
            stride = dataWords;
        }

        volatile uint32_t *data = reinterpret_cast<volatile uint32_t *>(dataTensor->GetData());
        uint64_t *elapsedNs = reinterpret_cast<uint64_t *>(elapsedNsTensor->GetData());
        uint64_t *checksum = reinterpret_cast<uint64_t *>(checksumTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());

        status[0] = kAicpuSuccess;
        uint64_t sum = 0;
        uint64_t idx = 0;
        const uint64_t startNs = NowNs();
        for (uint64_t r = 0; r < repeat; ++r) {
            for (uint64_t i = 0; i < readsPerRepeat; ++i) {
                sum += static_cast<uint64_t>(data[idx]);
                idx += stride;
                if (idx >= dataWords) {
                    idx -= dataWords;
                }
            }
        }
        elapsedNs[0] = NowNs() - startNs;
        checksum[0] = sum;
        return KERNEL_STATUS_OK;
    }
};

class AivAicpuReadChaseKernel : public CpuKernel {
public:
    uint32_t Compute(CpuKernelContext &ctx) override
    {
        Tensor *nextTensor = ctx.Input(0);
        const int64_t *config = nullptr;
        Tensor *elapsedNsTensor = ctx.Output(0);
        Tensor *checksumTensor = ctx.Output(1);
        Tensor *statusTensor = ctx.Output(2);
        if (!HasData(nextTensor) || !GetConfig(ctx, 1, &config) || !HasData(elapsedNsTensor) ||
            !HasData(checksumTensor) || !HasData(statusTensor)) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const int64_t dataWords64 = config[0];
        const int64_t steps64 = config[1];
        const int64_t startIndex64 = config[2];
        if (dataWords64 <= 0 || steps64 <= 0 || startIndex64 < 0 || startIndex64 >= dataWords64) {
            return KERNEL_STATUS_PARAM_INVALID;
        }
        const uint64_t dataWords = static_cast<uint64_t>(dataWords64);
        const uint64_t steps = static_cast<uint64_t>(steps64);
        uint32_t idx = static_cast<uint32_t>(startIndex64);
        volatile uint32_t *next = reinterpret_cast<volatile uint32_t *>(nextTensor->GetData());
        uint64_t *elapsedNs = reinterpret_cast<uint64_t *>(elapsedNsTensor->GetData());
        uint64_t *checksum = reinterpret_cast<uint64_t *>(checksumTensor->GetData());
        uint32_t *status = reinterpret_cast<uint32_t *>(statusTensor->GetData());

        status[0] = kAicpuSuccess;
        uint64_t sum = 0;
        const uint64_t startNs = NowNs();
        for (uint64_t i = 0; i < steps; ++i) {
            idx = next[idx];
            if (idx >= dataWords) {
                status[0] = kAicpuInvalidParam;
                return KERNEL_STATUS_PARAM_INVALID;
            }
            sum += static_cast<uint64_t>(idx);
        }
        elapsedNs[0] = NowNs() - startNs;
        checksum[0] = (sum << 32U) ^ static_cast<uint64_t>(idx);
        return KERNEL_STATUS_OK;
    }
};

REGISTER_CPU_KERNEL(kAicpuNoopOpName, AivAicpuNoopKernel);
REGISTER_CPU_KERNEL(kAicpuPollFlagsOpName, AivAicpuPollFlagsKernel);
REGISTER_CPU_KERNEL(kAicpuPollFlagsScanOpName, AivAicpuPollFlagsScanKernel);
REGISTER_CPU_KERNEL(kAicpuStampFlagOpName, AivAicpuStampFlagKernel);
REGISTER_CPU_KERNEL(kAicpuStampOnlyOpName, AivAicpuStampOnlyKernel);
REGISTER_CPU_KERNEL(kAicpuReadBenchOpName, AivAicpuReadBenchKernel);
REGISTER_CPU_KERNEL(kAicpuReadChaseOpName, AivAicpuReadChaseKernel);

} // namespace
} // namespace aicpu
