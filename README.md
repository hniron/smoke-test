# AIV-AICPU 同步开销 Benchmark

这个目录是一个独立实验工程，用来比较 AIV 和 AICPU 之间两种同步方案的端到端开销。

## 实验目标

背景场景是：AIV kernel 在计算过程中分阶段产生中间结果，AICPU 希望尽早感知这些中间结果 ready，然后继续后续控制或 IO 流程。本实验暂时不模拟真实 IO，只测同步与调度开销。

需要比较的不是单个 API 的裸开销，而是两条实际执行路径的整体代价：

```text
方案一：一个 AIV kernel 内部分阶段写 flag，AICPU kernel 提前启动并轮询 flag
方案二：把 AIV kernel 拆成 N 个 kernel，每段后面接 event，再启动 AICPU kernel
```

如果方案一成立，它可以让 AICPU 在 AIV kernel 尚未结束时就看到某个阶段的结果，从而把 AICPU 后续动作和 AIV 后续计算 overlap 起来。方案二更成熟可靠，但会引入 kernel 拆分、event 依赖和多次 AICPU kernel 启动的额外开销。

## 两种方案

### 方案一：共享内存 flag 同步

执行流程：

```text
stream1: AICPU PollFlags kernel 启动，开始轮询 flags[0..N-1]
stream0: AIV aiv_multi_stage_add kernel 启动

AIV kernel 内部：
  stage 0 计算完成 -> 写 flag[0] = 1
  stage 1 计算完成 -> 写 flag[1] = 2
  ...
  stage N-1 计算完成 -> 写 flag[N-1] = N

AICPU kernel 内部：
  看到 flag[i] 达到期望值后，记录当前 AICPU 时间戳
```

这个方案主要观察：AIV 在同一个 kernel 内写出的阶段 flag，AICPU 是否能及时看到，以及看到这些 flag 的间隔是否接近 AIV 阶段计算节奏。

### 方案二：event synchronize

执行流程：

```text
for i in 0..N-1:
  stream0: 启动一次 AIV aiv_one_stage_add kernel
  stream0: aclrtRecordEvent(event[i])
  stream1: aclrtStreamWaitEvent(event[i])
  stream1: 启动一次 AICPU StampFlag kernel
```

这个方案的计时口径是端到端开销，也就是：

```text
拆 kernel + event record/wait + AICPU kernel 启动 + stream 执行完成
```

这样比只测裸 event wait 更合理，因为真实方案二的成本正是来自 kernel 切分和调度节点增多。

## 代码结构

```text
aiv_aicpu_bench/
  bench_main.asc          # Host driver + AIV Ascend C kernels
  aicpu_poll_kernel.cc    # AICPU PollFlags / StampFlag kernels
  aicpu_poll_kernel.h     # Host 和 AICPU 共享的参数结构
  CMakeLists.txt          # 构建 AIV 可执行文件、AICPU so、AICPU json
  README.md
```

其中：

- `aiv_multi_stage_add`：方案一使用，一个 AIV kernel 内部执行 N 个阶段。
- `aiv_one_stage_add`：方案二保留校验版本使用，每次只执行一个阶段，并写 flag。
- `aiv_one_stage_add_no_flag`：纯 event 版本使用，每次只执行一个阶段，不写 flag。
- `PollFlags`：方案一使用，AICPU 轮询所有 flag，并记录每个 flag 被看到的时间。
- `StampOnly`：纯 event 版本使用，event wait 之后启动，只记录 AICPU kernel 真正执行到的时间。
- `StampFlag`：方案二保留校验版本使用，event wait 之后启动，记录时间并校验 flag。

## 构建

先加载 CANN 环境，例如：

```bash
source /usr/local/Ascend/cann/set_env.sh
```

如果 CANN 不在默认路径，需要确保 `CANN_INSTALL_PATH` 或 `ASCEND_HOME_PATH` 指向实际安装目录。

然后构建：

```bash
cd /home/allen/workdir/zhn/aiv_aicpu_bench
mkdir -p build
cd build
cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
make -j
```

`dav-c220` 是你这套 910B3 环境已验证可用的 AICore 架构值；如果当前 CANN 样例使用其他架构名，需要替换成实际支持的值。当前会话里没有 CANN 环境变量，所以我这里只能验证 AICPU C++ 部分，完整 `.asc` 编译需要在实际 Ascend/CANN 环境中执行。

## 运行

示例：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=5 \
  --mode=all \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

