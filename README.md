# SIMT flag visibility test

这个目录是一个独立实验工程，用于目标 A：比较 AIV 写 GM flag 和 SIMT 写 GM flag 后，被同一个 AICPU 轮询 consumer 观察到的可见性差异。

## 实验边界

- AICPU consumer 固定为 `AivAicpuPollFlags`，它 volatile 轮询 `flags[i * kFlagPadCount]`，看到 `i + 1` 后记录 `seen_ns[i]` 和 `poll_iters[i]`。
- `aiv_store` 使用 AscendC AIV kernel，通过 UB local tensor + `DataCopy` 写 GM flag。
- `simt_store` 使用一个 SIMT thread 顺序写 flag，写法是 volatile global store。
- `simt_atomic` 使用一个 SIMT thread 顺序执行 `asc_atomic_exch` 写 flag。
- 默认 `--delay-iters=0`，producer 不做真实阶段性计算，重点看 flag 写入后对 AICPU 的可见性。
- 主要看 `aicpu_seen_interval_avg_us/min/max` 和 `poll_iters_avg`。`host_total_us` 会包含 launch 和 stream sync 等开销，只作为辅助指标。

## 构建

在 A5/CANN 9.x 环境中：

```bash
cd /home/allen/workdir/zhn/simt_test
cmake -S . -B build \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann9.1.0 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build -j
```

如果你的环境里 `CANN_INSTALL_PATH` 或 `ASCEND_HOME_PATH` 已经指向 CANN 根目录，可以省略 `-DCANN_INSTALL_PATH=...`。

## 运行

运行前把 custom OPP 指到本工程构建出来的位置：

```bash
cd /home/allen/workdir/zhn/simt_test
export ASCEND_CUSTOM_OPP_PATH=$PWD/build/custom_opp/vendors/cust
./build/simt_flag_visibility --mode=all --tasks=64 --iters=20 --warmup=3 --delay-iters=0 --simt-threads=32
```

单独跑某个 producer：

```bash
./build/simt_flag_visibility --mode=aiv_store
./build/simt_flag_visibility --mode=simt_store
./build/simt_flag_visibility --mode=simt_atomic
./build/simt_flag_visibility --mode=aicpu_noop
```

如果 `--delay-iters=0` 下多个 flag 太快，AICPU interval 分辨不出来，可以临时加大：

```bash
./build/simt_flag_visibility --mode=all --tasks=64 --delay-iters=1000
```

这只用于拉开 flag 写入间隔，不代表真实业务计算阶段。
