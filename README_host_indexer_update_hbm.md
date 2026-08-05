# AIV -> host DRAM -> host CPU indexer_update

这个模式是当前 indexer_update 卸载到 host CPU 的端到端 transport smoke：

```text
AIV producer（确定性 stand-in，模拟 LightningIndexer 的 HBM 输出）
    -> device GM/HBM source buffer
AIV copy kernel
    -> HBM/GM -> UB（DataCopy/MTE2）
    -> UB -> host pinned DRAM（DataCopy/MTE3）
    -> per-batch flag 写入 host pinned DRAM
Host CPU
    -> 轮询 host flag
    -> 读取 host payload
    -> HixlIndexerUpdateCpu
    -> 发布 CPU done_flags
```

它和旧的 `host_indexer_update` 的区别是：旧模式由一个 AIV kernel 直接把 token ID 写到 host DRAM；新模式 `host_indexer_update_hbm` 先写入 HBM source，再由第二个 AIV kernel 通过 UB 搬到 host DRAM，因此可以验证目标数据传输路径和 flag 发布顺序。

当前 smoke-test 仓不链接 `DSA_offload_ops` 的 LightningIndexer torch extension，所以 source producer 使用固定 token ID `0..2047` 作为确定性 stand-in。CPU 算法、flag 轮询、host pinned DRAM payload 和 AIV 的 HBM→UB→DRAM copy 都是真实路径。将来接入真实 LightningIndexer 时，只需把 `aiv_indexer_hbm_payload_producer` 替换成 LightningIndexer，并让 `aiv_indexer_hbm_to_host_payload` 消费 LightningIndexer 的 output buffer。

## A5 编译

在 A5 服务器上执行，`SOC_VERSION` 按本机实际 CANN 支持的名字填写：

```bash
cd /path/to/smoke-test
rm -rf build-a5
cmake -S . -B build-a5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCANN_INSTALL_PATH="$ASCEND_HOME_PATH" \
  -DHCOMM_ROOT=/path/to/hccl/hcomm \
  -DSOC_VERSION=ascend950dt_95a1 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build-a5 -j$(nproc)
```

## 端到端运行

```bash
./build-a5/aiv_hostcpu_bench \
  --device=0 \
  --mode=host_indexer_update_hbm \
  --tasks=4 \
  --seq-len=65536 \
  --warmup=1 \
  --iters=5 \
  --timeout-ms=5000
```

预期输出至少包含：

```text
host_indexer_update_hbm status=0 cpu_status=0 ... payload_errors=0 output_errors=0 done_errors=0 hit_total=4096 miss_total=4096 ... first_flag=1 last_flag=4
```

关键验收条件：

- `status=0`、`cpu_status=0`：CPU indexer_update 正常返回。
- `payload_errors=0`：CPU 看到的 host DRAM payload 与 AIV HBM source 的 `0..2047` 一致，说明 payload 在 flag 之前已经完成写入。
- `output_errors=0`：每个 batch 预期 `1024 hit + 1024 miss`，并且沿用当前淘汰策略。
- `done_errors=0`：CPU 已为每个 batch 写入 done flag。
- `first_flag=1`、`last_flag=tasks`：AIV 已逐 batch 发布 host DRAM ready flag。

## 对照测试

旧的直接写 Host DRAM 路径仍然可以运行：

```bash
./build-a5/aiv_hostcpu_bench --device=0 \
  --mode=host_indexer_update --tasks=4 --seq-len=65536 \
  --warmup=1 --iters=5 --timeout-ms=5000
```

两者都应通过校验；新模式额外覆盖了 HBM/GM→UB→host DRAM 这段传输。

## 失败定位

- `status=2` 或 `cpu_status=2`：CPU 未在 timeout 内观察到 host flag，优先检查 AIV 写 host pinned DRAM、`aclrtHostRegister(...MAPPED...)` 和 flag 写入顺序。
- `payload_errors>0` 且 flag 已完成：检查 HBM source 到 UB、UB 到 host alias 的 `DataCopy`，以及 stream 内两个 AIV kernel 的顺序。
- `output_errors>0`：先确认 `seq-len>=3072`，再检查 CPU cache 初始化和 `evict_slots=-1` 的校验定义；`-1` 是正常的“没有旧 slot 需要淘汰”。

