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

host_aiv_roundtrip
  Host CPU 写 request flag 到 mapped host memory，AIV 在线轮询 request，
  AIV 看到后写 ack flag，Host CPU 再轮询 ack。现在同时返回每个 event 的
  AIV 设备时间戳，用于拟合 Host/AIV 时钟映射和后续单向时延测试。

aiv_host_paced_timestamp
  先自动执行 roundtrip 时钟校准，再由 AIV 按 delay-iters 间隔写设备时间戳和 ready flag，
  Host CPU 轮询 ready flag 后计算 AIV->Host 单向观测时延。

host_aiv_roundtrip_fast
  去掉 host_aiv_roundtrip 热路径里的诊断统计和每轮 timeout/status 检查，
  只保留 Host 写 req、AIV 读 req、AIV 写 ack、Host 读 ack。
  这个 mode 用来更接近最瘦 roundtrip 开销。

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

## 主对比实验口径

当前主实验不是测纯 flag latency，而是对比同一段 AIV workload 下两条 flag 可见路径的阶段间隔。

对比基线是 `aiv_aicpu_bench --mode=shared`：

```text
AIV float DataCopy/Add/DataCopyOut
  -> AIV 写 HBM/GM flag
  -> AICPU 轮询 HBM/GM flag
```

本工程对应使用 `aiv_hostcpu_bench --mode=host_poll_plain`：

```text
AIV float DataCopy/Add/DataCopyOut
  -> AIV 写 Host registered memory flag
  -> Host CPU 轮询 Host DRAM flag
```

为了让这个对比成立，`host_poll_plain` 的 AIV `RunStage` 和 `aiv_aicpu_bench` 的 `shared` 路径保持同样结构：

```text
for task in tasks:
    for repeat in repeat:
        for offset in elements / tile:
            DataCopy x
            DataCopy y
            Add z = x + y
            DataCopy z
    PipeBarrier
    写 flag
```

默认参数也对齐：`tasks=4`、`elements=262144`、`tile=1024`、`repeat=1`。主对比时两边必须使用相同参数。

flag 写入大小也对齐：`aiv_aicpu_bench` 使用 `kFlagPadCount=16`，本工程 `host_poll_plain` 使用 `flag_stride_words=16`，也就是每个 task 写 16 个 `uint32_t`、64B。这样 AIV 每次 flag `DataCopy` 的数据量一致，并且每个 flag slot 独占一条常见 64B Host CPU cache line；差异主要来自 flag 目标位置和轮询侧不同。

主对比字段：

```text
aiv_aicpu_bench shared:
  aicpu_seen_interval_avg_us

aiv_hostcpu_bench host_poll_plain:
  host_seen_interval_avg_us
```

二者都不是纯 flag latency，二者都包含同样的 AIV float add 阶段。可以近似理解为：

```text
aicpu_seen_interval_avg_us
  ~= AIV float add 时间
   + AIV 写 HBM/GM flag 时间
   + AICPU 轮询采样误差

host_seen_interval_avg_us
  ~= AIV float add 时间
   + AIV 写 Host DRAM flag 时间
   + Host CPU 轮询采样误差
```

所以这个实验要看的不是单次 store/load 物理延迟，而是同 workload 下 `HBM + AICPU poll` 与 `Host DRAM + Host CPU poll` 两条工程路径的阶段间隔差异。

## Cache 一致性风险

这个实验最大的风险不是写代码，而是 cache 可见性。

Host CPU 轮询 `flagHost` 时，可能一直读到自己 cache 里的旧值。AIV 已经通过 device outbound write 把新值写到 Host DRAM，但如果映射不是 coherent 的，CPU 可能不会立刻看到。

所以代码里需要：

1. 主对比实验使用 `flagStride = 16` 个 `uint32_t`，即 64B，并同步修改 `aiv_aicpu_bench` 的 `kFlagPadCount=16`。这样两边 AIV 每次 flag `DataCopy` 大小一致，同时每个 flag slot 独占一条常见 Host CPU cache line，减少不同 flag 共享同一 cache line 的干扰。
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

