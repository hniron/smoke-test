# SIMT flag visibility test

这个目录是一个独立实验工程，用于目标 A：比较 AIV 写 GM flag 和 SIMT 写 GM flag 后，被同一个 AICPU 轮询 consumer 观察到的可见性差异。

## 实验边界

- AICPU consumer 默认使用 `AivAicpuPollFlags` 顺序轮询 `flags[i * kFlagPadCount]`，看到 `i + 1` 后记录 `seen_ns[i]` 和 `poll_iters[i]`。并发写 flag 实验可使用 `AivAicpuPollFlagsScan` 扫描记录模式。
- `aiv_store` 使用 AscendC AIV kernel，通过 UB local tensor + `DataCopy` 写 GM flag。
- `simt_store` 使用一个 SIMT thread 顺序写 flag，写法是 volatile global store。
- `simt_atomic` 使用一个 SIMT thread 顺序执行 `asc_atomic_exch` 写 flag。
- 默认 `--delay-iters=0`，producer 不做真实阶段性计算，重点看 flag 写入后对 AICPU 的可见性。
- 主要看 `aicpu_seen_interval_avg_us/min/max` 和 `poll_iters_avg`。`host_total_us` 会包含 launch 和 stream sync 等开销，只作为辅助指标。

## 三种写 flag 方式

三种模式的高层语义一致：都把 `task + 1` 写到 GM flag 数组的 `flags[task * kFlagPadCount]`，AICPU consumer 轮询的也是这个地址。因此它们在 AICPU 观察侧是可比的。

底层写内存方式不同：

- `aiv_store`：AIV/AscendC 写法。先在 UB local tensor 里准备数据，再通过 `DataCopy(flagGm_[task * kFlagPadCount], local, kFlagPadCount)` 从 UB 写到 GM。当前 `kFlagPadCount = 8`，所以每个 flag slot 实际写 8 个 `int32_t`，第 0 个是 flag，后面是 padding。
- `simt_store`：SIMT 普通 GM store。一个 SIMT thread 把 `flags` 转成 `volatile uint32_t *`，然后执行 `out[task * kFlagPadCount] = task + 1U`。它不经过 UB tensor，也不使用 atomic，一次只写 flag 这个 4 字节位置。`volatile` 主要用于避免编译器优化掉这个内存写。
- `simt_atomic`：SIMT atomic exchange。一个 SIMT thread 对同一个 GM flag 地址执行 `asc_atomic_exch(&flags[task * kFlagPadCount], task + 1U)`。它通过 CANN SIMT atomic API 写入，语义比普通 store 更强，通常也更重。当前实验只有一个 writer 写 flag，所以 atomic 的多 writer 原子性不是必须条件，更适合用来观察 atomic 写路径本身的可见性代价。

因此，当前实验比较的是三种实际写法的可见性：`aiv_store` 是 UB + `DataCopy` 到 GM，`simt_store` 是 SIMT 直接普通 GM store，`simt_atomic` 是 SIMT atomic exchange 到 GM。如果后续希望写入粒度也完全对齐，可以把 AIV 改成只写 1 个 `int32_t`，或把 SIMT store 改成写满 `kFlagPadCount` 个 `uint32_t`。

## 构建

在 A5/CANN 9.x 环境中：

```bash
cd /home/z00888267/simt_test/smoke-test
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5
cmake -S . -B build-a5 \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build-a5 -j
```

如果你的环境里 `CANN_INSTALL_PATH` 或 `ASCEND_HOME_PATH` 已经指向 CANN 根目录，可以省略 `-DCANN_INSTALL_PATH=...`。

当前工程会生成三个独立可执行文件：

- `aiv_flag_visibility`：只包含 AIV producer。
- `simt_store_visibility`：只包含 SIMT 普通 store producer。
- `simt_atomic_visibility`：只包含 SIMT atomic exchange producer。

不要使用旧的 `simt_flag_visibility` 单体方案；A5 当前链接器不允许一个可执行里混用 AIV launch 和 SIMT launch。

## 运行

每次测试前，建议在当前 shell 里统一准备运行环境。`source set_env.sh` 之后显式补上 Python site-packages，避免 GE/TBE 初始化 custom AICPU op 时找不到 `numpy`：

```bash
cd /home/z00888267/simt_test/smoke-test
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

export PATH=/usr/local/python3.11.10/bin:$PATH
unset PYTHONHOME
export PYTHONPATH=/usr/local/python3.11.10/lib/python3.11/site-packages:$PYTHONPATH

export ASCEND_CUSTOM_OPP_PATH=$PWD/build-a5/custom_opp/vendors/cust
export LD_LIBRARY_PATH=$ASCEND_CUSTOM_OPP_PATH/op_proto/lib/linux/$(uname -m):$ASCEND_CUSTOM_OPP_PATH/op_impl/cpu/aicpu_kernel/impl:$LD_LIBRARY_PATH
```

同一个 shell 里只需要设置一次；重新登录、重新开终端或重新开 shell 后需要重新执行这一段。

先跑 AICPU 自定义算子冒烟测试：

```bash
./build-a5/aiv_flag_visibility --device=0 --mode=aicpu_noop --warmup=0 --iters=1
```

