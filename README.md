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
  参考 A5 上已跑通的 `hixl_example_d2rh --protocol=ub_ctp:host,ub_ctp:device --device=0,5 --version=1`。
  本工程同进程创建两个 HIXL engine：target engine 绑定 `--device` 并注册目标 HBM，source engine 绑定 `--hixl-peer-device` 并注册 host buffer。
  source engine 用 `TransferSync(WRITE)` 把 host buffer 写到 target engine 注册的 device HBM。
  注意：这里的 source engine 仍然绑定一个 NPU device；`MEM_HOST` 只表示 source 侧参与传输的内存是 Host DRAM，不等价于“Host CPU 自己作为纯通信 endpoint”。

hixl_read_hbm
  需要 -DENABLE_HIXL=ON 编译。
  两个 engine 和注册方式同上。
  source engine 用 `TransferSync(READ)` 从 target engine 注册的 device HBM 读到 host buffer。

hixl_host_write_hbm
  需要 -DENABLE_HIXL=ON 编译。
  这是 Host placement / Host endpoint 版本，不再用 `--hixl-peer-device` 作为 source engine 的通信身份。
  可以走 RoCE/IP 的 `placement=host`，也可以显式传 UBC/URMA EID 版 LocalCommRes。
  source 注册 Host DRAM，target 注册 device HBM，然后用 `TransferSync(WRITE)` 写 HBM。

hixl_host_read_hbm
  需要 -DENABLE_HIXL=ON 编译。
  Host placement 配置同上，用 `TransferSync(READ)` 从 target device HBM 读到 source host buffer。

hcomm_host_write_hbm
  需要 -DENABLE_HCOMM=ON 编译。
  参考 `hixl_send_ubc_ring` 的底层 HCOMM 写法，不走 HIXL `TransferSync`。
  同进程创建 Host endpoint(client, COMM_ENGINE_CPU) 和 Device endpoint(server, COMM_ENGINE_AICPU)。
  Host 注册 Host DRAM，Device 注册 target HBM；Host 侧通过 `HcommWriteNbi` 写远端 HBM，并用 `HcommChannelFence` 等待完成。

hcomm_host_read_hbm
  需要 -DENABLE_HCOMM=ON 编译。
  建链和内存注册同上；Host 侧通过 `HcommReadNbi` 从远端 HBM 读到 Host DRAM，并用 `HcommChannelFence` 等待完成。

hcomm_host_rw_hbm
  需要 -DENABLE_HCOMM=ON 编译。
  建链和内存注册同上；Host 侧按传输次数交替执行 `HcommWriteNbi` 和 `HcommReadNbi`，每次传输后用 `HcommChannelFence` 等待完成。
```

HCOMM Host endpoint 的 Host buffer 使用 4096 字节对齐的普通 Host DRAM（`posix_memalign`），再注册为 `COMM_MEM_TYPE_HOST`。这和 `hixl_send_ubc_ring.cpp` 里已跑通的 Host recv buffer 分配方式保持一致，避免 Host endpoint 场景下 `aclrtMallocHost` 内存注册失败。

`hixl_write_hbm` / `hixl_read_hbm` 是 HIXL `TransferSync(WRITE/READ)` 的 device-source 路径。本工程同进程创建两个 HIXL engine，通信身份更准确地说是 `source engine(device5) <-> target engine(device0)`，实际传输内存是 `Host DRAM <-> device0 HBM`。

`hixl_host_write_hbm` / `hixl_host_read_hbm` 是 HIXL Host placement 尝试路径。RoCE/IP Host placement 仍可作为对照；但在当前 A5 UBC/EID 场景下，纯 `source=HOST_EID/host, target=DEVICE_EID/device` 的 HIXL LocalCommRes 可能在 `target.Initialize` 阶段报 `endpoint_list is nullptr`。如果目标是老师说的 Host endpoint 通过 UBC/URMA write/read 本机 Device HBM，优先使用 `hcomm_host_write_hbm` / `hcomm_host_read_hbm` / `hcomm_host_rw_hbm`。

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

HIXL 路径不是默认开启的，因为它需要先有 `libcann_hixl.so`，并且当前 URMA/UB host 路径要参考 A5 上已跑通的 `a5-sendrecv-transport-hacking/hixl` 版本。这个版本的样例 `hixl_example_d2rh --protocol=ub_ctp:host,ub_ctp:device --device=0,5 --version=1` 已验证可通。

如果当前目标只是测试本工程里的 `hixl_write_hbm` / `hixl_read_hbm` baseline，只需要构建 HIXL 主库，不需要构建 HIXL 自带 examples。不要使用 `send` 路径替代本测试；本测试要走 HIXL `TransferSync(WRITE/READ)`。

先在 HIXL 工程中构建 HIXL 主库，参考对应 HIXL 工程的 build 文档：

```bash
# 按 A5 上实际 HIXL 源码目录填写；优先使用已验证可通的 a5-sendrecv-transport-hacking/hixl。
hixl_root=/home/z00888267/a5-sendrecv-transport-hacking/hixl
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
hixl_root=/home/z00888267/a5-sendrecv-transport-hacking/hixl
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