这里的 `host_seen_interval_avg_us` 包含每个 task 前面的 float DataCopy/Add/DataCopyOut 时间。这个口径是主对比实验需要的：它和 `aiv_aicpu_bench` 的 `aicpu_seen_interval_avg_us` 都包含同样的 AIV float workload。

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
host_aiv_roundtrip
host_aiv_roundtrip_fast
aiv_host_paced_timestamp
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

主对比实验要用同一组参数分别跑 `aiv_aicpu_bench shared` 和本工程 `host_poll_plain`。

`aiv_aicpu_bench` 侧，按该工程 README 构建并配置 CUST AICPU 后，在 build 目录运行：

```bash
cd /home/allen/workdir/zhn/aiv_aicpu_bench/build
export ASCEND_CUSTOM_OPP_PATH=${PWD}/custom_opp/vendors/cust
export LD_LIBRARY_PATH=${ASCEND_CUSTOM_OPP_PATH}/op_proto/lib/linux/$(uname -m):${ASCEND_CUSTOM_OPP_PATH}/op_impl/cpu/aicpu_kernel/impl:${LD_LIBRARY_PATH}

./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=5 \
  --timeout-ms=5000 \
  --mode=shared
```

`aiv_hostcpu_bench` 侧，使用同样参数运行：

```bash
cd /home/allen/workdir/zhn/aiv_hostcpu_bench
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

主对比看这两个字段：

```text
aiv_aicpu_bench shared:          aicpu_seen_interval_avg_us
aiv_hostcpu_bench host_poll_plain: host_seen_interval_avg_us
```

两边都包含同样的 AIV float add workload 和同样 64B flag write。差异主要来自 `HBM/GM + AICPU poll` 和 `Host DRAM + Host CPU poll` 两条路径。64B padding 只减少 cache-line sharing 干扰，不代表平台一定提供 Host cache coherent 可见性。

如果只想确认 AIV 最终是否写到了 mapped host memory，先跑 `host_poll_postcheck`：

```bash
./build-a5/aiv_hostcpu_bench --device=0 --warmup=0 --iters=1 --mode=host_poll_postcheck
```

### Host/AIV 双向 roundtrip 测试

`host_aiv_roundtrip` 对应老师提到的双向 flag 同步思路：

```text
Host CPU:
  等 AIV 写 ready
  对每个 event:
      写 cpu_req = seq
      while aiv_ack < seq:
          轮询 host memory

AIV:
  写 ready = 1
  对每个 event:
      while cpu_req < seq:
          从 mapped host memory 读 request
      写 aiv_ack = seq
```

这个 mode 不做 float add，也不加人工 delay。它不是主对比实验，而是单独验证：

```text
Host CPU 写 Host DRAM flag
  -> AIV 通过 mapped device address 读到
  -> AIV 写 Host DRAM ack flag
  -> Host CPU 读到 ack
```

先用很小的 event 数验证路径是否打通：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_aiv_roundtrip \
  --events=1 \
  --warmup=0 \
  --iters=1 \
  --timeout-ms=5000 \
  --aiv-poll-limit=1000000
```

如果 `status=0` 且 `ready=1 final_req=1 final_ack=1 final_status=0 abort=0`，说明最小 CPU->AIV->CPU 闭环成功。然后再扩大 event 数：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_aiv_roundtrip \
  --events=256 \
  --warmup=2 \
  --iters=10 \
  --timeout-ms=5000 \
  --aiv-poll-limit=1000000
```

重点看这些字段：

```text
roundtrip_avg_us / roundtrip_min_us / roundtrip_max_us
  Host 写 cpu_req 到 Host 看到 aiv_ack 的时间。
  它包含 Host store 可见到 AIV、AIV polling 采样误差、AIV 写 ack、
  Host polling 采样误差，不是单向 AIV->Host latency。

host_poll_iters_avg
  Host 每个 event 平均轮询多少次才看到 AIV ack。

aiv_poll_iters_avg
  AIV 每个 event 平均轮询多少次才看到 Host request。

ready / final_req / final_ack / final_status / abort
  控制 flag 的最终值。`final_req == final_ack == events` 通常说明所有 event 都完成。