如果 `aicpu_noop` 成功，再分别跑三组 producer：

```bash
./build-a5/aiv_flag_visibility --device=0 --mode=aiv_store --tasks=64 --iters=20 --warmup=3 --delay-iters=0
./build-a5/simt_store_visibility --device=0 --mode=simt_store --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
```

如果 `--delay-iters=0` 下多个 flag 太快，AICPU interval 分辨不出来，可以临时加大：

```bash
./build-a5/aiv_flag_visibility --device=0 --mode=aiv_store --tasks=64 --delay-iters=1000
./build-a5/simt_store_visibility --device=0 --mode=simt_store --tasks=64 --delay-iters=1000 --simt-threads=32
./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic --tasks=64 --delay-iters=1000 --simt-threads=32
```

这只用于拉开 flag 写入间隔，不代表真实业务计算阶段。

## 多 thread 并发写 flag

并发写 flag 第一版新增四个 mode：

- `simt_store_parallel_seq`：多个 SIMT thread 按 stride 分配不同 flag，普通 GM store 写入；AICPU 按 `flag0 -> flag1 -> ...` 顺序消费。
- `simt_store_parallel_scan`：多个 SIMT thread 按 stride 分配不同 flag，普通 GM store 写入；AICPU 使用 `AivAicpuPollFlagsScan` 循环扫描所有 flag，谁先出现就先记录谁。
- `simt_atomic_parallel_seq`：多个 SIMT thread 按 stride 分配不同 flag，atomic exchange 写入；AICPU 顺序消费。
- `simt_atomic_parallel_scan`：多个 SIMT thread 按 stride 分配不同 flag，atomic exchange 写入；AICPU 扫描记录。

SIMT producer 的分配方式是：

```cpp
for (uint32_t task = threadIdx.x; task < taskCount; task += blockDim.x) {
    flags[task * kFlagPadCount] = task + 1;
}
```

例如 `--tasks=64 --simt-threads=4` 时，`thread0` 写 `flag0/4/8...`，`thread1` 写 `flag1/5/9...`，以此类推。

建议先对比 seq 和 scan 两种 AICPU 观察方式：

```bash
./build-a5/simt_store_visibility --device=0 --mode=simt_store_parallel_seq --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
./build-a5/simt_store_visibility --device=0 --mode=simt_store_parallel_scan --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic_parallel_seq --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic_parallel_scan --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
```

`seq` 模式回答的是 AICPU 按顺序消费并发 flag 的表现；`scan` 模式更适合观察多 thread 并发写不同 flag 后，AICPU 第一次扫描到每个 flag 的时间。`scan` 记录的仍然是 AICPU 观察时间，不是 producer 写入指令真正完成的绝对时间，精度会受 `taskCount` 和扫描周期影响。

## Producer-only profiling

如果 `msprof` 包住带 custom AICPU consumer 的模式时卡在启动阶段，可以先用 producer-only mode 只采 AIV/SIMT producer kernel。这个路径不会调用 `AivAicpuPollFlags` 或 `AivAicpuNoop`，因此不依赖 `ASCEND_CUSTOM_OPP_PATH`；如果环境里已经设置了也不影响。

先不加 `msprof` 做冒烟：

```bash
./build-a5/aiv_flag_visibility --device=0 --mode=aiv_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0
./build-a5/simt_store_visibility --device=0 --mode=simt_store_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0 --simt-threads=32
./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0 --simt-threads=32
```

再用轻量 `msprof` 包 producer-only：

```bash
mkdir -p msprof_out
msprof --output=./msprof_out/aiv_only --application="./build-a5/aiv_flag_visibility --device=0 --mode=aiv_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0" --task-time=on --runtime-api=on
msprof --output=./msprof_out/simt_store_only --application="./build-a5/simt_store_visibility --device=0 --mode=simt_store_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0 --simt-threads=32" --task-time=on --runtime-api=on
msprof --output=./msprof_out/simt_atomic_only --application="./build-a5/simt_atomic_visibility --device=0 --mode=simt_atomic_only --tasks=64 --iters=1 --warmup=0 --delay-iters=0 --simt-threads=32" --task-time=on --runtime-api=on
```

注意：`*_only` 只用于看 producer kernel 的 launch/执行时间线，输出的 `host_total_us` 是 host 侧 launch 到 stream sync 的时间，不包含 AICPU 观察 flag 的可见性指标。目标 A 的同步可见性对比仍然要看 `aiv_store`、`simt_store`、`simt_atomic` 三个带 AICPU poll 的模式。

## 指标解读

每组重点看：

- `status=0`：AICPU poll 正常完成。
- `aicpu_seen_interval_avg_us`：AICPU 看到相邻 flag 的平均间隔，是目标 A 的主要比较指标。
- `aicpu_seen_interval_min_us` / `aicpu_seen_interval_max_us`：相邻 flag 可见间隔的范围。
- `poll_iters_avg`：AICPU 轮询次数，辅助判断可见性差异。

`host_total_us` 包含 kernel launch、stream synchronize 等 host 侧开销，只作为辅助参考。
