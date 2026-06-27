# AIV-AICPU Benchmark 测试流程

下面是在有 CANN 和 NPU 的机器上，从构建到测试的完整流程。每个命令后面都说明它的作用和需要观察的点。

## 1. 进入目录并加载环境

进入 benchmark 工程目录：

```bash
cd /home/allen/workdir/zhn/aiv_aicpu_bench
```

作用：后续 `rm -rf build`、`cmake ..`、`make -j` 和运行 `./aiv_aicpu_bench` 都以这个工程目录为基准。这里的 `build` 指的就是 `/home/allen/workdir/zhn/aiv_aicpu_bench/build`。

加载 CANN 环境变量：

```bash
source /usr/local/Ascend/cann/set_env.sh
```

作用：把 CANN 的编译器、头文件、库文件和运行时路径加载到当前 shell。后续 `cmake` 才能找到 Ascend C/ASC 包，`make` 才能找到 `acl/acl.h`、`kernel_operator.h` 和 `ascendcl`，运行程序时也才能找到 CANN 动态库。

如果 CANN 不在这个路径，先确认实际安装目录：

```bash
ls /usr/local/Ascend
```

作用：查看 `/usr/local/Ascend` 下实际有哪些 CANN/Ascend 安装目录，避免 `source` 一个不存在的路径。

也可以显式指定 CANN 路径：

```bash
export CANN_INSTALL_PATH=/usr/local/Ascend/cann
source ${CANN_INSTALL_PATH}/set_env.sh
```

作用：第一行把 CANN 安装路径保存到 `CANN_INSTALL_PATH`，第二行从这个路径加载环境变量。适合 CANN 不在默认路径，或者你希望后续 CMake 明确使用这个路径的情况。

## 2. 重新构建

建议清掉旧 build：

```bash
rm -rf build
```

作用：删除当前工程下的旧构建目录 `/home/allen/workdir/zhn/aiv_aicpu_bench/build`。这不会删除源码文件，只会清理旧的 CMake cache、旧 Makefile、旧可执行文件、旧 `.so` 和旧 JSON。改过源码或改过 `NPU_ARCH` 后建议这样做，避免旧缓存影响构建。

重新创建 build 目录：

```bash
mkdir build
```

作用：创建一个干净的构建目录。工程采用 out-of-source build，编译产物会放在 `build` 里，不污染源码目录。

进入 build 目录：

```bash
cd build
```

作用：后续 `cmake ..` 会以当前 `build` 目录作为输出目录，以 `..` 指向上一级源码目录 `aiv_aicpu_bench`。

生成构建系统：

```bash
cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
```

作用：让 CMake 检测当前 CANN/ASC 编译环境，生成 Makefile，并把 `dav-c220` 通过 `--cce-aicore-arch` 传给 Ascend C 编译器。你这套 910B3 环境已验证这组参数可完成构建。如果你的 CANN 样例使用其他参数名或架构名，可以改 `NPU_ARCH` 或 `ASC_ARCH_FLAG`。

编译：

```bash
make -j
```


### 2.1 安装 AICPU 自定义 kernel 包

`make -j` 现在会额外生成 AICPU package：

```text
build/aicpu_aiv_aicpu_bench.tar.gz
```

这个 tar 包内部包含：

```text
aicpu_kernels_device/libaiv_aicpu_poll_kernel.so
```

需要把该包和 JSON 安装到 CANN 自定义 AICPU 目录，并把包登记到 `ascend_package_load.ini`：

```bash
make install_aicpu_package
```

作用：把文件安装到：

```text
$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/kernel/aicpu_aiv_aicpu_bench.tar.gz
$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
```

并向下面的文件追加白名单项：

```text
$ASCEND_HOME_PATH/conf/ascend_package_load.ini
```

追加内容类似：

```text
name:aicpu_aiv_aicpu_bench.tar.gz
install_path:2
optional:true
package_path:opp/vendors/cust/aicpu/kernel
load_as_per_soc:false
```

如果 CANN 安装在 `/usr/local/Ascend/cann-9.0.0` 且当前用户没有写权限，需要用 root 执行，或用 `sudo`：

```bash
sudo -E make install_aicpu_package
```

如果运行时日志仍提示包验签失败，需要按 HCOMM 自定义算子文档关闭自定义 AICPU 包验签。以 device 0 为例：

```bash
npu-smi set -t custom-op-secverify-enable -i 0 -d 1
npu-smi set -t custom-op-secverify-mode -i 0 -d 0
```

注意：关闭验签有安全风险，只建议在受控验证环境使用。

安装后，建议使用安装后的 JSON 路径运行：

```bash
--aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
```

作用：并行编译 host 程序、AIV `.asc` kernel 和 AICPU kernel 动态库，并生成 AICPU kernel 描述 JSON。`-j` 表示使用多核并行编译。

构建成功后，查看 build 目录产物：

```bash
ls
```

作用：确认关键产物是否生成。重点看：

```text
aiv_aicpu_bench
libaiv_aicpu_poll_kernel.so
libaiv_aicpu_poll_kernel.json
```

含义：

- `aiv_aicpu_bench`：最终运行的 benchmark 可执行文件。
- `libaiv_aicpu_poll_kernel.so`：AICPU 侧 `PollFlags`、`StampOnly`、`StampFlag` kernel 动态库。
- `libaiv_aicpu_poll_kernel.json`：AICPU kernel 描述文件，运行时通过 `--aicpu-json` 传给程序加载。

## 3. 先做最小冒烟

