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

支持四类 mode：

```text
hbm_baseline
  只跑 AIV HBM kernel，测 AICore 访问 HBM 的 baseline 带宽。

comm_baseline
  只跑通信压力源。

interference
  AIV HBM kernel 和通信压力源并发运行，测 AIV HBM 带宽下降和通信侧带宽。

hcomm_smoke
  需要 -DENABLE_HCOMM=ON 编译。
  只做 HCOMM Host endpoint / Device endpoint 创建和 Host/HBM 内存注册验证。
  默认不做真实传输，也不默认建 channel。
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

`hcomm_smoke` 是 HCOMM 低层接口的第一步验证，不和 `comm-op` 绑定。它验证的是：

```text
HcommEndpointCreate(ENDPOINT_LOC_TYPE_HOST)
HcommMemReg(COMM_MEM_TYPE_HOST)
HcommEndpointCreate(ENDPOINT_LOC_TYPE_DEVICE)
HcommMemReg(COMM_MEM_TYPE_DEVICE)
可选：HcommChannelCreate(..., COMM_ENGINE_CPU, ...)
```

这个 mode 的目标不是测带宽，而是确认 A5 上 HCOMM basic resource API 是否真的支持“Host 自己作为 endpoint + Host 内存注册 + Device HBM 注册”这条路径。

## A5 构建命令

参考已有 A5 工程，使用 CANN 9.x 路径：

```bash
# 按 A5 上实际 benchmark 工程目录填写。
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

如果当前 shell 里 `ASCEND_HOME_PATH` 或 `CANN_INSTALL_PATH` 已经指向实际 CANN 根目录，也可以把 `-DCANN_INSTALL_PATH=...` 改成对应环境变量。


### A5 HIXL 构建命令

HIXL 路径不是默认开启的，因为它需要先有 `libcann_hixl.so`。本仓当前 `hixl/hixl-master/hixl-master` 下有源码和头文件，但不一定已经编出 `.so`。

如果当前目标只是测试本工程里的 `hixl_write_hbm` / `hixl_read_hbm` baseline，只需要构建 HIXL 主库，不需要构建 HIXL 自带 examples。不要加 `--examples`，否则会额外编译 `aicpu_send_hcomm` 等 sample；这些 sample 依赖额外 HCOMM 头文件路径，和本 benchmark 的 baseline 测试无关。

先在 HIXL 工程中构建 HIXL 主库，参考 `hixl/hixl-master/hixl-master/docs/build.md`：

```bash
# 按 A5 上实际 HIXL 源码目录填写；从你当前 log 看，这里通常是 /home/z00888267/hixl/hixl。
hixl_root=/home/z00888267/hixl/hixl
cd "${hixl_root}"
source /usr/local/Ascend/cann-9.1.T560/set_env.sh
rm -rf build build_out
bash build.sh -j8
find "$(pwd)" -name 'libcann_hixl.so' -o -name 'hixl.h'

# 建议把 HIXL run 包安装到和 CANN toolkit 相同的安装根路径。
# A5 上如果 toolkit 的安装根路径不是 /usr/local/Ascend，需要替换 install_root。
install_root=/usr/local/Ascend
run_pkg=$(ls build_out/cann-hixl_*_linux-*.run | head -n 1)
chmod +x "${run_pkg}"
"${run_pkg}" --full --quiet --pylocal --install-path="${install_root}"

source /usr/local/Ascend/cann-9.1.T560/set_env.sh
for f in \
  "${ASCEND_HOME_PATH}/aarch64-linux/include/hixl/hixl.h" \
  "${ASCEND_HOME_PATH}/aarch64-linux/lib64/libcann_hixl.so" \
  "${ASCEND_HOME_PATH}/opp/built-in/op_impl/aicpu/config/libcann_hixl_kernel.json"
do
  if [ -f "${f}" ]; then
    echo "OK ${f}"
  else
    echo "MISS ${f}"
  fi
done
```

`TransferSync` 运行时会从 `${ASCEND_HOME_PATH}/opp/built-in/op_impl/aicpu/config/libcann_hixl_kernel.json` 加载 HIXL AICPU kernel，所以只编出 `build/src/hixl/libcann_hixl.so` 只能说明能链接，不一定说明运行环境已经完整。源码自编译产生的 `cann-hixl-compat.tar.gz` 默认不含签名头，如果运行时报 HIXL kernel 加载或验签相关错误，需要参考 HIXL `docs/build.md` 的签名说明处理。

然后构建本工程的 HIXL 版本：

```bash
# 按 A5 上实际 benchmark 工程目录填写。
bench_root=/home/z00888267/rdma_urma_bench
hixl_root=/home/z00888267/hixl/hixl
cd "${bench_root}"
source /usr/local/Ascend/cann-9.1.T560/set_env.sh

rm -rf build-a5-hixl
cmake -S . -B build-a5-hixl \
  -DCANN_INSTALL_PATH=/usr/local/Ascend/cann-9.1.T560 \
  -DNPU_ARCH=dav-3510 \
  -DASC_ARCH_FLAG=--npu-arch \
  -DENABLE_HIXL=ON \
  -DHIXL_ROOT="${hixl_root}"
cmake --build build-a5-hixl -j
```

