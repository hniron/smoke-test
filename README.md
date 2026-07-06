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
Host 用 aclrtMallocHost 申请一块 host memory：flagHost
  -> 按 4KB 对齐后的 registered_bytes 调用 aclrtHostRegister(flagHost, registered_bytes, ACL_HOST_REGISTER_MAPPED, &flagDev)
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
flagHost  : aclrtMallocHost 申请的 Host memory，Host CPU 轮询
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

当前主要 mode：

```text
host_poll_plain
  AIV 先做一段 float workload，再写 mapped host flag，Host CPU 在线轮询。
  这个 mode 用来验证中途可见性，不适合当成纯 flag latency。

host_poll_postcheck
  Host 不在线轮询，只在 AIV stream synchronize 后检查 flagHost。
  用来判断 AIV 写 mapped host memory 是否至少最终可见。

flag_latency
  AIV 不再处理 float 数据，只按 event 序列写多个独立 host flag slot。
  Host CPU 在线轮询这些 slot，记录每个 flag 被看到的 Host 时间。

delay_calib
  AIV 只跑和 flag_latency 相同的 delay loop，不写 host flag。
  用来标定某个 --delay-iters 大概带来多少 AIV 侧间隔。

aiv_snoop
  空 AIV kernel launch baseline。

all
  当前只组合 host_poll_plain + host_poll_postcheck，不自动包含 flag_latency/delay_calib。
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
4. Host flag 使用 `aclrtMallocHost` 申请，避免普通 malloc/posix 内存被 runtime/driver 识别为非法 Host 注册内存。
5. `aclrtHostRegister` 的注册区间使用按 4KB 向上对齐后的 `registered_bytes`，避免只注册 256B 这类小区间导致 runtime/driver 报 invalid argument。
6. 如果平台要求 flush/invalidate，需要在后续版本加对应 runtime API 或平台同步原语。

`volatile` 或 `std::atomic` 只能约束 Host CPU 侧的编译器和 CPU load 行为，不能单独保证 device write 一定让 CPU cache 立刻可见。

## 计时口径

`host_poll_plain` 保留原来的 Host 侧计时：

```text
host_total_us:
  从 Host 准备启动 poller / launch AIV 开始
  到 Host poller 看到所有 flag 且 AIV stream 完成结束

host_seen_interval_avg_us:
  Host CPU 连续看到两个 task flag 的时间间隔平均值
```

这里的 `host_seen_interval_avg_us` 包含每个 task 前面的 float DataCopy/Add/DataCopyOut 时间，所以它证明中途可见性，但不是纯 AIV 写 flag -> Host CPU 读 flag 的时延。

`flag_latency` 的计时口径更干净：

```text
AIV:
  for event in events:
      AivDelay(delay_iters)
      写 flagDev[event * 64B] = event + 1

Host CPU:
  kernel launch 前启动 poller
  依次轮询 flagHost[event * 64B]
  每看到一个 event flag 就记录 HostNowNs()
```

`flag_latency` 输出里的 `seen_interval_avg_us` / `seen_interval_min_us` / `seen_interval_max_us` 是 Host CPU 看到相邻 event flag 的时间间隔。它比 `host_poll_plain` 更接近 flag 可见性开销，但仍然不是纯硬件单次 store-to-load latency，因为 Host 只能记录“什么时候看到”，不知道 AIV 精确哪一拍发出写。

`delay_calib` 用来估算人工 delay：

```text
T0 = delay_calib(events=N, delay_iters=0)
TD = delay_calib(events=N, delay_iters=D)
人工 delay 每个 event 的额外时间 ~= (TD - T0) / N
```

后续分析可以用：

```text
flag_latency_seen_interval_avg_us - delay_calib_extra_per_event_us
```

这个差值只能理解成“去掉人工 delay 后的剩余观测开销估计”，里面仍包含 AIV 写 host registered memory、Host 可见性路径、Host polling 采样误差、flag 写循环本身开销。

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
flag_latency
delay_calib
aiv_snoop
all  # 只组合 host_poll_plain + host_poll_postcheck
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

### AIV 写 Host flag latency 测试

`flag_latency` 不再分配和处理 x/y/z float 数据，只分配 host registered flag buffer。每个 event 使用一个独立 64B flag slot，AIV 写 `event + 1`，Host CPU 轮询对应 slot。

先从无 delay 开始跑：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=flag_latency \
  --events=256 \
  --delay-iters=0 \
  --warmup=2 \
  --iters=10 \
  --timeout-ms=5000
```

重点看这些字段：

```text
seen_interval_avg_us / seen_interval_min_us / seen_interval_max_us
  Host 看到相邻两个 event flag 的间隔。

one_poll_count
  有多少个 event 是 Host 第一次读这个 slot 就已经看到新值。
  如果这个值接近 events，说明 AIV 写得太快，Host 很可能是在事后扫到一批已经写好的 flag。

poll_iters_avg
  Host 每个 event 平均轮询多少次才看到 flag。
  太接近 1 时通常说明事件堆积，delay 不够。

first_seen_us
  第一个 flag 被看到的时间，包含 kernel launch 和调度开销，不要当成 AIV->Host 单次时延。
```

做 delay sweep，找到 Host 能稳定逐个捕捉 event 的最小 delay：

```bash
for d in 0 64 256 1024 4096; do
  ./build-a5/aiv_hostcpu_bench \
    --device=0 \
    --mode=flag_latency \
    --events=256 \
    --delay-iters=${d} \
    --warmup=2 \
    --iters=10 \
    --timeout-ms=5000
done
```

判断标准不是 delay 越大越好，而是找最小的稳定档位：`seen_interval_min_us` 不再大量接近 0，`one_poll_count` 不再接近 `events`，`seen_interval_avg_us` 在多次迭代中比较稳定。

标定 delay loop：

```bash
for d in 0 64 256 1024 4096; do
  ./build-a5/aiv_hostcpu_bench \
    --device=0 \
    --mode=delay_calib \
    --events=4096 \
    --delay-iters=${d} \
    --warmup=2 \
    --iters=10
done
```

这里 `delay_calib_per_event_avg_us` 仍然包含少量 kernel launch/sync 均摊开销。更推荐用差分：

```text
delay_extra_per_event_us(D) ~= delay_calib_per_event_avg_us(D) - delay_calib_per_event_avg_us(0)
```

然后再估算：

```text
residual_us ~= flag_latency_seen_interval_avg_us(D) - delay_extra_per_event_us(D)
```

这个 `residual_us` 是当前软件观测方法下的剩余可见性开销估计，不是严格的硬件单次 AIV store 到 Host CPU load latency。

如果怀疑平台需要显式 cache 维护，可以尝试：

```bash
--flush-after-reset=1
--invalidate-before-read=1
```

其中 `--invalidate-before-read=1` 只影响 `host_poll_postcheck` 的最终检查，不解决 `host_poll_plain` 在线轮询中的非一致性 cache 问题。在线轮询成立的前提仍然是 mapped host memory 对 Host CPU 读侧具备正确可见性。
