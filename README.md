# RDMA/URMA HBM Interference Benchmark

## 目标

本工程用于测试 RDMA/URMA 传输对 AICore 访问 HBM 带宽的影响。

核心问题：

```text
当 RDMA/URMA 正在读写 HBM 时，
AICore(AIV/AIC) 自己访问 HBM 的带宽会下降多少？
```

## 测试分层

### 1. RDMA/URMA 基础数据

参考 `hcomm/hixl` 的通信接口或已有测试，先单独测 RDMA/URMA 的基础能力：

```text
RDMA/URMA latency
RDMA/URMA bandwidth
不同 message size
不同 Read/Write 方向
不同发起方：NPU / CPU / 同时发起
```

### 2. AICore 访问 HBM baseline

先不跑 RDMA/URMA，只跑 AICore HBM kernel：

```text
AIV/AIC read HBM
AIV/AIC write HBM
AIV/AIC copy HBM -> HBM
```

得到：

```text
AICore_HBM_BW_alone
```

### 3. 并发干涉测试

让 AICore HBM kernel 和 RDMA/URMA 传输同时运行：

```text
AICore 访问 HBM
同时 RDMA/URMA 读写 HBM
```

观察：

```text
AICore_HBM_BW_with_RDMA
RDMA/URMA_BW_with_AICore
```

下降比例：

```text
AICore_drop =
  (AICore_HBM_BW_alone - AICore_HBM_BW_with_RDMA)
  / AICore_HBM_BW_alone
```

## 关键测试轴

必须明确区分：

```text
谁发起传输：NPU / CPU / NPU+CPU
操作类型：Read / Write
src 位置：CPU DRAM / HBM / remote memory
dst 位置：CPU DRAM / HBM / remote memory
AICore 行为：读 HBM / 写 HBM / HBM copy
```

老师给出的例子：

```text
NPU: Read(src_cpu, dst_hbm)
CPU: Write(src_cpu, dst_hbm)
```

这两个最终都是把数据放到 HBM，但发起方和协议路径不同，需要分开测。

## 和已有工程的关系

之前的 `aiv_hostcpu_bench` / `aiv_aicpu_bench` 主要测细粒度 flag 同步时延：

```text
AIV 写 flag
Host CPU / AICPU 轮询 flag
单位是 ns/us
```

本工程测大流量带宽干涉：

```text
RDMA/URMA 大块传输
AICore 大块访问 HBM
单位是 GB/s
```

两者有关联：之前的结果说明方向和发起方会显著影响时延；本工程也必须把 Read/Write、NPU/CPU 发起方、HBM 读写方向拆开，不混成一个数字。

## 当前实现

当前代码包含：

```text
CMakeLists.txt
bench_main.asc
```

支持三类 mode：

```text
hbm_baseline
  只跑 AIV HBM kernel，测 AICore 访问 HBM 的 baseline 带宽。

comm_baseline
  只跑通信压力源。

interference
  AIV HBM kernel 和通信压力源并发运行，测 AIV HBM 带宽下降和通信侧带宽。
```

当前支持的 `comm-op`：

```text
acl_write_hbm
  默认可编译。
  这个路径用 Host CPU/ACL 发起 H2D copy，模拟 CPU: Write(src_cpu, dst_hbm) 的本地写 HBM 压力。
  注意：它不是 RDMA/URMA。

hixl_write_hbm
  需要 -DENABLE_HIXL=ON 编译。
  同进程创建 HIXL server/client 两个 engine：server 注册本机 device HBM，client 注册 host buffer。
  client 用 TransferSync(WRITE) 把 host buffer 写到 server 注册的 device HBM。

hixl_read_hbm
  需要 -DENABLE_HIXL=ON 编译。
  同进程创建 HIXL server/client 两个 engine：server 注册本机 device HBM，client 注册 host buffer。
  client 用 TransferSync(READ) 从 server 注册的 device HBM 读到 host buffer。
```

`hixl_write_hbm` / `hixl_read_hbm` 是为了验证“本机 Host endpoint 通过 HIXL/HCOMM 通信路径读写本机 Device HBM”。它和 `acl_write_hbm` 的差别是：`acl_write_hbm` 是本机 runtime copy；HIXL 路径会执行 `RegisterMem(MEM_HOST/MEM_DEVICE) -> Connect -> TransferSync(READ/WRITE)`。是否真的走 RoCE/UB/URMA，需要结合 HIXL 配置、`HCCL_INTRA_ROCE_ENABLE=1`、`local_comm_res` 和运行日志确认。

## A5 构建命令

参考已有 A5 工程，使用 CANN 9.x 路径：

```bash
cd /home/allen/workdir/zhn/rdma_urma_bench
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5
cmake -S . -B build-a5 \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch
cmake --build build-a5 -j
```

如果当前 shell 里 `ASCEND_HOME_PATH` 或 `CANN_INSTALL_PATH` 已经指向实际 CANN 根目录，也可以把 `-DCANN_INSTALL_PATH=...` 改成对应环境变量。


### A5 HIXL 构建命令