```

如果出现 `status=2`、`final_ack < final_req` 或 `aiv_poll_iters_avg` 接近 `--aiv-poll-limit`，优先判断 CPU 写 Host DRAM 后 AIV 在线读取可见性不成立，或者需要平台提供额外的 cache flush / invalidate / coherent mapping 语义。这个 mode 里保留 `--aiv-poll-limit`，就是为了避免 AIV 一直轮询看不到 Host request 时整条 stream 卡死。

### Roundtrip 时钟校准和 AIV->Host 单向观测时延

`host_aiv_roundtrip` 现在除了原来的 request/ack flag 外，还会让 AIV 为每个 event 记录两个设备侧时间戳：

```text
D2: AIV 看到 Host request 后的 GetSystemCycle()
D3: AIV 写 response 前的 GetSystemCycle()
```

Host 侧同时记录：

```text
H1: Host 写 request 前的 HostNowNs()
H4: Host 看到 response 后的 HostNowNs()
```

每个样本用中点近似建立两个时钟的映射：

```text
Dmid = (D2 + D3) / 2
Hmid = (H1 + H4) / 2

Hmid ~= scale * Dmid + offset
```

程序使用所有成功样本做线性拟合，并输出：

```text
clock_valid
clock_samples
clock_scale_ns_per_cycle
clock_offset_ns
clock_residual_p50_us
clock_residual_p99_us
```

其中 `clock_residual_*` 是拟合残差，不是 AIV->Host 时延。Roundtrip 假设 Host->AIV 和 AIV->Host 的路径延迟大致对称，因此它是工程上的近似校准；如果需要严格无偏的单向硬件时延，需要平台提供 Host/AIV 同步时钟或硬件时间戳。

建议先跑 roundtrip 校准：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_aiv_roundtrip \
  --events=256 \
  --warmup=2 \
  --iters=10 \
  --timeout-ms=5000 \
  --aiv-poll-limit=1000000
```

要求：

```text
status=0
final_req == final_ack == events
clock_valid=1
clock_samples 接近 events
clock_residual_p99_us 不出现异常大的离群值
```

`events=1` 可以验证通信路径，但不能拟合时钟斜率；做校准时至少使用几十个 event，推荐 256 或 1024。

新增的 `aiv_host_paced_timestamp` 会在一次运行内部自动执行两阶段：

```text
阶段一：用 calibration-events 个 event 执行 host_aiv_roundtrip，拟合 scale/offset
阶段二：AIV 按 delay-iters 间隔写包含 timestamp 和 flag 的 64B record，Host CPU 轮询 ready 字段
```

每个 event 使用一个独立的 64B record，timestamp 和 flag 在同一次 AIV `DataCopy` 中写入：

```text
record[event] word 0/1: device cycle
record[event] word 2:   sequence
record[event] word 3:   ready flag
```

AIV 在发起这次 64B 写入前读取 device cycle，并把 ready flag 作为 record 的完成字段。Host 只有看到 ready flag 后，才读取时间戳并计算。这样测试插桩不会额外增加第二次 Host DRAM 写入；该模式测的是一次 64B instrumented flag record 的可见性。

```text
t1_host = scale * device_cycle + offset
latency = HostNowNs() - t1_host
```

这个 latency 的准确含义是：

```text
AIV 开始发起 Host DRAM 写入
    -> Host CPU 观察到对应 flag
```

它包含 mapped Host memory 的可见性、缓存一致性和 Host polling 发现延迟，适合作为 pipeline 同步开销指标；不是单独的 DRAM 物理传播时间。

最小测试：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=aiv_host_paced_timestamp \
  --events=256 \
  --calibration-events=256 \
  --delay-iters=4096 \
  --warmup=2 \
  --iters=10 \
  --timeout-ms=5000
```

推荐先做 delay sweep：

```bash
for d in 256 1024 4096 16384; do
  ./build-a5/aiv_hostcpu_bench \
    --device=0 \
    --mode=aiv_host_paced_timestamp \
    --events=256 \
    --calibration-events=256 \
    --delay-iters=${d} \
    --warmup=2 \
    --iters=10 \
    --timeout-ms=5000
done
```

重点看：

```text
latency_avg_us / latency_p50_us / latency_p99_us
  AIV->Host 单向观测时延，建议以 p50/p99 为主要结果。

latency_min_us / latency_max_us
  用来发现异常值，不建议只拿 min 作为结论。

calibration_min_rtt_us
  本轮时钟校准中观测到的最小 Host roundtrip。