### A5 HCOMM 构建命令

如果 A5 环境已经支持 HCOMM，可以直接构建本工程的 HCOMM 版本。这个版本同时支持 `hcomm_smoke` 和 `hcomm_host_write_hbm` / `hcomm_host_read_hbm`：

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

如果 HCOMM 头文件和 `libhcomm.so` 已经在 CANN 路径下，`-DHCOMM_ROOT=...` 可以省略；CMake 会从 `${ASCEND_HOME_PATH}/aarch64-linux/include`、`${ASCEND_HOME_PATH}/aarch64-linux/lib64` 等路径查找。构建 `hcomm_host_write_hbm/read_hbm/rw_hbm` 需要同时能找到 `hcomm_res.h`、`hcomm_primitives.h` 和 `libhcomm.so`。

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


### 3. HIXL/URMA MEM_HOST 写 target Device HBM baseline

这条路径对齐老师说的 write/read 需求，不使用 `send`。默认配置参考已跑通的 A5 样例：

```text
hixl_example_d2rh --protocol=ub_ctp:host,ub_ctp:device --device=0,5 --version=1
```

本工程里的含义是：

```text
--device=0              target device，AICore HBM kernel 和被通信读写的 HBM 都在这里
--hixl-peer-device=5    source/initiator engine 绑定的另一个 NPU device；不是 device5 HBM 参与传输
--hixl-protocols=...    HIXL v2 自动生成 Host/Device placement 的 UB/URMA 通信资源
```

当前实现需要按两层理解：

```text
建链/通信身份层：source engine(device5) <-> target engine(device0)
实际传输内存层：Host DRAM buffer <-> target device0 HBM buffer
```

写 HBM baseline：

```bash
./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-peer-device=5 \
  --hixl-local-engine=127.0.0.1:16001 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-protocols=ub_ctp:host,ub_ctp:device
```

这条路径对应：

```text
source engine(device5): RegisterMem(MEM_HOST, host buffer)
target engine(device0): RegisterMem(MEM_DEVICE, target HBM buffer)
TransferSync(WRITE): host buffer -> target device0 HBM
```

严格说，这不是“纯 Host CPU endpoint 直接和 device0 endpoint 建链”。它是借助 peer-device HIXL engine 承载通信资源，但传输对象是 Host DRAM 和 target HBM。

### 4. HIXL/URMA 读 target Device HBM baseline

读 HBM baseline：

```bash
./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_read_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-peer-device=5 \
  --hixl-local-engine=127.0.0.1:16001 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-protocols=ub_ctp:host,ub_ctp:device
```

这条路径对应：

```text
source engine(device5): RegisterMem(MEM_HOST, host buffer)
target engine(device0): RegisterMem(MEM_DEVICE, target HBM buffer)
TransferSync(READ): target device0 HBM -> host buffer
```

如果 `hixl_example_d2rh --protocol=ub_ctp:host,ub_ctp:device --device=0,5 --version=1` 能通，而本工程 HIXL mode 不通，优先对比两边使用的 HIXL_ROOT、运行时 `libcann_hixl.so`、`libhcomm.so`、`libascend_hal.so` 和 `${ASCEND_HOME_PATH}/opp/built-in/op_impl/aicpu/config/libcann_hixl_kernel.json` 是否一致。