`HIXL_ROOT` 必须指向 A5 上真实存在的 HIXL 源码/构建目录，里面应该有 `include/hixl/hixl.h` 和 `build/src/hixl/libcann_hixl.so`。如果已经通过 run 包把 HIXL 安装进 CANN，本工程的 CMake 也会从 `${ASCEND_HOME_PATH}/aarch64-linux/include` 和 `${ASCEND_HOME_PATH}/aarch64-linux/lib64` 查找 HIXL。

### A5 HCOMM smoke 构建命令

如果 A5 环境已经支持 HCOMM，可以直接构建本工程的 HCOMM 版本：

```bash
# 按 A5 上实际 benchmark 工程目录填写。
bench_root=/home/z00888267/rdma_urma_bench
# 如果 HCOMM 已安装进 CANN，可以不设置 HCOMM_ROOT；否则指向本机 hcomm 源码或安装根目录。
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

如果 HCOMM 头文件和 `libhcomm.so` 已经在 CANN 路径下，`-DHCOMM_ROOT=...` 可以省略；CMake 会从 `${ASCEND_HOME_PATH}/aarch64-linux/include`、`${ASCEND_HOME_PATH}/aarch64-linux/lib64` 等路径查找。

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

注意：当前 `rdma_urma_bench` 的 HIXL 实现是单进程内同时创建 client/server，并且二者使用同一个 `--device`。这种写法在 HIXL Connect 阶段会被判定为 self device，不是有效的 HIXL smoke 路径。下面命令仅保留参数形态，真正运行 HIXL baseline 需要把 client/server 拆成不同 rank/device endpoint。

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=127.0.0.1 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-buffer-pool=0:0
```

这条路径对应：

```text
HIXL client: MEM_HOST host buffer
HIXL server: MEM_DEVICE device HBM buffer
TransferSync(WRITE): host buffer -> device HBM
```

### 4. HIXL Host 读本机 Device HBM baseline

同样需要 client/server 属于不同 rank/device endpoint；当前单进程同 device 写法会被 HIXL 判成 self device。

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_read_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=127.0.0.1 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-buffer-pool=0:0
```

这条路径对应：

```text
HIXL client: MEM_HOST host buffer
HIXL server: MEM_DEVICE device HBM buffer
TransferSync(READ): device HBM -> host buffer
```

要验证真实 RoCE/URMA 物理链路时，`--hixl-local-engine` / `--hixl-server-engine` 里的 IP 必须是 A5 环境上实际可绑定的 host/RoCE IP。先用 `ip -br addr`、`ifconfig`、`ibdev2netdev` 或环境已有配置确认网口；不能使用 README 里的占位网段地址，也不要使用不属于本机网口的 IP，否则 HIXL server 会在 Initialize 阶段 bind 失败。即使用 `127.0.0.1` 能 bind，当前单进程同 device 方案也会在 Connect 阶段因为 self device 被拒绝。

### 5. HCOMM Host endpoint smoke

最小 smoke 默认只验证 endpoint 创建和内存注册，不做真实传输，也不建 channel：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=0 \
  --mode=hcomm_smoke \
  --bytes=4K \
  --hcomm-protocol=roce \
  --hcomm-host-ip=<A5_host_or_roce_ip> \
  --hcomm-device-ip=<device_or_roce_ip> \
  --hcomm-device-phy-id=0 \
  --hcomm-create-channel=0
```

输出里重点看这些步骤：

```text
hcomm_smoke_step=host_endpoint_create ret=0
hcomm_smoke_step=device_endpoint_create ret=0
hcomm_smoke_step=host_mem_reg ret=0
hcomm_smoke_step=device_mem_reg ret=0
hcomm_smoke_step=cleanup ret=0
```

如果这些步骤都是 `ret=0`，说明当前 A5 环境至少已经打通：

```text
Host endpoint + Host DRAM 注册
Device endpoint + Device HBM 注册
```

`hcomm_smoke` 输出里的 `comm_us` 是这几个 HCOMM resource 操作的总耗时，只用于观察 smoke 本身，不代表 RDMA/URMA 带宽或真实传输时延。

如果要进一步试探 `HcommChannelCreate`，显式打开：

```bash
HCCL_INTRA_ROCE_ENABLE=1 ./build-a5-hcomm/rdma_urma_bench \
  --device=0 \
  --mode=hcomm_smoke \
  --bytes=4K \
  --hcomm-protocol=roce \
  --hcomm-host-ip=<A5_host_or_roce_ip> \
  --hcomm-device-ip=<device_or_roce_ip> \
  --hcomm-device-phy-id=0 \
  --hcomm-create-channel=1 \
  --hcomm-role=server \
  --hcomm-port=17000
```

注意：`HcommChannelCreate` 内部会等待 channel ready。若 A5 的 HCOMM 需要成对 endpoint、socket role、真实对端或额外 rank 配置，这一步可能超时；这不等价于前面的 endpoint/mem register smoke 失败。

`--hcomm-device-phy-id=-1` 时，代码会临时把 `--device` 当作 phy id 使用。如果 A5 上逻辑 device id 和物理 device id 不一致，需要显式传正确的物理 id。

### 6. HIXL 干涉测试

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
  --hixl-local-engine=127.0.0.1 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-buffer-pool=0:0
```

这条路径同时运行：

```text
AIV: copy HBM -> HBM
HIXL: TransferSync(WRITE/READ) 读写 server 注册的 device HBM
```

### 7. 干涉测试

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
