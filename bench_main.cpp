#include "aicpu_poll_kernel.h"

#include "acl/acl.h"
#include "acl/acl_op_compiler.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#if defined(ENABLE_AIV_PRODUCER)
extern "C" int LaunchAivWriteFlags(void *flags, uint32_t taskCount, uint32_t delayIters, aclrtStream stream);
#endif
#if defined(ENABLE_SIMT_STORE_PRODUCER)
extern "C" int LaunchSimtWriteFlagsStore(void *flags, uint32_t taskCount, uint32_t delayIters,
    uint32_t simtThreads, aclrtStream stream);
extern "C" int LaunchSimtWriteFlagsStoreParallel(void *flags, uint32_t taskCount, uint32_t delayIters,
    uint32_t simtThreads, aclrtStream stream);
#endif
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
extern "C" int LaunchSimtWriteFlagsAtomic(void *flags, uint32_t taskCount, uint32_t delayIters,
    uint32_t simtThreads, aclrtStream stream);
extern "C" int LaunchSimtWriteFlagsAtomicParallel(void *flags, uint32_t taskCount, uint32_t delayIters,
    uint32_t simtThreads, aclrtStream stream);
#endif

constexpr uint32_t kDefaultTasks = 64;
constexpr uint32_t kDefaultIters = 20;
constexpr uint32_t kDefaultWarmup = 3;
constexpr uint64_t kDefaultTimeoutMs = 5000;
constexpr uint32_t kDefaultSimtThreads = 32;