### 5. HIXL Host placement / Host endpoint 版本

这个版本对应新的 `hixl_host_write_hbm` / `hixl_host_read_hbm`。Host buffer 用普通 `malloc`，source 侧注册 `MEM_HOST`，target 侧注册 `MEM_DEVICE`，然后用 HIXL `TransferSync(WRITE/READ)` 做 HBM 读写。

这里要分清 `comm_id` 的格式：

```text
protocol=roce:
  comm_id 填 Host RoCE 网卡 IP，例如 10.x.x.x

protocol=ub_ctp / ub_tp:
  comm_id 填 EID，例如 00000000003f030000100000df080b01
```

所以 `HOST_EID` 不能填到 `--hixl-host-roce-ip`。如果老师给的是 EID，就用下面的 UBC/EID 方式。

#### 5.1 RoCE/IP 方式

如果 A5 环境有 Host RoCE/HCOMM 数据面 IP，不是 SSH 管理 IP，可以用简化参数。假设查到的是 `10.x.x.x`：

```bash
HOST_ROCE_IP=10.x.x.x
```

RoCE/IP 写 target Device0 HBM：

```bash
./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_host_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=127.0.0.1:16001 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-host-roce-ip=${HOST_ROCE_IP}
```

RoCE/IP 读 target Device0 HBM：

```bash
./build-a5-hixl/rdma_urma_bench \
  --device=0 \
  --mode=comm_baseline \
  --comm-op=hixl_host_read_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hixl-local-engine=127.0.0.1:16001 \
  --hixl-server-engine=127.0.0.1:16000 \
  --hixl-host-roce-ip=${HOST_ROCE_IP}
```

使用 `--hixl-host-roce-ip` 时，本工程会给 source engine 和 target engine 都生成下面这种 Host placement LocalCommRes：

```json
{"version":"1.3","net_instance_id":"default","endpoint_list":[{"protocol":"roce","comm_id":"10.x.x.x","placement":"host"}]}
```

#### 5.2 UBC/URMA EID 方式

HIXL `TransferSync` 的 UBC/EID Host->Device LocalCommRes 方式目前保留为实验路径。当前 A5 上如果出现 `target.Initialize ... endpoint_list is nullptr`，不要继续在这个路径上纠缠，直接使用下一节的 HCOMM primitive 版本。

EID 配置仍然沿用老师脚本里的这组信息：

```bash
export DEVICE_EID=000000000000020000100000df00c101
export DEVICE_ID=6
export DEVICE_PHY_ID=6
export HOST_EID=00000000003f030000100000df080b01
```

#### 5.3 自定义 LocalCommRes

如果 A5 环境要求 source/target 使用不同的 Host endpoint 资源，或者不能接受同一个 RoCE IP 同进程创建两个 HIXL engine，就不要只用 `--hixl-host-roce-ip`，改成分别传：

```bash
--hixl-source-local-comm-res='<source LocalCommRes json>'
--hixl-target-local-comm-res='<target LocalCommRes json>'
```

这条路径和前面的 device5 source 路径区别是：

```text
hixl_write_hbm/read_hbm:
  通信身份层：source engine(device5) <-> target engine(device0)
  数据层：Host DRAM <-> device0 HBM

hixl_host_write_hbm/read_hbm:
  通信身份层：source 不再由 device5 承载；source LocalCommRes 可以是 placement=host
  RoCE/IP 简化写法里 source/target 都生成 placement=host
  UBC/EID 写法里 source 是 HOST_EID/placement=host，target 是 DEVICE_EID/placement=device
  数据层：Host DRAM <-> target device HBM
```

如果 RoCE/IP 方式报建链或 LocalCommRes 错误，优先拿同一组 RoCE IP 去跑 `a5-sendrecv-transport-hacking/hixl/benchmarks/comm_benchmark` 的 `H2rD` / `rD2H`，因为它是 HIXL 官方 benchmark 里最接近本测试 write/read 语义的参考。如果 UBC/EID 方式报错，优先回到老师给的 `hixl_send_ubc_ring` 脚本确认这组 `HOST_EID/DEVICE_EID/DEVICE_ID` 本身仍然可通。

