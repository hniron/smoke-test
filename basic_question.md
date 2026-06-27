# Basic Questions

## so 是什么

**问题：** 而是 AICPU 执行侧打开这个 so 失败，这个so是什么

**回答：**

这个 so 是我们自己编出来的 **AICPU 自定义 kernel 动态库**：

```text
libaiv_aicpu_poll_kernel.so
```

它由这个源文件编译生成：

```text
aiv_aicpu_bench/aicpu_poll_kernel.cc
```

里面包含这些 AICPU kernel 入口函数：

```cpp
PollFlags
StampFlag
StampOnly
Noop
```

对应关系是：

- `PollFlags`：方案一共享内存 flag 轮询
- `StampFlag`：方案二 event 后再检查 flag 的版本
- `StampOnly`：纯 event 版本，只打时间戳，不检查 flag
- `Noop`：隔离测试 kernel，什么都不做，只验证 AICPU kernel 能不能被成功启动

构建后在 `build` 目录下会生成：

```text
build/libaiv_aicpu_poll_kernel.so
build/libaiv_aicpu_poll_kernel.json
```

其中 JSON 告诉 CANN runtime：

```json
"functionName": "Noop",
"kernelSo": "libaiv_aicpu_poll_kernel.so",
"opKernelLib": "AICPUKernel"
```

意思是：启动 `Noop` 这个 AICPU kernel 时，到 `libaiv_aicpu_poll_kernel.so` 里找 `Noop` 函数入口。

现在日志里的：

```text
open so failed
Get PollFlags api from libaiv_aicpu_poll_kernel.so failed
```

就是说 AICPU 执行侧想打开这个动态库，但没有打开成功，所以连里面的 `PollFlags/Noop` 函数都拿不到。

## 当前环境是否执行不了这个 so

**问题：** 你的意思是，当前环境上的AICPU执行不了当前测试代码中的so文件对吧

**回答：**

对，准确说是：

当前环境里的 **AICPU 执行侧没有成功打开/加载我们测试代码生成的 `libaiv_aicpu_poll_kernel.so`**，所以 AICPU kernel 没有真正执行起来。

从日志看，失败点不是 `Noop/PollFlags` 函数内部逻辑，而是在进入函数之前：

```text
Get Noop api from libaiv_aicpu_poll_kernel.so failed
errcode:11002, msg:open so failed
```

也就是说：

1. Host 侧能构建出这个 so。
2. Host 侧 `nm` 能看到 `Noop` 符号。
3. Host 侧 `ldd` 没有 `not found`。
4. 但是 AICPU 执行环境无法打开这个 so。

所以当前不是“测试同步逻辑跑失败”，而是更前置的 **自定义 AICPU so 没有被 AICPU runtime 正确加载**。需要先解决 so 的 AICPU package/安装/下发路径问题。

## AICPU 打不开 so 的可能原因

**问题：** 好的，你觉得当前AICPU打不开这个so，原因可能是什么呢

**回答：**

我判断最可能的原因是：**这个 so 只是生成在 host 的 build 目录里，但没有按 CANN AICPU 运行时要求，被打包/安装/下发到 AICPU 执行侧可访问的位置。**

按可能性排序：

1. **没有进入 AICPU package**

   日志里 CANN 启动时会加载：

   ```text
   aicpu_hccl.tar.gz
   aicpu_hcomm.tar.gz
   Ascend-aicpu_legacy.tar.gz
   ```

   但没有看到我们的：

   ```text
   libaiv_aicpu_poll_kernel.so
   ```

   被下发。当前只是 `build/` 目录下有这个 so，AICPU 侧不一定能看到。

2. **`kernelSo` 只能写 so 名，不能写 host 绝对路径**

   我们试过：

   ```json
   "kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so"
   ```

   设备侧直接报：

   ```text
   So name ... is not invalid
   ```

   说明 AICPU runtime 不接受 host 绝对路径。它期望的是 package 内部可解析的 so 名称。

3. **AICPU 侧动态依赖不满足**

   虽然 host 上：

   ```bash
   ldd ./libaiv_aicpu_poll_kernel.so
   ```

   没有 `not found`，但这只说明 host 环境能找到依赖，不代表 AICPU 执行侧也能找到同版本的 `libstdc++/libgcc/libc` 等依赖。

4. **CANN AICPU 自定义 kernel 的安装格式不完整**

   现在我们只有：

   ```text
   libaiv_aicpu_poll_kernel.so
   libaiv_aicpu_poll_kernel.json
   ```

   但 CANN 可能要求它们进入特定目录、tar 包结构、配置目录，甚至需要通过类似 HCOMM/HIXL 的 AICPU kernel package 方式安装。