calibration_residual_p50_us / calibration_residual_p99_us
  时钟拟合误差；如果明显大于 latency 本身，说明校准结果不可靠。

one_poll_count
  Host 第一次读取对应 flag 就看到新值的 event 数。delay 太小时该值可能接近 events，
  表示 Host 仍然会批量扫到已经完成的 flag；此时增大 delay-iters。

timestamp_errors
  timestamp record 中的 sequence 与 flag 不匹配时增加，正常应为 0。
```

注意：`aiv_host_paced_timestamp` 的 `host_total_us` 包含前面的 roundtrip 校准阶段，不要用它代表单向时延；单向时延应看 `latency_*_us`。当前在线轮询路径不在每次 load 时调用 `aclrtMemInvalidate`，这样测到的是正常 coherent mapped Host DRAM 路径；如果平台必须显式 invalidate，应单独增加对照实验，否则 invalidate 开销会混入主结果。

`host_aiv_roundtrip_fast` 是更瘦的 roundtrip 版本。它的热路径是：

```text
Host CPU:
  对每个 event:
      t0 = HostNowNs()
      cpu_req = seq
      while aiv_ack < seq:
          只轮询 ack，不计 host_poll_iters，不检查 status/timeout/yield
      t1 = HostNowNs()

AIV:
  写 ready = 1
  对每个 event:
      while cpu_req < seq:
          只读 cpu_req，不写 aiv_poll_iters，不检查 abort/poll_limit
      写 aiv_ack = seq
```

也就是说，相比 `host_aiv_roundtrip`，fast 版本去掉了每轮这些诊断开销：

```text
AIV 每轮写 aiv_poll_iters
Host 每轮 hostPolls++
Host 每轮读 status
Host 每轮 HostNowNs() timeout 检查
Host 每轮 yield 检查
AIV 每轮 abort/poll_limit 检查
```

fast 版本仍然包含必要动作：

```text
Host 写 cpu_req
Host store fence
AIV 从 mapped host memory 读 cpu_req
AIV 写 aiv_ack 到 mapped host memory
Host CPU 轮询读 aiv_ack
HostNowNs() 记录每轮开始和结束
```

建议先确认 `host_aiv_roundtrip` 已经 `status=0`，再跑 fast 版本：

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_aiv_roundtrip_fast \
  --events=256 \
  --warmup=2 \
  --iters=10 \
  --timeout-ms=5000
```

fast 输出没有 `host_poll_iters_avg` 和 `aiv_poll_iters_avg`，重点看：

```text
roundtrip_avg_us / roundtrip_min_us / roundtrip_max_us
  更接近 Host 写 req -> AIV 读 req -> AIV 写 ack -> Host 读 ack 的最瘦软件观测闭环。

ready / final_req / final_ack / final_status / abort
  `final_req == final_ack == events` 且 `status=0` 时，说明所有 event 都完成。
```

注意：fast 版本为了减少热路径干扰，没有在每轮 req/ack 等待里做 timeout 和 abort 检查。如果 CPU->AIV 方向没有先被 debug 版本验证过，fast 版本可能卡在 AIV 轮询 `cpu_req` 或 Host 轮询 `aiv_ack` 上。

### AIV 写 Host flag latency 测试

`flag_latency` 不再分配和处理 x/y/z float 数据，只分配 host registered flag buffer。每个 event 使用一个独立 64B flag slot，AIV 写 `event + 1`，Host CPU 轮询对应 slot。这个模式是辅助实验，不是当前主对比口径。

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

## Host payload smoke test: AIV -> host DRAM

The `host_payload_poll` mode validates the next step needed by the DSA pipeline:

```text
AIV writes 2048 int32 token IDs to mapped host DRAM
    -> AIV writes the per-batch ready flag
    -> Host CPU polls the flag
    -> Host CPU validates all 2048 token IDs
```

The payload buffer is allocated with `aclrtMallocHost`, registered with
`aclrtHostRegister(..., ACL_HOST_REGISTER_MAPPED, &payloadDev)`, and accessed by
the AIV kernel through `payloadDev`. The host poller reads `payloadHost` only
after observing the corresponding ready flag. Each batch uses a distinct
pattern, so stale, partial, or cross-batch data is detected.

Build on an A5 host:

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