namespace {

struct Options {
    int32_t device = 0;
    uint32_t tasks = kDefaultTasks;
    uint32_t iters = kDefaultIters;
    uint32_t warmup = kDefaultWarmup;
    uint64_t timeoutMs = kDefaultTimeoutMs;
    uint32_t delayIters = 0;
    uint32_t simtThreads = kDefaultSimtThreads;
    std::string mode = "all";
};

struct DeviceBuffers {
    void *flags = nullptr;
    void *seenNs = nullptr;
    void *pollIters = nullptr;
    void *status = nullptr;
    void *config = nullptr;
    size_t flagsBytes = 0;
    size_t timeBytes = 0;
    size_t configBytes = 0;
};

struct RunResult {
    double hostTotalUs = 0.0;
    std::vector<uint64_t> seenNs;
    std::vector<uint64_t> pollIters;
    uint32_t status = 0;
};

uint64_t HostNowNs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

int Fail(const char *expr, int ret, int line)
{
    std::cerr << "FAIL line " << line << ": " << expr << " ret=" << ret << std::endl;
    const char *recent = aclGetRecentErrMsg();
    if (recent != nullptr && recent[0] != '\0') {
        std::cerr << "ACL recent error: " << recent << std::endl;
    }
    return ret == 0 ? 1 : ret;
}

#define CHECK_ACL(expr) do { aclError _ret = (expr); if (_ret != ACL_SUCCESS) return Fail(#expr, static_cast<int>(_ret), __LINE__); } while (0)

aclError LogAclDeviceCount()
{
    uint32_t count = 0;
    aclError ret = aclrtGetDeviceCount(&count);
    std::cerr << "aclrtGetDeviceCount ret=" << static_cast<int>(ret);
    if (ret == ACL_SUCCESS) {
        std::cerr << " count=" << count;
    }
    std::cerr << std::endl;
    return ret;
}

bool ParseU32(const char *arg, const char *name, uint32_t *out)
{
    const size_t n = std::strlen(name);
    if (std::strncmp(arg, name, n) != 0 || arg[n] != '=') {
        return false;
    }
    *out = static_cast<uint32_t>(std::strtoul(arg + n + 1, nullptr, 0));
    return true;
}

bool ParseU64(const char *arg, const char *name, uint64_t *out)
{
    const size_t n = std::strlen(name);
    if (std::strncmp(arg, name, n) != 0 || arg[n] != '=') {
        return false;
    }
    *out = std::strtoull(arg + n + 1, nullptr, 0);
    return true;
}

bool ParseI32(const char *arg, const char *name, int32_t *out)
{
    const size_t n = std::strlen(name);
    if (std::strncmp(arg, name, n) != 0 || arg[n] != '=') {
        return false;
    }
    *out = static_cast<int32_t>(std::strtol(arg + n + 1, nullptr, 0));
    return true;
}

bool ParseStr(const char *arg, const char *name, std::string *out)
{
    const size_t n = std::strlen(name);
    if (std::strncmp(arg, name, n) != 0 || arg[n] != '=') {
        return false;
    }
    *out = arg + n + 1;
    return true;
}

void Usage(const char *prog)
{
    std::cerr
        << "usage: " << prog << " [--device=0] [--tasks=64] [--iters=20] [--warmup=3]\n"
        << "       [--timeout-ms=5000] [--delay-iters=0] [--simt-threads=32]\n"
        << "       [--mode=all|aiv_store|simt_store|simt_atomic|aicpu_noop]\n"
        << "       [--mode=aiv_only|simt_store_only|simt_atomic_only]\n"
        << "       [--mode=simt_store_parallel_seq|simt_store_parallel_scan]\n"
        << "       [--mode=simt_atomic_parallel_seq|simt_atomic_parallel_scan]\n"
        << "       CUST AICPU polling modes require ASCEND_CUSTOM_OPP_PATH to point at "
        << "build/custom_opp/vendors/cust.\n";
}

bool ParseOptions(int argc, char **argv, Options *opt)
{
    for (int i = 1; i < argc; ++i) {
        if (ParseI32(argv[i], "--device", &opt->device) ||
            ParseU32(argv[i], "--tasks", &opt->tasks) ||
            ParseU32(argv[i], "--iters", &opt->iters) ||
            ParseU32(argv[i], "--warmup", &opt->warmup) ||
            ParseU64(argv[i], "--timeout-ms", &opt->timeoutMs) ||
            ParseU32(argv[i], "--delay-iters", &opt->delayIters) ||
            ParseU32(argv[i], "--simt-threads", &opt->simtThreads) ||
            ParseStr(argv[i], "--mode", &opt->mode)) {
            continue;
        }
        return false;
    }

    const bool validMode = opt->mode == "all" || opt->mode == "aiv_store" ||
        opt->mode == "simt_store" || opt->mode == "simt_atomic" || opt->mode == "aicpu_noop" ||
        opt->mode == "aiv_only" || opt->mode == "simt_store_only" || opt->mode == "simt_atomic_only" ||
        opt->mode == "simt_store_parallel_seq" || opt->mode == "simt_store_parallel_scan" ||
        opt->mode == "simt_atomic_parallel_seq" || opt->mode == "simt_atomic_parallel_scan";
    return validMode && opt->tasks > 0 && opt->iters > 0 && opt->timeoutMs > 0 &&
        opt->simtThreads > 0 && opt->simtThreads <= 2048;
}

bool NeedAivStore(const Options &opt)
{
#if defined(ENABLE_AIV_PRODUCER)
    return opt.mode == "all" || opt.mode == "aiv_store";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtStore(const Options &opt)
{
#if defined(ENABLE_SIMT_STORE_PRODUCER)
    return opt.mode == "all" || opt.mode == "simt_store";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtAtomic(const Options &opt)
{
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
    return opt.mode == "all" || opt.mode == "simt_atomic";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtStoreParallelSeq(const Options &opt)
{
#if defined(ENABLE_SIMT_STORE_PRODUCER)
    return opt.mode == "simt_store_parallel_seq";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtStoreParallelScan(const Options &opt)
{
#if defined(ENABLE_SIMT_STORE_PRODUCER)
    return opt.mode == "simt_store_parallel_scan";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtAtomicParallelSeq(const Options &opt)
{
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
    return opt.mode == "simt_atomic_parallel_seq";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtAtomicParallelScan(const Options &opt)
{
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
    return opt.mode == "simt_atomic_parallel_scan";
#else
    (void)opt;
    return false;
#endif
}

bool NeedAicpuNoop(const Options &opt)
{
    return opt.mode == "aicpu_noop";
}

bool NeedAivOnly(const Options &opt)
{
#if defined(ENABLE_AIV_PRODUCER)
    return opt.mode == "aiv_only";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtStoreOnly(const Options &opt)
{
#if defined(ENABLE_SIMT_STORE_PRODUCER)
    return opt.mode == "simt_store_only";
#else
    (void)opt;
    return false;
#endif
}

bool NeedSimtAtomicOnly(const Options &opt)
{
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
    return opt.mode == "simt_atomic_only";
#else
    (void)opt;
    return false;
#endif
}

bool UsesCustomAicpu(const Options &opt)
{
    return opt.mode == "all" || opt.mode == "aicpu_noop" || opt.mode == "aiv_store" ||
        opt.mode == "simt_store" || opt.mode == "simt_atomic" ||
        opt.mode == "simt_store_parallel_seq" || opt.mode == "simt_store_parallel_scan" ||
        opt.mode == "simt_atomic_parallel_seq" || opt.mode == "simt_atomic_parallel_scan";
}

bool CustomOppPathConfigured()
{
    const char *path = std::getenv("ASCEND_CUSTOM_OPP_PATH");
    return path != nullptr && path[0] != '\0';
}

void DestroyTensorResources(aclTensorDesc **descs, size_t descCount, aclDataBuffer **buffers, size_t bufferCount)
{
    for (size_t i = 0; i < bufferCount; ++i) {
        if (buffers[i] != nullptr) {
            (void)aclDestroyDataBuffer(buffers[i]);
        }
    }
    for (size_t i = 0; i < descCount; ++i) {
        if (descs[i] != nullptr) {
            (void)aclDestroyTensorDesc(descs[i]);
        }
    }
}

void *ConfigPtr(const DeviceBuffers &buf, size_t int64Offset)
{
    return static_cast<void *>(static_cast<uint8_t *>(buf.config) + int64Offset * sizeof(int64_t));
}

int CopyConfig(const DeviceBuffers &buf, size_t int64Offset, const int64_t *values, size_t count)
{
    CHECK_ACL(aclrtMemcpy(ConfigPtr(buf, int64Offset), count * sizeof(int64_t), values,
        count * sizeof(int64_t), ACL_MEMCPY_HOST_TO_DEVICE));
    return 0;
}

int ExecuteCustomAicpu(const char *opName, uint32_t inputCount, aclTensorDesc **inputDescs,
    aclDataBuffer **inputBuffers, uint32_t outputCount, aclTensorDesc **outputDescs,
    aclDataBuffer **outputBuffers, aclTensorDesc **allDescs, size_t descCount,
    aclDataBuffer **allBuffers, size_t bufferCount, aclrtStream stream)
{
    for (size_t i = 0; i < descCount; ++i) {
        if (allDescs[i] == nullptr) {
            DestroyTensorResources(allDescs, descCount, allBuffers, bufferCount);
            return Fail("aclCreateTensorDesc(custom aicpu)", 1, __LINE__);
        }
    }
    for (size_t i = 0; i < bufferCount; ++i) {
        if (allBuffers[i] == nullptr) {
            DestroyTensorResources(allDescs, descCount, allBuffers, bufferCount);
            return Fail("aclCreateDataBuffer(custom aicpu)", 1, __LINE__);
        }
    }

    aclopAttr *attr = aclopCreateAttr();
    if (attr == nullptr) {
        DestroyTensorResources(allDescs, descCount, allBuffers, bufferCount);
        return Fail("aclopCreateAttr(custom aicpu)", 1, __LINE__);
    }
    aclError ret = aclopCompileAndExecute(opName, inputCount, inputDescs, inputBuffers,
        outputCount, outputDescs, outputBuffers, attr, ACL_ENGINE_SYS, ACL_COMPILE_SYS, nullptr, stream);
    (void)aclopDestroyAttr(attr);
    DestroyTensorResources(allDescs, descCount, allBuffers, bufferCount);
    if (ret != ACL_SUCCESS) {
        std::cerr << "custom_aicpu_op=" << opName << std::endl;
        return Fail("aclopCompileAndExecute(custom aicpu)", static_cast<int>(ret), __LINE__);
    }
    return 0;
}

int LaunchAicpuNoop(const DeviceBuffers &buf, aclrtStream stream)
{
    const int64_t configValues[] = {0};
    int ret = CopyConfig(buf, 0, configValues, 1);
    if (ret != 0) return ret;

    int64_t configDims[] = {1};
    int64_t statusDims[] = {1};
    aclTensorDesc *inputDescs[] = {aclCreateTensorDesc(ACL_INT64, 1, configDims, ACL_FORMAT_ND)};
    aclTensorDesc *outputDescs[] = {aclCreateTensorDesc(ACL_INT32, 1, statusDims, ACL_FORMAT_ND)};
    aclDataBuffer *inputBuffers[] = {aclCreateDataBuffer(ConfigPtr(buf, 0), sizeof(int64_t))};
    aclDataBuffer *outputBuffers[] = {aclCreateDataBuffer(buf.status, sizeof(uint32_t))};
    aclTensorDesc *allDescs[] = {inputDescs[0], outputDescs[0]};
    aclDataBuffer *allBuffers[] = {inputBuffers[0], outputBuffers[0]};
    return ExecuteCustomAicpu(kAicpuNoopOpName, 1, inputDescs, inputBuffers, 1, outputDescs, outputBuffers,
        allDescs, 2, allBuffers, 2, stream);
}

int LaunchAicpuPollOp(const char *opName, const Options &opt, const DeviceBuffers &buf, aclrtStream stream)
{
    const int64_t configValues[] = {static_cast<int64_t>(opt.tasks), static_cast<int64_t>(kFlagPadCount),
        static_cast<int64_t>(opt.timeoutMs * 1000000ULL)};
    int ret = CopyConfig(buf, 0, configValues, 3);
    if (ret != 0) return ret;

    int64_t flagDims[] = {static_cast<int64_t>(opt.tasks), static_cast<int64_t>(kFlagPadCount)};
    int64_t configDims[] = {3};
    int64_t vectorDims[] = {static_cast<int64_t>(opt.tasks)};
    int64_t statusDims[] = {1};
    aclTensorDesc *inputDescs[] = {
        aclCreateTensorDesc(ACL_INT32, 2, flagDims, ACL_FORMAT_ND),
        aclCreateTensorDesc(ACL_INT64, 1, configDims, ACL_FORMAT_ND),
    };
    aclTensorDesc *outputDescs[] = {
        aclCreateTensorDesc(ACL_INT64, 1, vectorDims, ACL_FORMAT_ND),
        aclCreateTensorDesc(ACL_INT64, 1, vectorDims, ACL_FORMAT_ND),
        aclCreateTensorDesc(ACL_INT32, 1, statusDims, ACL_FORMAT_ND),
    };
    aclDataBuffer *inputBuffers[] = {
        aclCreateDataBuffer(buf.flags, buf.flagsBytes),
        aclCreateDataBuffer(ConfigPtr(buf, 0), 3 * sizeof(int64_t)),
    };
    aclDataBuffer *outputBuffers[] = {
        aclCreateDataBuffer(buf.seenNs, buf.timeBytes),
        aclCreateDataBuffer(buf.pollIters, buf.timeBytes),
        aclCreateDataBuffer(buf.status, sizeof(uint32_t)),
    };
    aclTensorDesc *allDescs[] = {inputDescs[0], inputDescs[1], outputDescs[0], outputDescs[1], outputDescs[2]};
    aclDataBuffer *allBuffers[] = {inputBuffers[0], inputBuffers[1], outputBuffers[0], outputBuffers[1],
        outputBuffers[2]};
    return ExecuteCustomAicpu(opName, 2, inputDescs, inputBuffers, 3, outputDescs, outputBuffers,
        allDescs, 5, allBuffers, 5, stream);
}

int LaunchAicpuPollFlags(const Options &opt, const DeviceBuffers &buf, aclrtStream stream)
{
    return LaunchAicpuPollOp(kAicpuPollFlagsOpName, opt, buf, stream);
}

int LaunchAicpuPollFlagsScan(const Options &opt, const DeviceBuffers &buf, aclrtStream stream)
{
    return LaunchAicpuPollOp(kAicpuPollFlagsScanOpName, opt, buf, stream);
}

int InitBuffers(const Options &opt, DeviceBuffers *buf)
{
    buf->flagsBytes = static_cast<size_t>(opt.tasks) * kFlagPadCount * sizeof(uint32_t);
    buf->timeBytes = static_cast<size_t>(opt.tasks) * sizeof(uint64_t);
    buf->configBytes = 3 * sizeof(int64_t);

    CHECK_ACL(aclrtMalloc(&buf->flags, buf->flagsBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc(&buf->seenNs, buf->timeBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc(&buf->pollIters, buf->timeBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc(&buf->status, sizeof(uint32_t), ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc(&buf->config, buf->configBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    return 0;
}

void FreeBuffers(DeviceBuffers *buf)
{
    if (buf->flags != nullptr) (void)aclrtFree(buf->flags);
    if (buf->seenNs != nullptr) (void)aclrtFree(buf->seenNs);
    if (buf->pollIters != nullptr) (void)aclrtFree(buf->pollIters);
    if (buf->status != nullptr) (void)aclrtFree(buf->status);
    if (buf->config != nullptr) (void)aclrtFree(buf->config);
}

int ResetResultBuffers(const DeviceBuffers &buf)
{
    CHECK_ACL(aclrtMemset(buf.flags, buf.flagsBytes, 0, buf.flagsBytes));
    CHECK_ACL(aclrtMemset(buf.seenNs, buf.timeBytes, 0, buf.timeBytes));
    CHECK_ACL(aclrtMemset(buf.pollIters, buf.timeBytes, 0, buf.timeBytes));
    CHECK_ACL(aclrtMemset(buf.status, sizeof(uint32_t), 0, sizeof(uint32_t)));
    return 0;
}

int CopyResult(const Options &opt, const DeviceBuffers &buf, double hostUs, RunResult *result)
{
    result->hostTotalUs = hostUs;
    result->seenNs.assign(opt.tasks, 0);
    result->pollIters.assign(opt.tasks, 0);
    CHECK_ACL(aclrtMemcpy(result->seenNs.data(), buf.timeBytes, buf.seenNs, buf.timeBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(result->pollIters.data(), buf.timeBytes, buf.pollIters, buf.timeBytes,
        ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(&result->status, sizeof(uint32_t), buf.status, sizeof(uint32_t),
        ACL_MEMCPY_DEVICE_TO_HOST));
    return 0;
}

int RunAicpuNoop(const DeviceBuffers &buf, aclrtStream producerStream, aclrtStream aicpuStream, double *hostUs)
{
    CHECK_ACL(aclrtSynchronizeStream(producerStream));
    CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
    CHECK_ACL(aclrtMemset(buf.status, sizeof(uint32_t), 0, sizeof(uint32_t)));

    const uint64_t start = HostNowNs();
    int ret = LaunchAicpuNoop(buf, aicpuStream);
    if (ret != 0) return ret;
    CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
    const uint64_t end = HostNowNs();
    *hostUs = static_cast<double>(end - start) / 1000.0;
    return 0;
}

enum class PollKind {
    Seq,
    Scan,
};

enum class ProducerKind {
    AivStore,
    SimtStore,
    SimtStoreParallel,
    SimtAtomic,
    SimtAtomicParallel,
};

int LaunchProducer(const Options &opt, const DeviceBuffers &buf, ProducerKind kind, aclrtStream producerStream)
{
    if (kind == ProducerKind::AivStore) {
#if defined(ENABLE_AIV_PRODUCER)
        int ret = LaunchAivWriteFlags(buf.flags, opt.tasks, opt.delayIters, producerStream);
        if (ret != 0) return ret;
#else
        return Fail("AIV producer is not compiled in this binary", 1, __LINE__);
#endif
    } else if (kind == ProducerKind::SimtStore) {
#if defined(ENABLE_SIMT_STORE_PRODUCER)
        int ret = LaunchSimtWriteFlagsStore(buf.flags, opt.tasks, opt.delayIters, opt.simtThreads, producerStream);
        if (ret != 0) return ret;
#else
        return Fail("SIMT store producer is not compiled in this binary", 1, __LINE__);
#endif
    } else if (kind == ProducerKind::SimtStoreParallel) {
#if defined(ENABLE_SIMT_STORE_PRODUCER)
        int ret = LaunchSimtWriteFlagsStoreParallel(buf.flags, opt.tasks, opt.delayIters, opt.simtThreads,
            producerStream);
        if (ret != 0) return ret;
#else
        return Fail("SIMT store parallel producer is not compiled in this binary", 1, __LINE__);
#endif
    } else if (kind == ProducerKind::SimtAtomic) {
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
        int ret = LaunchSimtWriteFlagsAtomic(buf.flags, opt.tasks, opt.delayIters, opt.simtThreads, producerStream);
        if (ret != 0) return ret;
#else
        return Fail("SIMT atomic producer is not compiled in this binary", 1, __LINE__);
#endif
    } else {
#if defined(ENABLE_SIMT_ATOMIC_PRODUCER)
        int ret = LaunchSimtWriteFlagsAtomicParallel(buf.flags, opt.tasks, opt.delayIters, opt.simtThreads,
            producerStream);
        if (ret != 0) return ret;
#else
        return Fail("SIMT atomic parallel producer is not compiled in this binary", 1, __LINE__);
#endif
    }
    return 0;
}

int RunProducerWithPoll(const Options &opt, const DeviceBuffers &buf, ProducerKind kind, PollKind pollKind,
    aclrtStream producerStream, aclrtStream aicpuStream, RunResult *result)
{
    CHECK_ACL(aclrtSynchronizeStream(producerStream));
    CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
    int ret = ResetResultBuffers(buf);
    if (ret != 0) return ret;

    const uint64_t start = HostNowNs();
    ret = (pollKind == PollKind::Scan) ? LaunchAicpuPollFlagsScan(opt, buf, aicpuStream) :
        LaunchAicpuPollFlags(opt, buf, aicpuStream);
    if (ret != 0) return ret;

    ret = LaunchProducer(opt, buf, kind, producerStream);
    if (ret != 0) return ret;

    CHECK_ACL(aclrtSynchronizeStream(producerStream));
    CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
    const uint64_t end = HostNowNs();
    return CopyResult(opt, buf, static_cast<double>(end - start) / 1000.0, result);
}

int RunProducerOnly(const Options &opt, const DeviceBuffers &buf, ProducerKind kind,
    aclrtStream producerStream, double *hostUs)
{
    CHECK_ACL(aclrtSynchronizeStream(producerStream));
    CHECK_ACL(aclrtMemset(buf.flags, buf.flagsBytes, 0, buf.flagsBytes));

    const uint64_t start = HostNowNs();
    int ret = LaunchProducer(opt, buf, kind, producerStream);
    if (ret != 0) return ret;
    CHECK_ACL(aclrtSynchronizeStream(producerStream));
    const uint64_t end = HostNowNs();
    *hostUs = static_cast<double>(end - start) / 1000.0;
    return 0;
}

double Average(const std::vector<double> &values)
{
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double AverageU64(const std::vector<uint64_t> &values)
{
    if (values.empty()) {
        return 0.0;
    }
    long double sum = 0.0;
    for (uint64_t value : values) {
        sum += static_cast<long double>(value);
    }
    return static_cast<double>(sum / static_cast<long double>(values.size()));
}

std::vector<double> SeenIntervalsUs(const RunResult &result)
{
    std::vector<double> intervals;
    for (size_t i = 1; i < result.seenNs.size(); ++i) {
        if (result.seenNs[i] != 0 && result.seenNs[i - 1] != 0 && result.seenNs[i] >= result.seenNs[i - 1]) {
            intervals.push_back(static_cast<double>(result.seenNs[i] - result.seenNs[i - 1]) / 1000.0);
        }
    }
    return intervals;
}

double MinValue(const std::vector<double> &values)
{
    return values.empty() ? 0.0 : *std::min_element(values.begin(), values.end());
}

double MaxValue(const std::vector<double> &values)
{
    return values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
}

double IntervalAverageUs(const RunResult &result)
{
    return Average(SeenIntervalsUs(result));
}

void PrintRun(const char *name, const Options &opt, const RunResult &result)
{
    const auto intervals = SeenIntervalsUs(result);
    const uint64_t firstPoll = result.pollIters.empty() ? 0 : result.pollIters.front();
    const uint64_t lastPoll = result.pollIters.empty() ? 0 : result.pollIters.back();
    std::cout << name
              << " status=" << result.status
              << " host_total_us=" << std::fixed << std::setprecision(2) << result.hostTotalUs
              << " host_per_flag_us=" << (result.hostTotalUs / static_cast<double>(opt.tasks))
              << " aicpu_seen_interval_avg_us=" << Average(intervals)
              << " aicpu_seen_interval_min_us=" << MinValue(intervals)
              << " aicpu_seen_interval_max_us=" << MaxValue(intervals)
              << " poll_iters_avg=" << AverageU64(result.pollIters)
              << " first_poll_iters=" << firstPoll
              << " last_poll_iters=" << lastPoll
              << std::endl;
}

void PrintSummaryValue(const char *name, const std::vector<double> &hostUs, const std::vector<double> &intervalUs)
{
    if (hostUs.empty()) {
        return;
    }
    std::cout << " " << name << "_host_avg_us=" << Average(hostUs)
              << " " << name << "_seen_interval_avg_us=" << Average(intervalUs);
}

void PrintProducerOnlyRun(const char *name, const Options &opt, double hostUs)
{
    std::cout << name
              << " status=0"
              << " host_total_us=" << std::fixed << std::setprecision(2) << hostUs
              << " host_per_flag_us=" << (hostUs / static_cast<double>(opt.tasks))
              << std::endl;
}

} // namespace

int32_t main(int32_t argc, char **argv)
{
    Options opt;
    if (!ParseOptions(argc, argv, &opt)) {
        Usage(argv[0]);
        return 1;
    }

    std::cout << "config:"
              << " device=" << opt.device
              << " tasks=" << opt.tasks
              << " iters=" << opt.iters
              << " warmup=" << opt.warmup
              << " timeout_ms=" << opt.timeoutMs
              << " delay_iters=" << opt.delayIters
              << " simt_threads=" << opt.simtThreads
              << " mode=" << opt.mode
              << std::endl;

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(LogAclDeviceCount());
    CHECK_ACL(aclrtSetDevice(opt.device));

    if (UsesCustomAicpu(opt) && !CustomOppPathConfigured()) {
        std::cerr << "FAIL: CUST AICPU modes require ASCEND_CUSTOM_OPP_PATH to point at "
                  << "build/custom_opp/vendors/cust before launching the benchmark." << std::endl;
        return 1;
    }

    aclrtStream producerStream = nullptr;
    aclrtStream aicpuStream = nullptr;
    CHECK_ACL(aclrtCreateStream(&producerStream));
    if (UsesCustomAicpu(opt)) {
        CHECK_ACL(aclrtCreateStream(&aicpuStream));
    }

    DeviceBuffers buf;
    int ret = InitBuffers(opt, &buf);
    if (ret != 0) return ret;

    std::vector<double> aivHostUs;
    std::vector<double> aivIntervalUs;
    std::vector<double> simtStoreHostUs;
    std::vector<double> simtStoreIntervalUs;
    std::vector<double> simtAtomicHostUs;
    std::vector<double> simtAtomicIntervalUs;
    std::vector<double> simtStoreParallelSeqHostUs;
    std::vector<double> simtStoreParallelSeqIntervalUs;
    std::vector<double> simtStoreParallelScanHostUs;
    std::vector<double> simtStoreParallelScanIntervalUs;
    std::vector<double> simtAtomicParallelSeqHostUs;
    std::vector<double> simtAtomicParallelSeqIntervalUs;
    std::vector<double> simtAtomicParallelScanHostUs;
    std::vector<double> simtAtomicParallelScanIntervalUs;
    std::vector<double> noopUs;
    std::vector<double> aivOnlyHostUs;
    std::vector<double> simtStoreOnlyHostUs;
    std::vector<double> simtAtomicOnlyHostUs;

    RunResult lastAiv;
    RunResult lastSimtStore;
    RunResult lastSimtAtomic;
    const uint32_t totalLoops = opt.warmup + opt.iters;
    for (uint32_t loop = 0; loop < totalLoops; ++loop) {
        RunResult aiv;
        RunResult simtStore;
        RunResult simtAtomic;
        RunResult simtStoreParallelSeq;
        RunResult simtStoreParallelScan;
        RunResult simtAtomicParallelSeq;
        RunResult simtAtomicParallelScan;
        double noop = 0.0;
        double aivOnly = 0.0;
        double simtStoreOnly = 0.0;
        double simtAtomicOnly = 0.0;

        if (NeedAicpuNoop(opt)) {
            ret = RunAicpuNoop(buf, producerStream, aicpuStream, &noop);
            if (ret != 0) return ret;
        }
        if (NeedAivStore(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::AivStore, PollKind::Seq, producerStream, aicpuStream,
                &aiv);
            if (ret != 0) return ret;
        }
        if (NeedSimtStore(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtStore, PollKind::Seq, producerStream, aicpuStream,
                &simtStore);
            if (ret != 0) return ret;
        }
        if (NeedSimtAtomic(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtAtomic, PollKind::Seq, producerStream, aicpuStream,
                &simtAtomic);
            if (ret != 0) return ret;
        }
        if (NeedSimtStoreParallelSeq(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtStoreParallel, PollKind::Seq, producerStream,
                aicpuStream, &simtStoreParallelSeq);
            if (ret != 0) return ret;
        }
        if (NeedSimtStoreParallelScan(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtStoreParallel, PollKind::Scan, producerStream,
                aicpuStream, &simtStoreParallelScan);
            if (ret != 0) return ret;
        }
        if (NeedSimtAtomicParallelSeq(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtAtomicParallel, PollKind::Seq, producerStream,
                aicpuStream, &simtAtomicParallelSeq);
            if (ret != 0) return ret;
        }
        if (NeedSimtAtomicParallelScan(opt)) {
            ret = RunProducerWithPoll(opt, buf, ProducerKind::SimtAtomicParallel, PollKind::Scan, producerStream,
                aicpuStream, &simtAtomicParallelScan);
            if (ret != 0) return ret;
        }
        if (NeedAivOnly(opt)) {
            ret = RunProducerOnly(opt, buf, ProducerKind::AivStore, producerStream, &aivOnly);
            if (ret != 0) return ret;
        }
        if (NeedSimtStoreOnly(opt)) {
            ret = RunProducerOnly(opt, buf, ProducerKind::SimtStore, producerStream, &simtStoreOnly);
            if (ret != 0) return ret;
        }
        if (NeedSimtAtomicOnly(opt)) {
            ret = RunProducerOnly(opt, buf, ProducerKind::SimtAtomic, producerStream, &simtAtomicOnly);
            if (ret != 0) return ret;
        }

        if (loop >= opt.warmup) {
            std::cout << "iter=" << (loop - opt.warmup) << std::endl;
            if (NeedAicpuNoop(opt)) {
                noopUs.push_back(noop);
                std::cout << "aicpu_noop status=0 host_total_us=" << std::fixed << std::setprecision(2) << noop
                          << std::endl;
            }
            if (NeedAivStore(opt)) {
                aivHostUs.push_back(aiv.hostTotalUs);
                aivIntervalUs.push_back(IntervalAverageUs(aiv));
                lastAiv = aiv;
                PrintRun("aiv_store", opt, aiv);
            }
            if (NeedSimtStore(opt)) {
                simtStoreHostUs.push_back(simtStore.hostTotalUs);
                simtStoreIntervalUs.push_back(IntervalAverageUs(simtStore));
                lastSimtStore = simtStore;
                PrintRun("simt_store", opt, simtStore);
            }
            if (NeedSimtAtomic(opt)) {
                simtAtomicHostUs.push_back(simtAtomic.hostTotalUs);
                simtAtomicIntervalUs.push_back(IntervalAverageUs(simtAtomic));
                lastSimtAtomic = simtAtomic;
                PrintRun("simt_atomic", opt, simtAtomic);
            }
            if (NeedSimtStoreParallelSeq(opt)) {
                simtStoreParallelSeqHostUs.push_back(simtStoreParallelSeq.hostTotalUs);
                simtStoreParallelSeqIntervalUs.push_back(IntervalAverageUs(simtStoreParallelSeq));
                PrintRun("simt_store_parallel_seq", opt, simtStoreParallelSeq);
            }
            if (NeedSimtStoreParallelScan(opt)) {
                simtStoreParallelScanHostUs.push_back(simtStoreParallelScan.hostTotalUs);
                simtStoreParallelScanIntervalUs.push_back(IntervalAverageUs(simtStoreParallelScan));
                PrintRun("simt_store_parallel_scan", opt, simtStoreParallelScan);
            }
            if (NeedSimtAtomicParallelSeq(opt)) {
                simtAtomicParallelSeqHostUs.push_back(simtAtomicParallelSeq.hostTotalUs);
                simtAtomicParallelSeqIntervalUs.push_back(IntervalAverageUs(simtAtomicParallelSeq));
                PrintRun("simt_atomic_parallel_seq", opt, simtAtomicParallelSeq);
            }
            if (NeedSimtAtomicParallelScan(opt)) {
                simtAtomicParallelScanHostUs.push_back(simtAtomicParallelScan.hostTotalUs);
                simtAtomicParallelScanIntervalUs.push_back(IntervalAverageUs(simtAtomicParallelScan));
                PrintRun("simt_atomic_parallel_scan", opt, simtAtomicParallelScan);
            }
            if (NeedAivOnly(opt)) {
                aivOnlyHostUs.push_back(aivOnly);
                PrintProducerOnlyRun("aiv_only", opt, aivOnly);
            }
            if (NeedSimtStoreOnly(opt)) {
                simtStoreOnlyHostUs.push_back(simtStoreOnly);
                PrintProducerOnlyRun("simt_store_only", opt, simtStoreOnly);
            }
            if (NeedSimtAtomicOnly(opt)) {
                simtAtomicOnlyHostUs.push_back(simtAtomicOnly);
                PrintProducerOnlyRun("simt_atomic_only", opt, simtAtomicOnly);
            }
        }
    }

    std::cout << "summary:";
    if (!noopUs.empty()) {
        std::cout << " aicpu_noop_avg_us=" << Average(noopUs);
    }
    PrintSummaryValue("aiv_store", aivHostUs, aivIntervalUs);
    PrintSummaryValue("simt_store", simtStoreHostUs, simtStoreIntervalUs);
    PrintSummaryValue("simt_atomic", simtAtomicHostUs, simtAtomicIntervalUs);
    PrintSummaryValue("simt_store_parallel_seq", simtStoreParallelSeqHostUs, simtStoreParallelSeqIntervalUs);
    PrintSummaryValue("simt_store_parallel_scan", simtStoreParallelScanHostUs, simtStoreParallelScanIntervalUs);
    PrintSummaryValue("simt_atomic_parallel_seq", simtAtomicParallelSeqHostUs, simtAtomicParallelSeqIntervalUs);
    PrintSummaryValue("simt_atomic_parallel_scan", simtAtomicParallelScanHostUs, simtAtomicParallelScanIntervalUs);
    if (!aivOnlyHostUs.empty()) {
        std::cout << " aiv_only_host_avg_us=" << Average(aivOnlyHostUs);
    }
    if (!simtStoreOnlyHostUs.empty()) {
        std::cout << " simt_store_only_host_avg_us=" << Average(simtStoreOnlyHostUs);
    }
    if (!simtAtomicOnlyHostUs.empty()) {
        std::cout << " simt_atomic_only_host_avg_us=" << Average(simtAtomicOnlyHostUs);
    }
    if (!aivIntervalUs.empty() && !simtStoreIntervalUs.empty()) {
        const double aivAvg = Average(aivIntervalUs);
        std::cout << " simt_store_over_aiv_seen_interval="
                  << (aivAvg > 0.0 ? Average(simtStoreIntervalUs) / aivAvg : 0.0);
    }
    if (!aivIntervalUs.empty() && !simtAtomicIntervalUs.empty()) {
        const double aivAvg = Average(aivIntervalUs);
        std::cout << " simt_atomic_over_aiv_seen_interval="
                  << (aivAvg > 0.0 ? Average(simtAtomicIntervalUs) / aivAvg : 0.0);
    }
    std::cout << std::endl;

    if (NeedAivStore(opt)) {
        PrintRun("last_aiv_store_detail", opt, lastAiv);
    }
    if (NeedSimtStore(opt)) {
        PrintRun("last_simt_store_detail", opt, lastSimtStore);
    }
    if (NeedSimtAtomic(opt)) {
        PrintRun("last_simt_atomic_detail", opt, lastSimtAtomic);
    }

    FreeBuffers(&buf);
    if (producerStream != nullptr) (void)aclrtDestroyStream(producerStream);
    if (aicpuStream != nullptr) (void)aclrtDestroyStream(aicpuStream);
    (void)aclrtResetDevice(opt.device);
    (void)aclFinalize();
    return 0;
}