5. **`opKernelLib` 类型或 JSON 注册方式与当前 CANN 运行链路不匹配**

   目前用的是：

   ```json
   "opKernelLib": "AICPUKernel"
   ```

   这个和参考资料一致，所以不是第一怀疑点。但如果 package 方式解决后仍失败，可以再验证是否需要 `CUSTAICPUKernel` 或其他配置。

我的当前判断：**首要原因是 package/安装/下发路径问题，不是 `Noop`/`PollFlags` 代码逻辑问题。**
下一步应该参考 CANN/HCOMM/HIXL 的 AICPU kernel 打包方式，让这个 so 进入 AICPU runtime 能实际打开的位置。
当前 open so failed 的根因高度可能就是：我们的 so 没有按 CANN AICPU package 机制安装/登记，AICPU 侧无法从 package 中找到它。

## aicpu_noop 成功日志如何解读

**问题：** 好的，你先帮我把上面的aicpu_noop的测试结果给我解读下，就是刚刚发你的log，这个里面有很多参数以及测试结果怎么看

**回答：**

这次日志如下：

```text
./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=aicpu_noop --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_noop status=0 host_total_us=90.31 host_per_task_us=90.31 aicpu_seen_interval_avg_us=0
iter=1
aicpu_noop status=0 host_total_us=49.09 host_per_task_us=49.09 aicpu_seen_interval_avg_us=0
iter=2
aicpu_noop status=0 host_total_us=55.86 host_per_task_us=55.86 aicpu_seen_interval_avg_us=0
summary: aicpu_noop_avg_us=65.09
```

### 命令参数含义

- `--device=0`：使用第 0 张 NPU。
- `--tasks=2`：配置了 2 个阶段。注意在 `aicpu_noop` 模式下不会真正按阶段启动 AIV kernel，这个参数主要保留配置一致性。
- `--elements=4096`：每阶段元素数。`aicpu_noop` 不做 AIV 计算，所以这个参数本轮不会影响 `Noop` kernel 内部计算量。
- `--tile=1024`：AIV kernel 的 tile 大小。`aicpu_noop` 不启动 AIV kernel，所以这里也只是保留配置。
- `--repeat=1`：每阶段重复计算次数。`aicpu_noop` 不执行 AIV 计算，因此不会产生实际影响。
- `--warmup=1`：先跑 1 轮预热，不计入最终 summary。
- `--iters=3`：正式统计 3 轮，所以看到 `iter=0`、`iter=1`、`iter=2`。
- `--mode=aicpu_noop`：只启动 AICPU 的 `Noop` kernel，不启动 AIV kernel，不访问 flag，不访问 event，不访问 device memory。
- `--aicpu-json=.../libaiv_aicpu_poll_kernel.json`：使用安装到 CANN 自定义 AICPU 目录下的 JSON。这个路径很关键，说明这次不是直接使用 build 目录里的裸 JSON，而是走安装后的 AICPU package 路径。

### config 行怎么看

```text
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=...
```

这行是程序实际解析到的配置。重点看：

- `mode=aicpu_noop`：确认这次确实跑的是 AICPU 空 kernel 隔离测试。
- `iters=3 warmup=1`：确认正式统计 3 轮，前面还有 1 轮预热。
- `aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json`：确认使用的是安装后的 JSON。

### 每轮结果怎么看

每轮输出类似：

```text
aicpu_noop status=0 host_total_us=90.31 host_per_task_us=90.31 aicpu_seen_interval_avg_us=0
```

字段含义：

- `aicpu_noop`：当前运行路径名称。
- `status=0`：AICPU `Noop` kernel 返回成功。这是最关键字段。
- `host_total_us=90.31`：Host 侧观测到的本轮耗时，单位是微秒。这个时间包含 host 下发 AICPU kernel、等待 AICPU stream 同步完成等端到端开销。
- `host_per_task_us=90.31`：平均每个 task 的 host 侧耗时。`aicpu_noop` 实际只启动一个 AICPU 空 kernel，不按 `tasks=2` 分阶段执行，所以这里可以理解为本次 noop launch 的平均观测耗时，不用于分析 AIV 分阶段性能。
- `aicpu_seen_interval_avg_us=0`：AICPU 连续看到阶段/flag 的平均间隔。`aicpu_noop` 不看 flag，也不记录阶段时间戳，所以这里为 0 是正常的。

