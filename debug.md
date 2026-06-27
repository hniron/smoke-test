# Debug Notes

## 问题现象

在 910B3 + CANN 9.0.0 环境中，`aiv_aicpu_bench` 可以完成构建：

```bash
source /usr/local/Ascend/cann-9.0.0/set_env.sh
cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
make -j
```

但是运行时失败，典型错误为：

```text
FAIL line 421: aclrtSynchronizeStream(aicpuStream) ret=507018
FAIL line 470: aclrtSynchronizeStream(aicpuStream) ret=507018
FAIL line 434: aclrtSynchronizeStream(aicpuStream) ret=507018
```

其中 `507018` 对应：

```text
ACL_ERROR_RT_AICPU_EXCEPTION
```

也就是 AICPU kernel 执行异常。

## 隔离方法

为了确认问题是否来自共享内存 flag 同步、event 同步、AIV kernel 或 device memory 访问，新增了 `aicpu_noop` 隔离模式。

运行命令：

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

`aicpu_noop` 只启动 AICPU `Noop` kernel：

- 不启动 AIV kernel
- 不访问 flag
- 不访问 AIV 输出
- 不等待 event
- 不读写 device memory

因此，如果 `aicpu_noop` 仍然失败，问题就不在同步算法本身，而在 AICPU 自定义 kernel 的加载/启动/执行链路。

实际结果：

```text
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 434: aclrtSynchronizeStream(aicpuStream) ret=507018
```

说明最小 AICPU kernel 也没有跑通。

## 基础检查

执行以下命令确认 JSON、符号和 host 侧依赖：

```bash
cat ./libaiv_aicpu_poll_kernel.json
nm -D ./libaiv_aicpu_poll_kernel.so | grep Noop
ldd ./libaiv_aicpu_poll_kernel.so
```

结果显示：

```text
JSON 里有 Noop
nm 里有 0000000000000a54 T Noop
ldd 没有 not found
```

这说明：

- JSON 中已经注册了 `Noop`
- `libaiv_aicpu_poll_kernel.so` 中确实导出了 `Noop`
- host 侧可见的动态依赖没有缺失

因此问题不是 JSON 缺入口、so 没导出符号，或 host 侧普通动态依赖缺失。

## 日志定位方法

运行失败后，用最近 5 分钟日志过滤当前进程相关错误：

```bash
find /root/ascend/log /var/log/npu "$HOME/ascend/log" -type f -mmin -5 2>/dev/null \
  | xargs grep -n -E "aiv_aicpu_bench|libaiv_aicpu_poll_kernel|Noop|PollFlags|507018|11002|open so failed|AICPU|dlopen|undefined|exception" 2>/dev/null \
  | tail -n 100
```

注意不要直接从 `/` 根目录递归 grep，否则会扫到大量无关历史日志。

有效日志中出现：

```text
Get PollFlags api from libaiv_aicpu_poll_kernel.so failed.
Aicpu engine process failed, result[11002]
errcode:11002, msg:open so failed.
soName=libaiv_aicpu_poll_kernel.so, funcName=PollFlags
```

在 `aicpu_noop` 模式下出现：

```text
Get Noop api from libaiv_aicpu_poll_kernel.so failed.
errcode:11002, msg:open so failed.
soName=libaiv_aicpu_poll_kernel.so, funcName=Noop
```

这说明 AICPU 执行侧打开 `libaiv_aicpu_poll_kernel.so` 失败，导致无法取得 `PollFlags` 或 `Noop` 函数入口。

## 绝对路径试验

曾尝试把 JSON 中的：

```json
"kernelSo": "libaiv_aicpu_poll_kernel.so"
```

改成 build 目录绝对路径：

```json
"kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so"
```

重新清理、构建并确认 JSON 已生效后，再跑 `aicpu_noop`，仍然失败：

```text
FAIL line 434: aclrtSynchronizeStream(aicpuStream) ret=507018
```

设备侧日志显示：

```text
So name /home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so is not invalid. Please check!
Get Noop api from /home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so failed.
errcode:11002, msg:open so failed.
soName=/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so, funcName=Noop
```

结论：

- `kernelSo` 不能简单填写 host 侧绝对路径。
- AICPU 侧会认为这种 so name 不合法。
- CANN AICPU 运行时不是直接从 host build 目录打开该 so。

因此已将 `kernelSo` 恢复为：

```json
"kernelSo": "libaiv_aicpu_poll_kernel.so"
```

## 当前结论

目前已经排除：

- AIV kernel 运算问题
- 共享内存 flag 同步逻辑问题
- event 同步逻辑问题
- AICPU 访问 device memory 问题
- JSON 缺少 `Noop` 注册
- `.so` 缺少 `Noop` 导出符号
- host 侧 `ldd` 可见依赖缺失
- `kernelSo` 使用 host 绝对路径可以解决问题这一假设

当前核心问题是：

```text
AICPU 执行侧打不开 libaiv_aicpu_poll_kernel.so
```

也就是自定义 AICPU kernel so 没有通过 CANN 期望的方式进入 AICPU 执行环境，或者没有被 CANN AICPU package/安装机制正确识别和下发。

## 下一步方向

后续应重点处理 `libaiv_aicpu_poll_kernel.so` 的 AICPU package/安装问题，而不是继续修改同步逻辑。

从日志看，CANN 启动时会加载系统 AICPU 包，例如：

```text
Ascend-aicpu_legacy.tar.gz
aicpu_hccl.tar.gz
aicpu_hcomm.tar.gz
```

但没有看到我们的：

```text
libaiv_aicpu_poll_kernel.so
```

被作为 AICPU package 下发到 device 侧。

因此下一步需要参考 CANN/HCOMM/HIXL 中 AICPU kernel 的打包和安装方式，让 `libaiv_aicpu_poll_kernel.so` 和对应 JSON 进入 CANN AICPU 运行时能够识别的位置。