Run the minimum test:

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_payload_poll \
  --tasks=4 \
  --warmup=1 \
  --iters=5 \
  --timeout-ms=5000
```

## 将测试日志整理为 Excel

`log_to_excel.py` 可以把 `aiv_hostcpu_bench` 的文本日志整理成老师可以直接查看的 Excel 报告：

```text
老师摘要：按 delay_iters 汇总 P50/P99、有效轮数、负时延和异常提示
每轮结果：每个 iter 的原始指标
字段说明：每个字段的含义、单位和解读方法
原始日志：保留原始行以及修复终端换行后的文本，便于追溯
```

运行方法：

```bash
python3 log_to_excel.py /path/to/log.txt \
  --output aiv_hostcpu_latency_summary.xlsx
```

当前脚本默认使用 `openpyxl` 生成 `.xlsx`，如果环境缺少该依赖：

```bash
python3 -m pip install openpyxl
```

报告中建议优先看 `老师摘要`：当前日志中 delay=256/1024/4096 的有效样本显示，典型单向观测时延约为 1 μs 量级；delay=16384 出现负时延，应先改善时钟校准稳定性后再作为结论。

Recommended stress run:

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_payload_poll \
  --tasks=24 \
  --warmup=2 \
  --iters=20 \
  --timeout-ms=5000
```

Expected result for every iteration:

```text
host_payload_poll status=0 ... payload_errors=0 ... first_flag=1 last_flag=24
```

`status=0` and `payload_errors=0` mean that every observed ready flag was
followed by a complete and correct 2048-element payload. `status=2` means the
host poll timed out. `status=4` means the flag was observed but at least one
payload element did not match the expected batch pattern.

## Host CPU indexer_update integration test

The `host_indexer_update` mode adds the first end-to-end bring-up for the
CPU-host version of `indexer_update`:

```text
AIV test kernel writes 2048 legal token IDs [0, 2047]
    -> mapped Host pinned DRAM payload
    -> AIV writes the per-batch ready flag
    -> HixlIndexerUpdateCpu polls the ready flags
    -> Host CPU classifies hits/misses and updates cached_token_slots
    -> Host CPU publishes per-batch done_flags
```

This is intentionally a simulated AIV producer, not the real
`lightning_indexer` output. It isolates the Host DRAM visibility and CPU
`indexer_update` consumer before the real lightning_indexer is connected.

The test initializes, for every batch:

```text
topk token IDs:       0 .. 2047
cached hits:          token 0 .. 1023
reusable cache rows:  token 2048 .. 3071
expected misses:      token 1024 .. 2047
```

Therefore each batch is expected to report 1024 hits, 1024 misses, and 1024
evictions. The test also checks the updated cache rows and every CPU done flag.

The smoke-test executable links the single host implementation from the
hcomm checkout. Configure with `HCOMM_ROOT` if the default sibling path does
not apply:

```bash
cmake -S . -B build-a5 \
  -DCANN_INSTALL_PATH="$ASCEND_HOME_PATH" \
  -DHCOMM_ROOT="$PWD/../hccl/hcomm"
cmake --build build-a5 -j
```

Run the minimum A5 test:

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_indexer_update \
  --tasks=4 \
  --seq-len=65536 \
  --warmup=1 \
  --iters=5 \
  --timeout-ms=5000
```

Recommended stress test:

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_indexer_update \
  --tasks=24 \
  --seq-len=65536 \
  --warmup=2 \
  --iters=20 \
  --timeout-ms=5000
```

Expected result for every measured iteration is similar to:

```text
host_indexer_update status=0 cpu_status=0 ... \
payload_errors=0 output_errors=0 done_errors=0 \
hit_total=4096 miss_total=4096 first_flag=1 last_flag=4
```

For `tasks=24`, the expected totals are `hit_total=24576` and
`miss_total=24576`. `cpu_update_us` includes the CPU-side ready-flag wait and
the Host CPU index update. `host_total_us` additionally includes the AIV
launch and final stream synchronization. Neither value is the pure
`lightning_indexer` latency.

This mode does not modify the existing AICPU `HixlIndexerUpdate` path or any
NPU graph. After it passes, replace only the simulated AIV producer with the
real lightning_indexer output and keep the Host CPU consumer and flag protocol
unchanged for the next integration step.