### summary 怎么看

```text
summary: aicpu_noop_avg_us=65.09
```

表示 3 轮正式统计的平均耗时：

```text
(90.31 + 49.09 + 55.86) / 3 ≈ 65.09 us
```

这个值可以粗略理解为当前环境下“启动并同步一个最小 AICPU 自定义 kernel”的 host 侧平均开销。它不是共享内存同步方案或 event 同步方案的最终性能数据，只是 AICPU 自定义 kernel launch 基线。

### 这次成功证明了什么

这次最重要的结论是：

```text
AICPU 侧已经能够打开并执行 libaiv_aicpu_poll_kernel.so 里的 Noop 入口。
```

因此之前的问题：

```text
open so failed
Get Noop api from libaiv_aicpu_poll_kernel.so failed
ret=507018
```

已经被 AICPU package/安装/白名单方案解决。

### 这次还不能证明什么

`aicpu_noop` 成功只证明 AICPU 自定义 kernel 的加载和最小 launch 路径已经打通。它还不能证明：

- AIV kernel 一定能正常执行。
- AICPU 一定能访问 AIV 写入的 device memory。
- shared flag 同步一定成功。
- event 同步一定成功。
- 两种同步方案的性能对比已经有效。

所以下一步应该继续依次测试：

```bash
--mode=event_sync_pure
--mode=event_sync_with_flag_check
--mode=all
```

如果这些模式再失败，问题就会从“so 加载问题”进入具体的同步路径或内存可见性问题。

## elements 变大但 aicpu_seen_interval_avg_us 变小的原因

**问题：** 你看下458行开始的log，我调大elements的值，怎么感觉aicpu_seen_interval_avg_us时间反而变小了？

**回答：**

从日志看，确实有这个现象：

```text
elements=8192   last aicpu_seen_interval_avg_us=24.92
elements=12288  last aicpu_seen_interval_avg_us=21.62
elements=16384  last aicpu_seen_interval_avg_us=23.46
```

但这里还不能说明“elements 越大，计算越快”。主要原因有几个：

1. `tasks=2` 时，`aicpu_seen_interval_avg_us` 实际只由 **1 个间隔** 算出来，统计太少，非常容易抖动。

2. `iters=3 warmup=1` 太少。从日志看每组都是第一轮最大，后面越来越小，说明还有明显的启动/缓存/调度预热效应。

3. 当前 elements 增量还太小：8192、12288、16384 对 AIV 来说可能还没大到压过 kernel launch、event、AICPU launch 这些固定开销。

4. `aicpu_seen_interval_avg_us` 不是纯 AIV 计算时间，它是 AICPU `StampOnly` 看到两个 stage 完成时间戳之间的间隔，里面混有 event、stream 调度、AICPU stamp kernel 启动等因素。

因此，当前这组数据主要体现的是固定开销和抖动，不适合用来校准 100us 单任务计算量。

建议下一轮不要小步调 elements，直接拉大一个数量级，并增加统计量：

```bash
./aiv_aicpu_bench \
  --device=0 \
  --tasks=8 \
  --elements=65536 \
  --tile=1024 \
  --repeat=1 \
  --warmup=5 \
  --iters=20 \
  --mode=event_sync_pure \
  --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
```

然后再试：

```text
elements=131072 repeat=1
elements=262144 repeat=1
```

如果还不到 100us，再固定 `elements=65536` 或 `131072`，调：

```text
repeat=2
repeat=4
repeat=8
```

判断标准不要看某一轮的 `last_*`，优先看多轮平均后的趋势。

## 当前测试环境上显示的时间字段是怎么来的

**问题：** 当前测试环境上显示的测试时间，例如 `aicpu_seen_interval_avg_us`，这个数据是怎么得到的？这个数据是直接写在 log 里面，然后通过脚本解析 log 得出来的？还是把这个数据写在内存里，然后通过 D2H 在主机上获取？这个数据是怎么算出来的，以及算这个数据的源头从哪里来的？

**回答：**

不是通过脚本解析 log 得出来的。程序运行时的路径是：

```text
AICPU kernel 内部打点
-> 时间戳写到 device memory 的 seenNs 数组
-> Host 等 stream 完成后 D2H 拷回 seenNs
-> Host 侧计算相邻时间戳间隔的平均值
-> 程序直接打印 aicpu_seen_interval_avg_us
```

也就是说，log 只是保存程序已经打印出来的结果；不是先把原始时间戳写进 log，再由脚本解析得到这个字段。

原始时间戳的源头在 AICPU 侧：

