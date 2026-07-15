# RDMA/URMA HBM Benchmark
## 目标
本工程只保留三类独立带宽测试：AIV、Host ACL memcpy、Host HCOMM/URMA。

## 1. AIV 读写 HBM

使用 `hbm_baseline`，AIV kernel 直接访问 Device HBM。

```text
--hbm-op=read   AIV 读取 HBM
--hbm-op=write  AIV 写入 HBM
--hbm-op=copy   AIV 读取后再写入 HBM
```

主要参数：

```text
bytes   总数据量
tile    每次 DataCopy 的数据粒度，单位为 float 元素
repeat  同一 kernel 内重复处理次数
blocks  启动的 AIV block 数
```

建议使用较大的连续数据量和 `repeat=1` 测量单次流式 HBM 访问。

## 2. Host CPU 通过 ACL memcpy 读写 HBM

使用 `comm_baseline`，由 Host CPU 调用 ACL runtime 的 H2D/D2H copy。

```text
Host DRAM --aclrtMemcpy--> Device HBM
Device HBM --aclrtMemcpy--> Host DRAM
```

当前 `acl_write_hbm` 用于测试 Host 写 Device HBM。该路径是本地 ACL/DMA copy baseline，不是 RDMA/URMA。

## 3. Host CPU 通过 HCOMM/URMA 读写 HBM

Host 作为通信 endpoint，Device HBM 作为远端内存。

```text
Host endpoint：COMM_ENGINE_CPU
Device endpoint：COMM_ENGINE_AICPU
Host memory：COMM_MEM_TYPE_HOST
Device HBM：COMM_MEM_TYPE_DEVICE
```

支持的操作：

```text
hcomm_host_write_hbm  Host DRAM -> Device HBM
hcomm_host_read_hbm   Device HBM -> Host DRAM
hcomm_host_rw_hbm     交替执行 Write 和 Read
```

Host/Device endpoint 使用 UBC/URMA EID。Host buffer 使用 4096 字节对齐的普通 Host DRAM，并注册为 `COMM_MEM_TYPE_HOST`；Device buffer 使用 `aclrtMalloc` 分配并注册为 `COMM_MEM_TYPE_DEVICE`。

## A5 构建

### AIV 和 ACL memcpy 版本

```bash
bench_root=/home/z00888267/rdma_urma_bench
cd "${bench_root}"
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5
cmake -S . -B build-a5 \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build-a5 -j
```

### HCOMM/URMA 版本

```bash
bench_root=/home/z00888267/rdma_urma_bench
hcomm_root=/home/z00888267/hcomm/hcomm
cd "${bench_root}"
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5-hcomm
cmake -S . -B build-a5-hcomm \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch \
  -DENABLE_HCOMM=ON \
  -DHCOMM_ROOT="${hcomm_root}"
cmake --build build-a5-hcomm -j
```

## A5 运行

### AIV 读取 HBM

```bash
./build-a5/rdma_urma_bench \
  --device=0 --mode=hbm_baseline --hbm-op=read \
  --bytes=512M --tile=16384 --repeat=1 --blocks=4096 \
  --warmup=5 --iters=5
```

### AIV 写入 HBM

```bash
./build-a5/rdma_urma_bench \
  --device=0 --mode=hbm_baseline --hbm-op=write \
  --bytes=512M --tile=16384 --repeat=1 --blocks=4096 \
  --warmup=5 --iters=5
```

输出中的 `hbm_bw_GBps` 是 AIV HBM 有效带宽。

### Host CPU 通过 ACL 写 HBM

```bash
./build-a5/rdma_urma_bench \
  --device=0 --mode=comm_baseline --comm-op=acl_write_hbm \
  --bytes=256M --comm-iters=32 --warmup=1 --iters=5
```

`comm-iters` 表示重复执行 H2D copy 的次数，`comm_bw_GBps` 是 Host ACL copy 有效带宽。

### 设置 HCOMM/URMA EID

```bash
export DEVICE_ID=6
export DEVICE_PHY_ID=6
export DEVICE_EID=000000000000020000100000df00c101
export HOST_EID=00000000003f030000100000df080b01
```

### Host CPU 通过 HCOMM/URMA 写 HBM

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} --mode=comm_baseline \
  --comm-op=hcomm_host_write_hbm --bytes=256M \
  --comm-iters=32 --warmup=1 --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17001
```

### Host CPU 通过 HCOMM/URMA 读 HBM

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} --mode=comm_baseline \
  --comm-op=hcomm_host_read_hbm --bytes=256M \
  --comm-iters=32 --warmup=1 --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17001
```

### Host CPU 通过 HCOMM/URMA 读写 HBM

`hcomm_host_rw_hbm` 按传输次数交替执行 Write 和 Read：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} --mode=comm_baseline \
  --comm-op=hcomm_host_rw_hbm --bytes=256M \
  --comm-iters=32 --warmup=1 --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17001
```

其中：

```text
comm-iters=32  表示 32 次 URMA 传输操作
偶数次执行 Write，奇数次执行 Read
comm_bytes     总传输字节数
comm_bw_GBps   Host HCOMM/URMA 有效带宽
```

## 输出参数

```text
hbm_bw_GBps   AIV 直接访问 HBM 的有效带宽
comm_bw_GBps  Host ACL 或 HCOMM/URMA 访问 HBM 的有效带宽
comm_us       通信路径总测量时间
comm_copies   实际完成的传输次数
comm_bytes    程序统计的逻辑传输字节数
```

`comm_bw_GBps` 是根据逻辑传输字节数和耗时计算出的有效带宽，不等同于硬件计数器统计的物理 HBM 流量。

### HCOMM/URMA 多通道参数

使用 `--hcomm-channels=N` 创建 N 对 Host/Device channel，并由 N 个 Host worker 并发传输。Host DRAM 和 Device HBM 会按 channel 切分为互不重叠的区域；内存只注册一次，多个 channel 复用远端内存描述。

保持总传输量不变，依次测试：

```bash
--hcomm-channels=1
--hcomm-channels=2
--hcomm-channels=4
--hcomm-channels=8
```

例如在 HCOMM 命令中增加：

```bash
--bytes=256M --comm-iters=32 --hcomm-channels=4
```

`comm-iters` 表示所有 channel 合计的操作数，不会因为增加 channel 而自动增加总传输字节数。每条 channel 使用独立端口 `hcomm-port + channel_index`。