目的：先确认程序能跑、AICPU kernel 能加载、三条路径都不报错。

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=2 \
  --elements=4096 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=3 \
  --mode=all \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：在 device 0 上跑一个很小规模的测试。`tasks=2` 表示 2 个阶段；`elements=4096` 表示每阶段计算量很小；`warmup=1` 预热 1 轮；`iters=3` 正式统计 3 轮；`--mode=all` 表示三条路径都跑；`--aicpu-json` 指向当前 build 目录生成的 AICPU kernel JSON。

重点观察每轮：

```text
shared status=0
event_sync_pure status=0
event_sync_with_flag_check status=0
```

如果 `shared status=2`，说明方案一 AICPU 轮询 flag 超时，AICPU 没看到 AIV 写的 flag。

如果 `event_sync_with_flag_check status=1`，说明 event 后 AICPU 没读到预期 flag，需要查 AIV 写 flag 或内存可见性。

如果 `shared` 路径失败，可以先单独验证纯 event 路径：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=2 \
  --elements=4096 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=3 \
  --mode=event_sync_pure \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：只跑 `event_sync_pure`，不启动 `shared` 的 AICPU 轮询 kernel。这个命令用于判断基础 AIV kernel、event wait 和 AICPU `StampOnly` launch 是否能跑通。

如果 `event_sync_pure` 也失败，可以进一步只验证 AICPU 空 kernel：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=2 \
  --elements=4096 \
  --tile=1024 \
  --repeat=1 \
  --warmup=1 \
  --iters=3 \
  --mode=aicpu_noop \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：只启动 AICPU `Noop` kernel，不启动 AIV kernel，也不访问 device memory。若这个模式仍失败，说明问题在 AICPU 自定义 kernel 的加载/launch/打包路径；若这个模式成功而 `event_sync_pure` 失败，说明问题更可能在 AICPU kernel 访问 device memory。

## 4. 调整到目标计算量

冒烟通过后，逐步调大 `elements` 或 `repeat`，把单阶段 AIV 计算时间调到你关心的 100us 量级。

中等规模测试：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=65536 \
  --tile=1024 \
  --repeat=1 \
  --warmup=2 \
  --iters=10 \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：把阶段数提高到 4，把每阶段元素数提高到 65536，并正式统计 10 轮。这个规模用于观察三条路径是否稳定、耗时是否开始拉开。

更大计算量测试：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=2 \
  --iters=10 \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：继续增大每阶段 AIV 计算量，目标是让单阶段计算时间接近真实关注的 100us 量级。这样更接近“计算期间 AICPU 能否提前感知中间结果”的目标场景。

如果还不够，再调 `repeat`：

```bash
--repeat=2
--repeat=4
```

作用：`repeat` 会让每个阶段重复执行计算流程，用来粗粒度增加单阶段 AIV 计算时间。实际运行时要把它放回完整命令里，例如 `--repeat=2`。

## 5. 主要看哪些结果

每轮会打印：

```text
shared
event_sync_pure
event_sync_with_flag_check
```

含义：

- `shared`：方案一，共享内存 flag 轮询。
- `event_sync_pure`：方案二纯 event 版本，不写/读 flag，是主对比对象。
- `event_sync_with_flag_check`：方案二保留校验版本，额外写/读 flag，用来观察校验路径开销。

summary 会打印：

```text
shared_avg_us
event_sync_pure_avg_us
event_sync_with_flag_check_avg_us
speedup_event_pure_over_shared
speedup_event_flag_check_over_shared
```

主结论看这两个：

```text
shared_avg_us
event_sync_pure_avg_us
```

以及：

```text
speedup_event_pure_over_shared
```

含义是：

```text
speedup_event_pure_over_shared = event_sync_pure_avg_us / shared_avg_us
```

如果大于 1，说明 shared flag 方案更快。

如果小于 1，说明纯 event 方案更快。

`event_sync_with_flag_check_avg_us` 只作为辅助参考，用来观察额外写 flag、读 flag、校验 flag 带来的开销。

## 6. 建议测试矩阵

建议至少跑这些组合。下面这些不是单独可执行命令，而是替换到完整 `./aiv_aicpu_bench ...` 命令里的参数组合。

```bash
# N=2，小规模
--tasks=2 --elements=65536 --repeat=1

# N=4，常规
--tasks=4 --elements=65536 --repeat=1

# N=8，更细粒度
--tasks=8 --elements=65536 --repeat=1

# 增大单阶段计算
--tasks=4 --elements=262144 --repeat=1

# 或用 repeat 增大
--tasks=4 --elements=65536 --repeat=2
--tasks=4 --elements=65536 --repeat=4
```

作用：通过改变阶段数 `tasks` 和单阶段计算量 `elements/repeat`，观察共享内存同步和纯 event 同步在不同粒度下的开销变化。

每组建议：

```bash
--warmup=2 --iters=10
```

作用：预热 2 轮，正式统计 10 轮。适合快速看趋势。

如果结果波动大，可以改：

```bash
--warmup=5 --iters=30
```

作用：增加预热和统计轮数，降低偶发调度波动对平均值的影响。

## 7. 一条推荐正式命令

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=4 \
  --elements=262144 \
  --tile=1024 \
  --repeat=1 \
  --warmup=5 \
  --iters=30 \
  --timeout-ms=5000 \
  --aicpu-json=./libaiv_aicpu_poll_kernel.json
```

作用：在冒烟通过后，用较多预热和统计轮数跑一组相对正式的数据。`--timeout-ms=5000` 表示方案一 AICPU 轮询最多等 5000ms，避免共享 flag 不可见时无限卡住。

先确认 `status=0`，再比较 summary。