- `shared`：`PollFlags` 轮询到第 `i` 个 flag 后，执行 `seenNs[i] = NowNs()`。
- `event_sync_pure`：第 `i` 个 `StampOnly` AICPU kernel 执行时，执行 `seenNs[i] = NowNs()`。
- `event_sync_with_flag_check`：第 `i` 个 `StampFlag` AICPU kernel 读 flag 并打点，执行 `seenNs[i] = NowNs()`。

每轮测试结束后，Host 在 `CopyResult()` 里把 device 侧结果拷回主机：

```cpp
aclrtMemcpy(result->seenNs.data(), ..., buf.seenNs, ..., ACL_MEMCPY_DEVICE_TO_HOST);
aclrtMemcpy(result->pollIters.data(), ..., buf.pollIters, ..., ACL_MEMCPY_DEVICE_TO_HOST);
aclrtMemcpy(&result->status, ..., buf.status, ..., ACL_MEMCPY_DEVICE_TO_HOST);
```

之后 `PrintRun()` 会调用 `SeenIntervalsUs(result)` 和 `Average(intervals)` 计算并打印：

```text
for i = 1..N-1:
  如果 seenNs[i-1] 和 seenNs[i] 都非 0，并且 seenNs[i] >= seenNs[i-1]:
    interval_us = (seenNs[i] - seenNs[i-1]) / 1000.0

aicpu_seen_interval_avg_us = 所有 interval_us 的平均值
```

所以如果 `tasks=N`，理论上最多有 `N-1` 个相邻间隔参与平均。这个字段的本质是：

```text
aicpu_seen_interval_avg_us
= AICPU kernel 写入 seenNs[] 的相邻时间戳差值平均值
```

在 `event_sync_pure` 模式下，虽然没有写 flag、也没有轮询 flag，但每个 `StampOnly` kernel 仍会写一次 `seenNs[i]`。因此该模式下的 `aicpu_seen_interval_avg_us` 表示相邻两次 AICPU `StampOnly` kernel 执行到打点位置之间的平均时间间隔，不表示 flag 可见性，也不表示单次 event wait 的裸开销。

## event 时间是否使用 elapsed time 方式测量

**问题：** 对于 event 的时间测量方式，是使用了 elapsed time 的方式还是其他方式？

**回答：**

当前代码没有使用 `aclrtEventElapsedTime()` 这类 event elapsed time API。`event` 在测试里只作为同步依赖节点使用：

```cpp
aclrtRecordEvent(events[i], aivStream);
aclrtStreamWaitEvent(aicpuStream, events[i]);
```

因此当前打印出来的时间不是 event elapsed time，也不是单次 event wait 的裸耗时。

当前代码里的时间主要分两类：

1. `host_total_us`

   这个字段用 Host 侧的 `HostNowNs()` 计时。以 `event_sync_pure` 为例，起点在 Host 开始下发第一组 AIV/event/AICPU 流程之前，终点在最后 `aclrtSynchronizeStream(aivStream)` 和 `aclrtSynchronizeStream(aicpuStream)` 都完成之后。

   它覆盖的是整条路径的端到端耗时：

   ```text
   N 次 AIV kernel 下发和执行
   + N 次 aclrtRecordEvent
   + N 次 aclrtStreamWaitEvent
   + N 次 AICPU StampOnly kernel 下发和执行
   + 最后的 stream synchronize 等待
   ```

2. `aicpu_seen_interval_avg_us`

   这个字段来自 AICPU kernel 内部的 `NowNs()` 打点，也不是 event elapsed time。`event_sync_pure` 下，每个 `StampOnly` kernel 执行时都会写一次：

   ```cpp
   seenNs[taskIndex] = NowNs();
   ```

   Host 后面通过 D2H 拷回 `seenNs[]`，再计算相邻时间戳差值平均值。

所以结论是：

```text
当前 event 方案没有单独测 event 的 elapsed time。
event 只用于建立 AIV stream 到 AICPU stream 的同步依赖。
打印出来的时间要么是 HostNowNs() 的端到端时间，
要么是 AICPU NowNs() 打点后的相邻间隔。
```

因此 `event_sync_pure` 的 `aicpu_seen_interval_avg_us` 不能解释为“单次 event wait 耗时”或“aclrtEventElapsedTime 测出来的 event 时间”。它更准确地表示：相邻两次 AICPU `StampOnly` kernel 执行到打点位置之间的平均间隔。

## aicpu_seen_interval_avg_us 能否作为 shared 和 event_sync 的最终对比指标

