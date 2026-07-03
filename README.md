# AIV Host CPU Flag Benchmark 设计说明

## 目标

这个工程用来验证一条新的细粒度同步路径：

```text
AIV kernel 中途写 flag
  -> flag 不放在 device HBM
  -> flag 放在 Host DRAM
  -> Host CPU 轮询这个 flag
```

它要替代 `aiv_aicpu_bench` 里的这条路径：

```text
AIV 写 HBM/GM flag
  -> AICPU kernel 轮询 HBM/GM flag
```

新的实验重点不是先追求性能，而是先证明三件事：

1. Host DRAM 是否能注册成 AIV 可访问的 device 地址。
2. AIV 是否能在 kernel 执行中途把 flag 写到这块 Host DRAM。
3. Host CPU 是否能在 AIV kernel 尚未结束时看到 flag 变化。

## 当前理解

`aclrtMalloc` 分配的是 device 侧内存。Host 拿到的指针是 device 地址，通常背后是 HBM/GM。这个地址可以传给 AIV kernel，用 `GlobalTensor` 或 `DataCopy` 访问；但 Host CPU 不能像普通 `malloc` 指针一样直接解引用它。

普通 `malloc` 指针背后是 Host 进程虚拟地址，对应 Host DRAM，CPU 可以直接 load/store。

当前 `aiv_aicpu_bench` 里的 `buf.flags` 是 `aclrtMalloc` 出来的 device memory，所以路径是：

```text
Host aclrtMalloc flags
  -> flags 位于 device GM/HBM 地址空间
  -> AIV 写 flags
  -> AICPU 读 flags
  -> Host 最后通过 aclrtMemcpy 把结果 copy 回来
```

如果让 Host CPU 直接读这个 `buf.flags`，是不成立的。Host CPU 的普通 load/store 路径里没有这个 device HBM 地址的有效映射。

## 新路径

新工程要把 flag 放到 Host DRAM，然后让 device 也能访问这块 Host DRAM。

大致流程：

```text
Host 申请一块页对齐的 host memory：flagHost
  -> aclrtHostRegister(flagHost, size, ACL_HOST_REGISTER_MAPPED, &flagDev)
  -> flagHost 是 Host CPU 使用的地址
  -> flagDev 是 AIV 使用的 device 地址
```

两者关系：

```text
flagHost：CPU 视角下的地址
flagDev ：NPU/AIV 视角下的地址
底层物理页：同一块 Host DRAM
```

AIV kernel 里不能传 `flagHost`，要传 `flagDev`。Host CPU 轮询时不能读 `flagDev`，要读 `flagHost`。

## 物理路径

AIV 写 `flagDev` 时，理想路径是：

```text
AIV Core
  -> device 侧 GM store / DataCopy 写请求
  -> Ascend 片上互连 / memory system
  -> 地址翻译单元 / SMMU / IOMMU
  -> 判断目标不是本地 HBM，而是 mapped host memory
  -> PCIe / C2C / host-device interconnect
  -> Host memory controller
  -> Host DRAM
```

`aclrtHostRegister` 的作用不是把 Host DRAM 搬到 device，而是 pin 住 Host DRAM 页面，并建立 device 地址到这些 Host 物理页的映射。

如果平台是 PCIe 形态，可以理解为 device 发起 outbound memory write。Host CPU 不主动 copy，device 自己把数据写到 Host 侧内存。

## 最小实现方案

先做一个单文件主程序和一个 AIV kernel，结构参考 `aiv_aicpu_bench/bench_main.asc`，但不引入 AICPU 自定义 op。

核心 buffer：

```text
x/y/z     : aclrtMalloc，仍放 device HBM，用来制造 AIV stage 计算时间
flagHost  : page-aligned host memory，Host CPU 轮询
flagDev   : aclrtHostRegister 返回的 device-visible address，AIV 写入
```

AIV kernel：

```text
for task in tasks:
    做一段 DataCopy/Add/DataCopyOut，模拟 stage 计算
    写 flagDev[task * flagStride] = task + 1
```

Host poller：

```text
for task in tasks:
    while flagHost[task * flagStride] < task + 1:
        检查 timeout
    记录 HostNowNs()
```

注意：Host poller 要在 AIV kernel launch 之前启动，避免错过早期 flag。

## 计划支持的 mode

先设计三个 mode：

```text
host_poll_plain
  AIV 写 mapped host flag，Host CPU 在线轮询。

host_poll_postcheck
  Host 不在线轮询，只在 AIV stream synchronize 后检查 flagHost。
  用来判断 AIV 写 mapped host memory 是否至少最终可见。

aiv_snoop
  空 AIV kernel launch baseline。
```