### 6. HCOMM primitive UBC/EID 写读 HBM

这条路径是当前对齐老师 `hixl_send_ubc_ring` 思路的 write/read 版本：

```text
Host endpoint(client, COMM_ENGINE_CPU)
  注册 Host DRAM：COMM_MEM_TYPE_HOST

Device endpoint(server, COMM_ENGINE_AICPU)
  注册 target HBM：COMM_MEM_TYPE_DEVICE

Host 侧拿到远端 device_hbm 后：
  hcomm_host_write_hbm: HcommWriteNbi，Host DRAM -> Device HBM
  hcomm_host_read_hbm:  HcommReadNbi， Device HBM -> Host DRAM
  hcomm_host_rw_hbm:    按传输次数交替 Write / Read
  HcommChannelFence:    每次传输后等待完成
```

7580 行开始的 A5 log 已验证 `hcomm_host_write_hbm` 能跑通，配置为 `bytes=256M`、`comm-iters=32`，每轮总传输 8 GiB，平均带宽约 `31.17 GB/s`。

先设置老师脚本里确认可通的 EID 和 device id：

```bash
export DEVICE_ID=6
export DEVICE_PHY_ID=6
export DEVICE_EID=000000000000020000100000df00c101
export HOST_EID=00000000003f030000100000df080b01
```

HCOMM primitive 写 target Device HBM，已在 A5 上跑通：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} \
  --mode=comm_baseline \
  --comm-op=hcomm_host_write_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17000
```

HCOMM primitive 读 target Device HBM：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} \
  --mode=comm_baseline \
  --comm-op=hcomm_host_read_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17000
```

HCOMM primitive 读写混合访问 target Device HBM：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} \
  --mode=comm_baseline \
  --comm-op=hcomm_host_rw_hbm \
  --bytes=256M \
  --comm-iters=32 \
  --warmup=1 \
  --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17000
```

`hcomm_host_rw_hbm` 中 `comm-iters=32` 表示 32 次 URMA 传输操作，不是 32 对读写；当前实现按次数交替，偶数次 write、奇数次 read，所以 `comm-iters=32` 对应 16 次 write + 16 次 read，总传输字节仍是 `32 * bytes`。

HCOMM primitive + AIV HBM 干涉测试，Host write Device HBM，同时 AIV copy HBM：

```bash
./build-a5-hcomm/rdma_urma_bench \
  --device=${DEVICE_ID} \
  --mode=interference \
  --hbm-op=copy \
  --comm-op=hcomm_host_write_hbm \
  --bytes=256M \
  --tile=1024 \
  --repeat=8 \
  --blocks=8 \
  --warmup=1 \
  --iters=5 \
  --hcomm-protocol=ubc_ctp \
  --hcomm-host-addr=${HOST_EID} \
  --hcomm-device-addr=${DEVICE_EID} \
  --hcomm-device-phy-id=${DEVICE_PHY_ID} \
  --hcomm-port=17000
```

Host read 或 read/write 混合的干涉测试，只需要替换 `--comm-op`：

```bash
--comm-op=hcomm_host_read_hbm
--comm-op=hcomm_host_rw_hbm
```

AIV 侧 `--hbm-op` 可选：

```text
read   AIV 只读 HBM
write  AIV 只写 HBM
copy   AIV 读 HBM 后再写 HBM
```

注意：这里的 `--device` 是 ACL runtime 使用的逻辑 device id；`--hcomm-device-phy-id` 是 HCOMM endpoint 使用的物理 device id。当前环境如果逻辑 id 和物理 id 一致，可以都填 6；如果不一致，以老师脚本或 HCOMM 可通样例里的 phy id 为准。

### 7. HCOMM Host endpoint smoke

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

### 8. HIXL 干涉测试

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

### 9. ACL 干涉测试

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
hixl_host_write_hbm
hixl_host_read_hbm
hcomm_host_write_hbm
hcomm_host_read_hbm
hcomm_host_rw_hbm
```

后续还可以继续补充的通信侧 `comm-op`：

```text
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