**问题：** 当前测试的目的是测 `shared` 模式和 `event_sync` 模式的两种同步开销比较。但是在 `shared` 中，`aicpu_seen_interval_avg_us` 时间其实是 AIV kernel 中一个阶段执行完、写 flag、AICPU 轮询到这个 flag 的整个阶段平均时间；在 `event_sync` 中，`aicpu_seen_interval_avg_us` 时间其实是 AIV 的一个计算 kernel 执行完、event record、event wait、AICPU kernel 执行到打点的整个阶段平均时间。这样看，实际差距就是 shared 中写 flag/读 flag 和 event_sync 中 event record/event wait 之间的差距，AIV 计算过程都是一样的。那么用 `aicpu_seen_interval_avg_us` 比较，能实现最终想要的验证目的吗？

**回答：**

`aicpu_seen_interval_avg_us` 可以辅助验证，但不能作为唯一的最终对比指标。它更适合看“阶段完成后，AICPU 侧感知到阶段推进的平均节奏”，而不是纯同步开销。

你的拆解方向基本对，但需要补两个差异。

第一，`event_sync_pure` 里不只是 `event record + event wait`。它还包含：

```text
每个阶段单独启动一个 AIV kernel
每个阶段单独启动一个 AICPU StampOnly kernel
stream 上调度多组节点
```

所以两种模式的差距不只是：

```text
shared 写/读 flag  vs  event record/wait
```

更准确是：

```text
shared:
一个 AIV kernel 内部分阶段计算
+ 每阶段写 flag
+ 一个 AICPU PollFlags kernel 持续轮询并看到 flag

event_sync_pure:
N 个 AIV kernel 拆分执行
+ N 次 event record/wait
+ N 个 AICPU StampOnly kernel 启动并打点
```

第二，两个模式下 `aicpu_seen_interval_avg_us` 的语义不完全等价：

```text
shared:
相邻两次 AICPU 轮询看到 flag 的时间间隔

event_sync_pure:
相邻两次 AICPU StampOnly kernel 执行到打点位置的时间间隔
```

它们都能反映阶段通知节奏，但一个是“单个 AICPU kernel 轮询 flag”，另一个是“event 依赖 + 多次 AICPU kernel 启动”。

因此，如果最终目标是验证：

```text
共享 flag 方案是否比 event 拆 kernel 方案端到端同步代价更低
```

当前 benchmark 是可以支撑这个方向的，但主指标应该优先看：

```text
shared_avg_us vs event_sync_pure_avg_us
shared host_per_task_us vs event_sync_pure host_per_task_us
```

`aicpu_seen_interval_avg_us` 作为辅助指标，用来看 AICPU 侧感知阶段推进的节奏是否符合预期。

如果最终目标变成只想隔离比较：

```text
flag 写/读/轮询开销 vs event record/wait 裸开销
```

那当前 `aicpu_seen_interval_avg_us` 不够纯，需要新增更细的实验路径，例如把 AIV 计算设为空或极小、单独测 event 节点，或者额外引入 event elapsed time/API 做隔离测量。

## shared 模式下 shared_avg_us 和 host_per_task_us 的区别

**问题：** 在 `shared` 模式下，`shared_avg_us` 和 `host_per_task_us` 有什么区别么？

**回答：**

有区别，两个字段的统计口径不同。

在 `shared` 模式下，每一轮正式测试会得到一个 `host_total_us`，表示这一轮 `RunShared()` 的 Host 侧端到端总时间，覆盖：

```text
1 次 AICPU PollFlags kernel 下发/执行
+ 1 次 AIV aiv_multi_stage_add kernel 下发/执行
+ AIV 内部 N 个阶段计算和写 flag
+ AICPU 轮询看到所有 flag
+ aivStream/aicpuStream 同步完成
```

这一轮打印的 `host_per_task_us` 是：

```text
host_per_task_us = host_total_us / tasks
```

也就是把当前这一轮的端到端总时间按阶段数 `tasks=N` 平均摊开。

而 `shared_avg_us` 是 summary 里的字段，表示正式统计的 `iters` 轮 `shared` 的 `host_total_us` 平均值：

```text
shared_avg_us = average(shared 每轮 host_total_us)
```

例如：

```text
tasks=4, iters=3

iter=0 shared host_total_us=100 us host_per_task_us=25 us
iter=1 shared host_total_us=120 us host_per_task_us=30 us
iter=2 shared host_total_us=110 us host_per_task_us=27.5 us
```

那么：

```text
shared_avg_us = (100 + 120 + 110) / 3 = 110 us
```