参数含义：

- `--device`：使用的 NPU device id。
- `--tasks`：阶段数 N，也就是要比较的细粒度 task 数。
- `--elements`：每个阶段处理的 float 元素数量。调大可以增加单阶段 AIV 计算时间。
- `--tile`：AIV kernel 内部 DataCopy/Add 的 tile 大小，要求 `elements % tile == 0`。
- `--repeat`：每个阶段重复计算次数。调大可以更粗粒度地增加单阶段计算开销。
- `--warmup`：预热轮数，不计入 summary。
- `--iters`：正式统计轮数。
- `--aicpu-json`：AICPU kernel JSON 路径，默认在 build 目录生成。
- `--mode`：选择运行路径，支持 `all`、`shared`、`event_sync_pure`、`event_sync_with_flag_check`、`aicpu_noop`、`aicpu_read_bench`、`aicpu_read_chase`，默认 `all`。
- `--read-stride`：`aicpu_read_bench` 使用的读步长，单位是 `uint32_t` word，默认 1。

建议先把 `--tasks=2`、`--elements` 设置小一点做冒烟；确认方案一能读到 flag 后，再调大 `--elements` 或 `--repeat`，把单阶段计算时间调到约 100us。

## 输出指标

每轮会打印三组结果：

- `shared`：方案一，共享内存 flag 轮询。
- `event_sync_pure`：方案二纯 event 版本，拆 kernel + event + AICPU StampOnly，不写/读 flag。
- `event_sync_with_flag_check`：方案二保留校验版本，拆 kernel + event + AICPU StampFlag，并额外写/读 flag 校验。

关键字段：

- `status`：AICPU kernel 返回状态。`0` 表示成功；非 0 通常表示参数错误或轮询超时。
- `host_total_us`：host 观察到的该方案端到端耗时。
- `host_per_task_us`：`host_total_us / N`。
- `aicpu_seen_interval_avg_us`：AICPU 连续看到两个 flag/阶段之间的平均时间间隔。
- `speedup_event_pure_over_shared`：`event_sync_pure_avg_us / shared_avg_us`。大于 1 表示共享内存方案更快。
- `speedup_event_flag_check_over_shared`：`event_sync_with_flag_check_avg_us / shared_avg_us`，用于观察额外 flag 校验带来的影响。

主对比建议看 summary 里的：

```text
shared_avg_us
event_sync_pure_avg_us
event_sync_with_flag_check_avg_us
speedup_event_pure_over_shared
```

同时用 `aicpu_seen_interval_avg_us` 辅助判断方案一是不是确实形成了细粒度阶段同步。

## AICPU 读延迟 microbenchmark

`aicpu_read_bench` 是一个独立的 AICPU device memory 读测试，不启动 AIV kernel，也不使用 event。它复用 `--tasks * --elements` 决定读数组大小，`--repeat` 决定重复读多少轮，`--read-stride` 决定每次读取的步长。

示例：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=8 \
  --elements=65536 \
  --repeat=16 \
  --read-stride=1 \
  --warmup=1 \
  --iters=5 \
  --mode=aicpu_read_bench \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

关键输出：

- `aicpu_elapsed_us`：AICPU `ReadBench` kernel 内部 `NowNs()` 计到的读循环耗时。
- `ns_per_read`：`aicpu_elapsed_us` 换算成 ns 后除以总读次数。
- `read_count`：本轮总 load 次数，等于 `tasks * elements * repeat`。
- `data_words`：被读数组大小，单位是 `uint32_t` word。
- `read_stride_words`：读步长，单位是 `uint32_t` word。
- `checksum`：读值累加结果，用来避免读循环被优化掉。

建议用不同 `--read-stride` 对比顺序读、跨 cache line 读和大步长读，例如 `1`、`16`、`1024`、`16384`。这个 microbenchmark 仍然不能单独证明每次 load 都打到 HBM，但比从 shared flag 轮询结果里反推 AICPU 读延迟更直接。

`aicpu_read_chase` 是依赖读延迟测试。Host 会在 `buf.x` 中生成一条随机 permutation 链，AICPU 每一步执行 `idx = next[idx]`，下一次读地址依赖上一次读出来的值，因此比固定 stride 读更难被预取或并发隐藏延迟。

示例：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=64 \
  --elements=1048576 \
  --repeat=1 \
  --warmup=0 \
  --iters=3 \
  --mode=aicpu_read_chase \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

