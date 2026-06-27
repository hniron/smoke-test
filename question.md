好的，对于上面两种同步方案，他们的测试时间是分别如何计算的？方案一的测试时间，起点是什么，终点是什么？方案二的测试时间，起点是什么，终点是什么？

当前代码里，两种方案的主测试时间都是 **Host 侧端到端时间**，不是 NPU event 计时。

计时函数是 `HostNowNs()`，结果打印成 `host_total_us`。

**方案一：shared flag 轮询**

代码在 [bench_main.asc](/home/allen/workdir/zhn/aiv_aicpu_bench/bench_main.asc:370)。

起点：

```cpp
const uint64_t start = HostNowNs();
ret = LaunchAicpu(kernel.poll, aicpuStream, &poll, sizeof(poll));
```

也就是说，起点在 **Host 准备下发 AICPU PollFlags kernel 之前**。

随后执行：

```text
1. Host 下发 AICPU PollFlags 到 aicpuStream
2. Host 下发 AIV aiv_multi_stage_add 到 aivStream
3. AICPU kernel 开始轮询 flags
4. AIV kernel 内部分阶段计算，每阶段写 flag
5. AICPU 看到所有 flag 后退出
6. Host synchronize aivStream
7. Host synchronize aicpuStream
```

终点：

```cpp
CHECK_ACL(aclrtSynchronizeStream(aivStream));
CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
const uint64_t end = HostNowNs();
```

所以方案一 `host_total_us` 覆盖的是：

```text
AICPU kernel 下发
+ AIV kernel 下发
+ AICPU 等 flag 的忙等时间
+ AIV 多阶段计算和写 flag
+ 两个 stream 执行完成等待
```

不是单独某个 flag 的同步开销。

**方案二：event sync**

代码在 [bench_main.asc](/home/allen/workdir/zhn/aiv_aicpu_bench/bench_main.asc:399)。

起点：

```cpp
const uint64_t start = HostNowNs();
for (uint32_t i = 0; i < opt.tasks; ++i) {
```

也就是说，起点在 **Host 开始下发第一组 AIV kernel/event/AICPU kernel 之前**。

每个 task 执行：

```text
1. Host 下发一次 AIV aiv_one_stage_add 到 aivStream
2. Host 在 aivStream 上 record event
3. Host 在 aicpuStream 上 wait event
4. Host 下发一次 AICPU StampFlag 到 aicpuStream
```

所有 task 都下发完后：

```cpp
CHECK_ACL(aclrtSynchronizeStream(aivStream));
CHECK_ACL(aclrtSynchronizeStream(aicpuStream));
const uint64_t end = HostNowNs();
```

终点是 **所有 AIV kernel、所有 event wait、所有 AICPU StampFlag 都执行完成之后**。

所以方案二 `host_total_us` 覆盖的是：

```text
N 次 AIV kernel 下发和执行
+ N 次 event record
+ N 次 event wait
+ N 次 AICPU kernel 下发和执行
+ 两个 stream 执行完成等待
```

总结一下：

```text
方案一：
起点 = Host 准备下发 AICPU PollFlags 前
终点 = AIV stream 和 AICPU stream 都 synchronize 完成后

方案二：
起点 = Host 准备下发第一组 AIV/event/AICPU 流程前
终点 = 所有拆分 kernel、event、AICPU kernel 都执行完成后
```

另外 `aicpu_seen_interval_avg_us` 不是 Host 总时间，它来自 AICPU kernel 内部的 `NowNs()` 打点，用来看 AICPU 连续看到 flag 或连续执行 StampFlag 的间隔。

**输出字段含义补充**

`status` 表示本轮 AICPU kernel 的执行状态，是 Host 在本轮结束后从 device 侧 `status` buffer 拷回来的结果。

- `status=0`：成功。
- `status=1`：参数错误，或者 `event_sync_with_flag_check` 模式下 AICPU 没读到期望的 flag。
- `status=2`：`shared` 模式下 AICPU 轮询 flag 超时。

其中 `aicpu_noop` 模式没有拷回 `status` buffer，代码直接打印 `status=0`，只表示这个空 AICPU kernel launch 和 stream sync 成功。

`host_total_us` 表示 Host 侧观测到的本轮端到端耗时，单位是微秒。它由 `HostNowNs()` 记录起点和终点后相减得到，覆盖当前方案从 Host 开始下发相关 AICPU/AIV/event 节点，到最后 `aivStream` 和 `aicpuStream` 都 synchronize 完成的总时间。因此它包含 Host 下发、stream 调度、kernel 执行、event wait、AICPU 等 flag/打点以及最终同步等待等开销，不是单个 API、单个 event 或单个 flag 的裸同步时间。

`host_per_task_us` 表示按阶段数摊到每个 task 上的 Host 侧平均耗时。对 `shared`、`event_sync_pure`、`event_sync_with_flag_check`，代码打印的是：

```text
host_per_task_us = host_total_us / tasks
```

这里的 `tasks` 就是 `--tasks` 配置的阶段数 N。这个字段方便比较不同 `tasks` 配置下的平均阶段成本，但它只是端到端总时间的平均摊分，不代表每个 task 真正独立测得的执行时间。`aicpu_noop` 模式实际只启动一个空 AICPU kernel，不按 `tasks` 分阶段执行，所以该模式下代码直接打印 `host_per_task_us = host_total_us`。