如果想得到 summary 级别的平均每阶段时间，可以再算：

```text
shared_avg_us / tasks = 110 / 4 = 27.5 us
```

所以总结是：

```text
host_per_task_us:
单轮结果，把该轮 host_total_us 除以 tasks

shared_avg_us:
多轮 summary，把 iters 轮 host_total_us 求平均
```

一个是单轮每阶段摊分值，一个是多轮端到端总时间平均值。

## 当前测试能否用 msprof 并在 MindStudio 里看 profile

**问题：** 当前测试是否能使用 `msprof`，采集 profile 数据，然后在 MindStudio 里面看到？

**回答：**

可以。当前 `aiv_aicpu_bench` 是普通 ACL Runtime 程序，走的是 `aclInit`、`aclrtSetDevice`、`aclrtCreateStream`、AIV kernel launch、AICPU kernel launch 这一套流程，理论上可以按普通 Ascend 应用用 `msprof` 采集 profile，然后把 profile 输出目录导入 MindStudio 查看 timeline。

但要注意：当前代码本身不会自动生成 MindStudio profile，需要在测试平台上用 `msprof` 包住可执行文件运行。

示例方式如下，具体参数以测试平台 CANN 版本的 `msprof --help` 为准：

```bash
source /usr/local/Ascend/cann/set_env.sh

cd /home/allen/workdir/zhn/aiv_aicpu_bench/build

msprof --output=./prof_event_sync \
  --application="./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json"
```

MindStudio 里通常导入 `msprof` 输出的 profile 目录，例如 `PROF_xxx` 或指定的输出目录。

理论上能看到的内容包括：

```text
shared 模式：
- AIV stream 上的 aiv_multi_stage_add
- AICPU stream 上的 PollFlags
- 两个 stream 的重叠关系
- PollFlags 是否长时间运行等待 flag

event_sync_pure 模式：
- 多个 aiv_one_stage_add_no_flag
- event record / wait 相关依赖或同步节点
- 多个 AICPU StampOnly
- AIV/AICPU 两条 stream 的阶段性串接关系
```

但 profile 不一定能直接看到这些细节：

```text
- AICPU 内部每次轮询 flag 的细节
- AIV 写 flag 的单条指令级耗时
- aicpu_seen_interval_avg_us 的计算过程
- 单纯 event record + event wait 的裸开销
```

`aicpu_seen_interval_avg_us` 仍然是程序自己通过 AICPU 写入 `seenNs[]`、Host D2H 拷回后计算并打印的指标；MindStudio profile 更多是从系统、stream、task 维度观察真实执行时间线。

建议 profile 时每次只跑一个模式，例如：

```text
--mode=shared
--mode=event_sync_pure
```

并把 `iters` 设置小一点，例如 `1~3`，否则 timeline 会很长，不容易看清每个阶段。

## ACL Runtime、msprof、profile 和 MindStudio timeline 是什么

**问题：** 当前 `aiv_aicpu_bench` 是普通 ACL Runtime 程序，走的是 `aclInit`、`aclrtSetDevice`、`aclrtCreateStream`、AIV kernel launch、AICPU kernel launch 这一套流程，理论上可以按普通 Ascend 应用用 `msprof` 采集 profile，然后把 profile 输出目录导入 MindStudio 查看 timeline。这段话中，有很多概念不清楚。ACL Runtime 程序是什么？ACL 和 Runtime 分别是什么？`aclInit`、`aclrtSetDevice`、`aclrtCreateStream` 是什么，是标准流程吗？`aclrt` 是 ACL Runtime 的缩写吗？普通 Ascend 应用是什么？`msprof` 是个应用吗？采集 profile 中的 profile 是什么？MindStudio 也是个应用吗？timeline 是什么？

**回答：**

`ACL Runtime 程序` 可以理解为：一个运行在 Host CPU 上的 C/C++ 程序，通过 Ascend 的 CANN/ACL 接口去控制 NPU。

当前 `aiv_aicpu_bench` 就是这种程序：

```text
Host CPU 上运行 ./aiv_aicpu_bench
-> 调 ACL Runtime API
-> 选择 NPU
-> 创建 stream
-> 下发 AIV kernel
-> 下发 AICPU kernel
-> 等待执行完成
```

`ACL` 一般指 Ascend Computing Language，是 CANN 提供给应用程序使用 Ascend 设备的一套接口。它不是某一个 kernel，也不是硬件本身，而是 Host 侧调用 NPU 的 API 层。