关键输出：

- `aicpu_elapsed_us`：AICPU `ReadChase` kernel 内部 `NowNs()` 计到的 chase 循环耗时。
- `ns_per_chase_read`：`aicpu_elapsed_us` 换算成 ns 后除以 `chase_steps`。
- `chase_steps`：依赖读步数，等于 `tasks * elements * repeat`。
- `data_words`：随机链表大小，单位是 `uint32_t` word。
- `checksum`：chase 过程中访问到的索引累加/最终索引混合值，用来避免读链路被优化掉。

`aicpu_read_chase` 更适合逼近 AICPU 读 device memory 的依赖 load latency；`aicpu_read_bench` 更适合看顺序/stride 读吞吐。

## 当前假设和风险

当前共享内存默认使用 `aclrtMalloc` 分配 device memory，flag 每个 task padding 到 8 个 `uint32_t`，这是参考本地 HCOMM AIV sync helper 里 GM flag 拷贝对齐习惯做的。

方案一是否真正成立，取决于底层是否保证 AIV 写 GM 后 AICPU 能及时看到。后续如果你确认共享内存类型、cache 可见性或 fence/invalidate 要求，需要重点检查：

- AIV 写 data/flag 的顺序是否被保证。
- AIV 写 flag 后是否需要额外 flush 或 fence。
- AICPU 轮询 flag 是否可能读到旧 cache。
- AICPU 轮询是否需要 timeout，避免死等。
- `aclrtMalloc` device memory 是否就是目标方案里 AIV/AICPU 都可见的内存。

当前代码已经有 timeout 和 `status` 返回，但 cache/fence 语义仍需要在真实环境中验证。

---

# AIV-AICPU Sync Expense Benchmark

This directory contains a standalone benchmark for comparing two AIV to AICPU synchronization schemes.

## Schemes

1. Shared-memory flag:
   - Launch one AICPU `PollFlags` kernel on `stream1`.
   - Launch one AIV `aiv_multi_stage_add` kernel on `stream0`.
   - The AIV kernel runs `N` internal stages. After every stage it writes `flag[i] = i + 1` to GM.
   - The AICPU kernel polls the padded flags and records the timestamp when each flag becomes visible.

2. Pure event synchronize:
   - Launch `N` AIV `aiv_one_stage_add_no_flag` kernels on `stream0`.
   - Record an event after each AIV kernel.
   - Make `stream1` wait for the event, then launch an AICPU `StampOnly` kernel.
   - This measures the end-to-end cost of kernel splitting, event dependency, and AICPU kernel launch without extra flag writes or reads.

3. Event synchronize with flag check:
   - Launch `N` AIV `aiv_one_stage_add` kernels on `stream0`.
   - Each AIV kernel writes a padded flag after compute.
   - Make `stream1` wait for the event, then launch an AICPU `StampFlag` kernel to timestamp and verify the flag.
   - This keeps the previous validation path for checking event-following memory visibility.

The benchmark intentionally uses one AIV block so a stage flag means the whole stage is complete in the shared-memory and flag-check paths. Increase `--elements` or `--repeat` to tune single-stage compute time toward 100 us.

## Build

Set the CANN environment first, for example:

```bash
source /usr/local/Ascend/cann/set_env.sh
```

Then build:

```bash
cd /home/allen/workdir/zhn/aiv_aicpu_bench
mkdir -p build
cd build
cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
make -j
```

For Ascend 910B3, use `dav-c220` with `--cce-aicore-arch`. Replace it if your CANN samples use another supported architecture value.

## Run

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=5 \
  --mode=all \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

Important output fields:

- `host_total_us`: host-observed end-to-end elapsed time for one scheme.
- `host_per_task_us`: `host_total_us / N`.
- `aicpu_seen_interval_avg_us`: average interval between consecutive AICPU timestamps.
- `speedup_event_pure_over_shared`: `event_sync_pure_avg_us / shared_avg_us`; greater than 1 means shared-memory flag is faster.
- `speedup_event_flag_check_over_shared`: `event_sync_with_flag_check_avg_us / shared_avg_us`; useful for observing the extra flag-check cost.

## Current Assumption

The shared memory is allocated with `aclrtMalloc` device memory and flags are padded to 8 `uint32_t` values per task to match AIV GM copy alignment patterns used in local HCOMM AIV sync helpers. If the final memory type changes, update the allocation and visibility/fence path while keeping the benchmark flow unchanged.