后续如果需要，再加：

```text
host_poll_memcpy_baseline
  AIV 写 HBM flag，Host 周期性 D2H copy 后检查。
  用来对比 mapped host flag 是否真的减少 copy 路径。
```

## 结果解释

`host_poll_plain` 成功：

```text
说明 AIV 写 mapped Host DRAM 后，Host CPU 能在 kernel 中途看到 flag。
```

`host_poll_plain` 超时，但 `host_poll_postcheck` 成功：

```text
说明 AIV 最终能写到 Host DRAM，但中途轮询可见性有问题。
重点查 CPU cache coherency、mapping 属性、flush/invalidate。
```

`host_poll_postcheck` 也失败：

```text
说明 AIV 写 mapped host memory 这条路径本身没打通。
重点查 aclrtHostRegister 是否成功、flagDev 是否能作为 AIV GM 地址使用、平台是否支持该映射。
```

## Cache 一致性风险

这个实验最大的风险不是写代码，而是 cache 可见性。

Host CPU 轮询 `flagHost` 时，可能一直读到自己 cache 里的旧值。AIV 已经通过 device outbound write 把新值写到 Host DRAM，但如果映射不是 coherent 的，CPU 可能不会立刻看到。

所以代码里需要：

1. flag 每个 task 独占 cache line，建议 `flagStride = 16` 个 `uint32_t`，即 64B。
2. Host 轮询使用 `volatile` 或 atomic load，避免编译器把 load 优化掉。
3. 每轮开始前清零 `flagHost`。
4. `aclrtHostRegister` 的注册区间使用按 4KB 向上对齐后的 `registered_bytes`，避免只注册 256B 这类小区间导致 runtime/driver 报 invalid argument。
5. 如果平台要求 flush/invalidate，需要在后续版本加对应 runtime API 或平台同步原语。

`volatile` 或 `std::atomic` 只能约束 Host CPU 侧的编译器和 CPU load 行为，不能单独保证 device write 一定让 CPU cache 立刻可见。

## 计时口径

初版保留 Host 侧计时：

```text
host_total_us:
  从 Host 准备启动 poller / launch AIV 开始
  到 Host poller 看到所有 flag 且 AIV stream 完成结束

host_seen_interval_avg_us:
  Host CPU 连续看到两个 flag 的时间间隔平均值
```

这个指标不是裸 PCIe write latency，也不是单次 cache miss latency。它是整条工程路径的端到端观测值。

## 构建方向

工程先保留独立目录：

```text
aiv_hostcpu_bench/
  README.md
  CMakeLists.txt
  bench_main.asc
```

不复用 `aiv_aicpu_bench` 的 AICPU 自定义 op packaging。这样可以先把问题缩小到：

```text
AIV kernel + Host mapped memory + Host CPU poller
```

后续如果这条路径成立，再补充更严格的 benchmark 参数、统计输出和对比 mode。


## 当前实现

当前代码已经实现：

```text
CMakeLists.txt
bench_main.asc
```

支持 mode：

```text
host_poll_plain
host_poll_postcheck
aiv_snoop
all
```

### A5 环境下的构建与运行指令

下面这组命令是 A5/CANN 9.x 测试环境下的运行指令，参考 `simt_test` 已验证过的路径和参数：

```bash
cd /home/allen/workdir/zhn/aiv_hostcpu_bench
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5
cmake -S . -B build-a5 \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build-a5 -j
```

如果当前 shell 里 `ASCEND_HOME_PATH` 或 `CANN_INSTALL_PATH` 已经指向实际 CANN 根目录，也可以把 `-DCANN_INSTALL_PATH=...` 改成 `${ASCEND_HOME_PATH}` 或 `${CANN_INSTALL_PATH}`。不要使用 `/path/to/cann/set_env.sh` 这种占位路径。

运行 `host_poll_plain`，验证 AIV 写 mapped Host DRAM 后 Host CPU 是否能在线轮询看到 flag：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=5 \
  --timeout-ms=5000 \
  --mode=host_poll_plain
```

如果只想确认 AIV 最终是否写到了 mapped host memory，先跑 `host_poll_postcheck`：

```bash
./build-a5/aiv_hostcpu_bench --device=0 --warmup=0 --iters=1 --mode=host_poll_postcheck
```

如果怀疑平台需要显式 cache 维护，可以尝试：

```bash
--flush-after-reset=1
--invalidate-before-read=1
```

其中 `--invalidate-before-read=1` 只影响 `host_poll_postcheck` 的最终检查，不解决 `host_poll_plain` 在线轮询中的非一致性 cache 问题。在线轮询成立的前提仍然是 mapped host memory 对 Host CPU 读侧具备正确可见性。