`Runtime` 指运行时层，负责设备选择、stream、memory、kernel/task 下发、同步等。`aclrt` 基本可以理解为 `ACL Runtime` 的缩写，所以这些 API 名字都是运行时相关接口：

```text
aclInit              初始化 ACL 环境
aclrtSetDevice       选择/设置使用哪张 Ascend device
aclrtCreateStream    创建一个 stream，也就是设备侧任务队列
```

这基本是标准流程。典型顺序是：

```text
aclInit
-> aclrtSetDevice
-> aclrtCreateStream
-> aclrtMalloc / aclrtMemcpy
-> kernel launch
-> aclrtSynchronizeStream
-> 释放资源
```

`stream` 可以理解成设备上的任务队列。同一个 stream 里的任务按顺序执行；不同 stream 之间可以并发，也可以通过 event 建立依赖。当前测试里有两条主要 stream：

```text
aivStream    放 AIV kernel
aicpuStream  放 AICPU kernel
```

`普通 Ascend 应用` 的意思是：不是 MindStudio 内部特殊程序，也不是只能由框架运行的模型，而是一个普通可执行程序，只是它会通过 CANN/ACL 使用 Ascend NPU。例如：

```bash
./aiv_aicpu_bench ...
```

就是普通 Ascend 应用。

`msprof` 是 CANN 提供的命令行性能采集工具，可以理解成一个 profiling 工具程序，类似 Linux `perf` 或 NVIDIA 的 `nsys/nvprof` 这类工具。它不是业务程序本身，而是用来运行或包住业务程序，并采集性能数据的工具。

`profile` 在这里不是“用户画像”，而是性能数据。它可以包含：

```text
哪个 stream 上跑了哪些 task
每个 kernel 什么时候开始、什么时候结束
AIV/AICPU kernel 是否重叠
Memcpy 花了多久
event/wait 依赖关系大概是什么样
Host API 调用情况
```

`MindStudio` 是华为 Ascend 的图形化开发、调试、性能分析 IDE，可以理解成一个桌面应用。可以把 `msprof` 采集出来的 profile 数据导入 MindStudio，用图形界面查看。

`timeline` 就是时间线视图。它通常可以理解成：

```text
时间轴: 0us ---- 50us ---- 100us ---- 150us

aivStream:   [AIV kernel 0] [AIV kernel 1] [AIV kernel 2]
aicpuStream:      [AICPU 0]      [AICPU 1]      [AICPU 2]
```

通过 timeline 可以观察：

```text
AIV 和 AICPU 有没有重叠
event_sync 模式是不是拆成了多个 kernel
shared 模式里 PollFlags 是否和 AIV kernel 同时运行
哪个 kernel 或 stream 等待时间长
```

所以前面那句话可以翻译成白话：

```text
当前 benchmark 是一个普通 C++ 可执行程序。
它通过 Ascend 官方 ACL Runtime API 控制 NPU。
因此可以用 CANN 的 msprof 工具采集它运行时的性能数据。
采集出来的数据可以导入 MindStudio，用 timeline 方式查看各个 stream、kernel、event 的执行关系。
```

## shared 模式下 100ns 量级 aicpu_seen_interval_avg_us 是否等于 HBM 读 flag 时延

**问题：** 在 `log.md` 648 行开始的日志里，当 `elements=1024` 时，`shared` 模式下 `aicpu_seen_interval_avg_us` 大概在 0.07~0.13us，也就是 100ns 量级。这个已经接近介质时延了，要么就是 AICPU 轮询 flag 时没走 HBM。这个 100ns 的时延合理吗？当前 AICPU 获取 flag 的方式是不是真的从 HBM 中读取的？还是从 AIV 的 cache 或其他地方读取到的？

**回答：**

这个 100ns 量级不能直接解释成“每阶段 AIV 计算 + 写 flag + AICPU 从 HBM 读 flag 的真实同步延迟”。更合理的解释是：当 `elements=1024, tile=1024, repeat=1` 时，AIV 每个 stage 只做一个 tile 的 `CopyIn + Add + CopyOut`，阶段非常短，AICPU 很可能没有实时逐阶段追上 flag，而是在多个 flag 已经写好之后，连续读到这些 ready flag。

当前 shared 流程是：

```text
Host 先下发 AICPU PollFlags
Host 再下发 AIV aiv_multi_stage_add

AIV:
  stage0 compute -> 写 flag0
  stage1 compute -> 写 flag1
  ...

AICPU:
  轮询 flag0，看到后打点 seenNs[0]
  轮询 flag1，看到后打点 seenNs[1]
  ...
```