HIXL 路径不是默认开启的，因为它需要先有 `libcann_hixl.so`。本仓当前 `hixl/hixl-master/hixl-master` 下有源码和头文件，但不一定已经编出 `.so`。

先在 HIXL 工程中构建 HIXL，参考 `hixl/hixl-master/hixl-master/docs/build.md`：

```bash
cd /home/allen/workdir/zhn/hixl/hixl-master/hixl-master
source /usr/local/Ascend/cann-9.1.T560/set_env.sh
bash build.sh --examples
```

然后构建本工程的 HIXL 版本：

```bash
cd /home/allen/workdir/zhn/rdma_urma_bench
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5-hixl
cmake -S . -B build-a5-hixl \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch \
  -DENABLE_HIXL=ON \
  -DHIXL_ROOT=/home/allen/workdir/zhn/hixl/hixl-master/hixl-master
cmake --build build-a5-hixl -j
```

如果 CMake 报 `libcann_hixl.so was not found`，说明 HIXL 还没有构建/安装到 `HIXL_ROOT` 下，或者需要把 `HIXL_ROOT` 指到实际安装目录。

## 运行示例

### 1. AIV HBM baseline

```bash
./build-a5/rdma_urma_bench \
  --device=0 \
  --mode=hbm_baseline \
  --hbm-op=copy \
  --bytes=256M \
  --tile=1024 \
  --repeat=8 \
  --blocks=8 \
  --warmup=1 \
  --iters=5
```

`--hbm-op` 可选：

```text
read   只读 HBM
write  只写 HBM
copy   HBM read + HBM write
```

输出重点：

```text
hbm_bw_GBps
```

这是 AIV/AICore 单独访问 HBM 的 baseline。

### 2. CPU/ACL 写 HBM baseline

```bash
./build-a5/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=acl_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5
```

这条路径对应本地近似版：

```text
CPU: Write(src_cpu, dst_hbm)
```

输出重点：

```text
comm_bw_GBps
```


### 3. HIXL Host 写本机 Device HBM baseline

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=10.10.10.0 \
  --hixl-server-engine=10.10.10.0:16000 \
  --hixl-buffer-pool=0:0
```

这条路径对应：

```text
HIXL client: MEM_HOST host buffer
HIXL server: MEM_DEVICE device HBM buffer
TransferSync(WRITE): host buffer -> device HBM
```

### 4. HIXL Host 读本机 Device HBM baseline

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_read_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=10.10.10.0 \
  --hixl-server-engine=10.10.10.0:16000 \
  --hixl-buffer-pool=0:0
```

这条路径对应：

```text
HIXL client: MEM_HOST host buffer
HIXL server: MEM_DEVICE device HBM buffer
TransferSync(READ): device HBM -> host buffer
```

`10.10.10.0` 需要替换成 A5 环境上实际可用的 host/RoCE IP。先用 `ibdev2netdev`、`ifconfig` 或环境已有配置确认网口。用 `127.0.0.1` 可以做功能烟测，但不能直接证明真实 RoCE/URMA 物理链路。

### 5. HIXL 干涉测试

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=interference \
  --hbm-op=copy \
  --comm-op=hixl_write_hbm \
  --bytes=256M \
  --tile=1024 \
  --repeat=8 \
  --blocks=8 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=10.10.10.0 \
  --hixl-server-engine=10.10.10.0:16000 \
  --hixl-buffer-pool=0:0
```

这条路径同时运行：

```text
AIV: copy HBM -> HBM
HIXL: TransferSync(WRITE/READ) 读写 server 注册的 device HBM
```

### 6. 干涉测试

```bash
./build-a5/rdma_urma_bench \
  --device=0 \
  --mode=interference \
  --hbm-op=copy \
  --comm-op=acl_write_hbm \
  --bytes=256M \
  --tile=1024 \
  --repeat=8 \
  --blocks=8 \
  --warmup=1 \
  --iters=5
```

这条路径同时运行：

```text
AIV: copy HBM -> HBM
CPU/ACL: Write(src_cpu, dst_hbm)
```

输出重点：

```text
hbm_bw_GBps
comm_bw_GBps
comm_copies
```

和 `hbm_baseline` 对比：

```text
AICore_drop =
  (hbm_baseline_hbm_bw_GBps - interference_hbm_bw_GBps)
  / hbm_baseline_hbm_bw_GBps
```

## 后续扩展

后续要把真正 RDMA/URMA 接进来时，建议保持现有三段式结构不变：

```text
comm_baseline      只测通信
hbm_baseline       只测 AICore HBM
interference       通信 + AICore HBM 并发
```

已经补充的通信侧 `comm-op`：

```text
hixl_write_hbm
hixl_read_hbm
```

后续还可以继续补充的通信侧 `comm-op`：

```text
hcomm_write
hcomm_read
hixl_put
hixl_get
```

并且每种通信侧都要明确：

```text
发起方：NPU / CPU / 同时
操作：Read / Write
src：CPU DRAM / HBM / remote memory
dst：CPU DRAM / HBM / remote memory
```