如果 AIV 很快把多个 flag 都写完了，那么 AICPU 后面可能是：

```text
读 flag0，已经 ready，打点
读 flag1，已经 ready，打点
读 flag2，已经 ready，打点
...
```

这时：

```text
seenNs[1] - seenNs[0]
```

更接近：

```text
AICPU 从检查 flag0 切到检查 flag1 的软件循环/打点间隔
```

而不是：

```text
AIV stage1 真实计算耗时
+ AIV 写 flag 到 HBM
+ AICPU 从 HBM 读取 flag
```

从日志也能看出这个差异：`shared host_total_us` 大概 40us，`host_per_task_us` 大概 5us，但 `aicpu_seen_interval_avg_us` 只有 0.1us。它们明显不是同一个物理阶段时间。

代码层面能确认的是：

```text
flags 由 aclrtMalloc 分配，是 device memory
AIV 用 GlobalTensor/GM DataCopy 写 flagGm_
AICPU 用 param->flags[i * stride] 读同一块 device memory 地址
```

所以程序语义上，AICPU 读的是同一块 device memory/GM 地址。但是这不等于每一次 AICPU load 都一定真实打到 HBM 介质。

**问题：** 即使 AICPU 连续看到已经 ready 的多个 flag，那 AICPU 也要一个一个从 HBM 中把这些 flag 读上来，这个时间也应该是 AICPU 从 HBM 中读取 flag 的时间吧？

**回答：**

AICPU 确实是在按地址一个一个读 flag，但关键区别是：

```text
读的是 HBM-backed device memory 地址
!= 每一次 load 都一定真的访问 HBM 介质
```

类比 CPU：`malloc` 出来的内存物理上在 DDR，但 CPU 连续读时不一定每次都访问 DDR，可能命中 L1/L2/L3 cache。AICPU 这里也类似，软件看到的是 device memory/GM 地址，但实际 load 可能经过 cache、一致性路径、L2、预取等机制。

当前 AICPU 读 flag 的代码是：

```cpp
const uint32_t value = param->flags[i * param->flagStride];
```

它只能说明 AICPU 发起了对 `flags` 地址的 load，不能证明这个 load 每次都从 HBM 介质取数。

100ns 不能直接当作 HBM 读取时间，主要有几个原因。

第一，flag 之间距离很近：

```text
flagStride = 8 个 uint32_t = 32 bytes
flag0 offset = 0
flag1 offset = 32B
flag2 offset = 64B
...
```

如果 cache line 是 64B 或更大，读 `flag0` 时可能已经把 `flag1` 所在 cache line 带上来了。后续读 `flag1` 就可能是 cache hit，而不是 HBM miss。

第二，AICPU 之前一直在轮询。轮询会反复 load 某些地址，这可能让相关 cache line 保持热状态。等它切到后续 flag 时，数据可能已经在更近的层级里。

第三，`aicpu_seen_interval_avg_us` 测的不是单次 load latency。代码逻辑是：

```cpp
value = flags[i * stride];
if (value >= expected) {
    seenNs[i] = NowNs();
    pollIters[i] = iters;
}
```

所以：

```text
seenNs[i+1] - seenNs[i]
```

实际包含：

```text
记录第 i 个 flag 的时间戳
-> 进入下一轮循环
-> load 第 i+1 个 flag
-> 判断 ready
-> 再调用 NowNs 打点
```

如果第 `i+1` 个 flag 已经 ready，而且 load 命中 cache，那么这个间隔就会很小。

因此当前结论是：

```text
AICPU 读的是 aclrtMalloc 分配的 device memory 地址；
但这份日志不能证明每次 load 都真实访问 HBM。
0.1us 更像 AICPU 连续读取已 ready flag 时的循环/打点间隔，
不能直接解释为 HBM 介质读取延迟。
```

如果要进一步验证是不是每次真的从 HBM 读，可以做几个实验：

```text
1. 打印 pollIters[]
   如果后续 flag 的 pollIters 大量是 1，说明 AICPU 读到它们时这些 flag 已经 ready。

2. 把 flagStride 改大
   例如从 32B 改到 4KB 或 64KB，避免同 cache line 或预取影响。

3. 做 AICPU 读延迟 microbenchmark
   让 AICPU 随机读很大一片 device memory，尽量避免 cache 命中，再和当前顺序读 flag 对比。

4. 查平台是否支持 uncached device memory 或 cache invalidate/fence
   如果能强制 uncached，再测才更接近 HBM 访问。
```

