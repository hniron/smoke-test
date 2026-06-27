(base) [root@localhost build]# make -j
[  0%] Built target aicpu_kernel_json
[ 25%] Building CXX object CMakeFiles/aiv_aicpu_poll_kernel.dir/aicpu_poll_kernel.cc.o
[ 50%] Building ASC object CMakeFiles/aiv_aicpu_bench.dir/bench_main.asc.o
[ 75%] Linking CXX shared library libaiv_aicpu_poll_kernel.so
[ 75%] Built target aiv_aicpu_poll_kernel
/home/zhn/aiv_aicpu_bench/bench_main.asc:16:9: warning: 'GM_ADDR' macro redefined [-Wmacro-redefined]
#define GM_ADDR __gm__ uint8_t *
        ^
/usr/local/Ascend/cann-9.0.0/aarch64-linux/tikcpp/tikcfw/impl/utils/kernel_utils_macros.h:23:9: note: previous definition is here
#define GM_ADDR __gm__ uint8_t*
        ^
/home/zhn/aiv_aicpu_bench/bench_main.asc:338:26: warning: implicit conversion from 'int' to 'uint16_t' (aka 'unsigned short') changes value from 120000 to 54464 [-Wconstant-conversion]
    attr.value.timeout = 120000;
                       ~ ^~~~~~
2 warnings generated.
/home/zhn/aiv_aicpu_bench/bench_main.asc:16:9: warning: 'GM_ADDR' macro redefined [-Wmacro-redefined]
#define GM_ADDR __gm__ uint8_t *
        ^
/usr/local/Ascend/cann-9.0.0/aarch64-linux/tikcpp/tikcfw/impl/utils/kernel_utils_macros.h:23:9: note: previous definition is here
#define GM_ADDR __gm__ uint8_t*
        ^
/home/zhn/aiv_aicpu_bench/bench_main.asc:338:26: warning: implicit conversion from 'int' to 'uint16_t' (aka 'unsigned short') changes value from 120000 to 54464 [-Wconstant-conversion]
    attr.value.timeout = 120000;
                       ~ ^~~~~~
2 warnings generated.
/home/zhn/aiv_aicpu_bench/bench_main.asc:16:9: warning: 'GM_ADDR' macro redefined [-Wmacro-redefined]
#define GM_ADDR __gm__ uint8_t *
        ^
/usr/local/Ascend/cann-9.0.0/aarch64-linux/tikcpp/tikcfw/impl/utils/kernel_utils_macros.h:23:9: note: previous definition is here
#define GM_ADDR __gm__ uint8_t*
        ^
/home/zhn/aiv_aicpu_bench/bench_main.asc:338:26: warning: implicit conversion from 'int' to 'uint16_t' (aka 'unsigned short') changes value from 120000 to 54464 [-Wconstant-conversion]
    attr.value.timeout = 120000;
                       ~ ^~~~~~
2 warnings generated.
[100%] Linking ASC executable aiv_aicpu_bench
[100%] Built target aiv_aicpu_bench



(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --aicpu-json=./libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 421: aclrtSynchronizeStream(aicpuStream) ret=507018


(base) [root@localhost aiv_aicpu_bench]# rm -rf build
(base) [root@localhost aiv_aicpu_bench]# mkdir build
(base) [root@localhost aiv_aicpu_bench]# cd build
(base) [root@localhost build]# source /usr/local/Ascend/cann-9.0.0/set_env.sh
(base) [root@localhost build]# cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
-- System processer: aarch64
-- CMAKE_ASC_COMPILER: /usr/local/Ascend/cann-9.0.0/bin/bisheng
-- ASCEND_CANN_PACKAGE_LINUX_PATH: /usr/local/Ascend/cann-9.0.0/aarch64-linux
-- CMAKE_ASC_LLD_LINKER: /usr/local/Ascend/cann-9.0.0/aarch64-linux/ccec_compiler/bin/ld.lld
-- The CXX compiler identification is GNU 10.3.1
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /home/zhn/aiv_aicpu_bench/build
(base) [root@localhost build]# make -j
[ 25%] Built target aicpu_kernel_json
[ 25%] Building ASC object CMakeFiles/aiv_aicpu_bench.dir/bench_main.asc.o
[ 50%] Building CXX object CMakeFiles/aiv_aicpu_poll_kernel.dir/aicpu_poll_kernel.cc.o
[ 75%] Linking CXX shared library libaiv_aicpu_poll_kernel.so
[ 75%] Built target aiv_aicpu_poll_kernel
[100%] Linking ASC executable aiv_aicpu_bench
[100%] Built target aiv_aicpu_bench
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=./libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 470: aclrtSynchronizeStream(aicpuStream) ret=507018
(base) [root@localhost build]#


(base) [root@localhost build]# date
  --iters=3 --mode=aicpu_noop --aicpu-json=./libaiv_aicpu_poll_kernel.json
  find /root/ascend/log /var/log/npu "$HOME/ascend/log" -type f -mmin -5 2>/dev/null \
    | xargs grep -n -E "aiv_aicpu_bench|libaiv_aicpu_poll_kernel|Noop|507018|AICPU|dlopen|
Sat Jun 13 05:00:25 PM CST 2026
    undefined|exception" 2>/dev/null \
    | tail -n 100(base) [root@localhost build]#   ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=5 warmup=1 timeout_ms=5000 mode=all aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 464: aclrtSynchronizeStream(aicpuStream) ret=507018
(base) [root@localhost build]#   --iters=3 --mode=aicpu_noop --aicpu-json=./libaiv_aicpu_poll_kernel.json
-bash: --iters=3: command not found
(base) [root@localhost build]#   find /root/ascend/log /var/log/npu "$HOME/ascend/log" -type f -mmin -5 2>/dev/null \
>     | xargs grep -n -E "aiv_aicpu_bench|libaiv_aicpu_poll_kernel|Noop|507018|AICPU|dlopen|
>     undefined|exception" 2>/dev/null \
>     | tail -n 100
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:101:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.414.694 [version_verify.cpp:71][PeerVersionCheck][tid:901649] VersionVerify: Check client version info, server[1230], client[1230]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:102:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.414.700 [version_verify.cpp:92][ParseVersionInfo][tid:901649] VersionVerify: pass client version info success
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:103:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.414.703 [hdc_client.cpp:271][CheckHdcConnection][tid:901649] Service[2] create hdc successfully.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:104:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.414.712 [version_verify.cpp:117][SpecialFeatureCheck][tid:901649] VersionVerify: previous type[67], supported
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:105:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.414.749 [process_mode_manager.cpp:732][GetDeviceCheckCodeOnce][tid:901649] [TsdClient][deviceId=0] [sessionId=1] wait package info response
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:106:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.416.095 [process_mode_manager.cpp:362][SendAICPUPackageSimple][tid:901649] [TsdClient][deviceId=0] no equal to begin send file[/usr/local/Ascend/cann-9.0.0/opp/built-in/op_impl/aicpu/kernel/aicpu_hcomm.tar.gz] to [/home/HwHiAiUser/hdcd/device0/aicpu/901649_aicpu_hcomm.tar.gz]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:107:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.429.986 [process_mode_manager.cpp:382][SendAICPUPackageSimple][tid:901649] [TsdClient][deviceId=0] hdc send file[/usr/local/Ascend/cann-9.0.0/opp/built-in/op_impl/aicpu/kernel/aicpu_hcomm.tar.gz] to [/home/HwHiAiUser/hdcd/device0/aicpu/901649_aicpu_hcomm.tar.gz] success
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:108:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.430.007 [process_mode_manager.cpp:522][InitTsdClient][tid:901649] [TsdClient] deviceId[0] begin to init hdc client
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:109:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.433.609 [version_verify.cpp:39][SetVersionInfo][tid:901649] VersionVerify: send client version to server
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:110:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.434.064 [version_verify.cpp:71][PeerVersionCheck][tid:901649] VersionVerify: Check client version info, server[1230], client[1230]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:111:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.434.070 [version_verify.cpp:92][ParseVersionInfo][tid:901649] VersionVerify: pass client version info success
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:112:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.434.072 [hdc_client.cpp:271][CheckHdcConnection][tid:901649] Service[2] create hdc successfully.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:113:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.434.081 [version_verify.cpp:117][SpecialFeatureCheck][tid:901649] VersionVerify: previous type[67], supported
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:114:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:32.434.118 [process_mode_manager.cpp:732][GetDeviceCheckCodeOnce][tid:901649] [TsdClient][deviceId=0] [sessionId=1] wait package info response
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:115:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.468 [process_mode_manager.cpp:2342][LoadPackageToDeviceByConfig][tid:901649] load package:aicpu_hcomm.tar.gz to device:0 success
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:116:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.475 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:901649] begin to load package:cann-hccd-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:117:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.479 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:901649] current package package:cann-hccd-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:118:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.482 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:901649] begin to load package:cann-hcomm-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:119:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.484 [process_mode_manager.cpp:2277][SupportLoadPkg][tid:901649] current device chip:5 does not support package:cann-hcomm-compat.tar.gz
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:120:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.486 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:901649] current package package:cann-hcomm-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:121:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.488 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:901649] begin to load package:cann-tsch-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:122:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.491 [process_mode_manager.cpp:2277][SupportLoadPkg][tid:901649] current device chip:5 does not support package:cann-tsch-compat.tar.gz
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:123:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.493 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:901649] current package package:cann-tsch-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:124:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.495 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:901649] begin to load package:cann-udf-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:125:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.497 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:901649] current package package:cann-udf-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:126:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.511 [process_mode_manager.cpp:522][InitTsdClient][tid:901649] [TsdClient] deviceId[0] begin to init hdc client
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:127:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.435.625 [version_verify.cpp:39][SetVersionInfo][tid:901649] VersionVerify: send client version to server
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:128:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.033 [version_verify.cpp:71][PeerVersionCheck][tid:901649] VersionVerify: Check client version info, server[1230], client[1230]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:129:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.039 [version_verify.cpp:92][ParseVersionInfo][tid:901649] VersionVerify: pass client version info success
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:130:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.041 [hdc_client.cpp:271][CheckHdcConnection][tid:901649] Service[2] create hdc successfully.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:131:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.047 [process_mode_manager.cpp:581][ConstructOpenMsg][tid:901649] [TsdClient] tsd get process sign successfully, procpid[901649] signSize[48]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:132:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.053 [version_verify.cpp:117][SpecialFeatureCheck][tid:901649] VersionVerify: previous type[6], supported
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:133:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.436.090 [process_mode_manager.cpp:124][OpenProcess][tid:901649] [ProcessModeManager] deviceId[0] sessionId[1] rankSize[0], wait subprocess start response
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:134:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.630.300 [stub_process_mode_nowin.cpp:22][ProcessQueueForAdc][tid:901649] [TsdClient] it is unnecessary for current mode[0] to grant queue auth to aicpusd
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:135:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.630.304 [process_mode_manager.cpp:162][OpenProcess][tid:901649] [TsdClient][deviceId=0] [sessionId=1] start hccp and computer process success,whole process spent time as follows:phase1:[83]ms load sink package config to device,phase2:[0]ms load opkernel to device,phase3:[7474]ms load sink package to device,phase4:[0]ms send open message to device,phase5:[194]ms receive open message to device,phase6:[0]ms process for adc on host,whole duration:[7752]ms
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:136:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.630.327 [event_sched.c:1536][ascend][curpid:901649,901649][drv][event-sche][eschedDeviceOpen]The devId open success. (devId=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:137:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.634.740 [devmm_svm.c:157][ascend][curpid:901649,901649][drv][devmm][devmm_setup_device]DrvMemDeviceOpen. (devid=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:138:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.636.672 [devmm_virt_interface.c:462][ascend][curpid:901649,901649][drv][devmm][devmm_ioctl_init_process]Init_process details. (hostpid=901649; ret=0; cnt=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:139:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.636.945 [drv_share_log.c:240][ascend][curpid:901649,901649][drv][devmm][share_log_read_in_single_module]Setup device succeeded. (logical_devid=0; devid=0; vfid=0; hostpid=901649; devpid=31334)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:140:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.636.953 [devmm_svm.c:119][ascend][curpid:901649,901649][drv][devmm][devmm_set_device_info]Device info. (devid=0; dvpp_size=17179869184; support_bar_mem=1; support_dev_read_only=1; support_dev_mem_map_host=1; support_bar_huge_mem=1; allocated_by_malloc=0; host_rw_dev_ro=1)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:141:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.636.957 [devmm_svm.c:125][ascend][curpid:901649,901649][drv][devmm][devmm_set_device_info]Device info. (double_pgtable_offset=17592186044416; support_host_pin_pre_register=0; support_host_mem_pool=0; is_support_agent_giant_page=1; is_support_host_giant_page=1; support_remote_mmap=1; support_shmem_map_exbus=0; support_mem_host_uva=1)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:142:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:33.637.107 [queue_interface.c:655][ascend][curpid:901649,901649][drv][queuemng][queueDeviceOpen]queue open finish. (devId=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:143:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.637.180 [raw_device.cc:550] 901649 Init: isAddrFlat:0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:144:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.638.123 [tsd_client.cpp:188][TsdCapabilityGet][tid:901649] TsdCapabilityGet Begin.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:145:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.638.129 [process_mode_manager.cpp:1036][CapabilityGet][tid:901649] [ProcessModeManager] enter into CapabilityGet process deviceId[0] type[0].
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:146:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.638.136 [version_verify.cpp:117][SpecialFeatureCheck][tid:901649] VersionVerify: previous type[38], supported
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:147:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.638.189 [process_mode_manager.cpp:1059][CapabilityGet][tid:901649] [TsdClient][deviceId=0] [sessionId=1] [type=0]send capability successfully.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:148:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.639.268 [process_mode_msg_parse.cpp:233][PidQosMsgProc][tid:901649] [TsdClient] PidQosMsgProc recvMsg realDeviceId[0] msgType[39] localDevId[0] rspCode[0],pidqos[1]
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:149:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.639.291 [process_mode_manager.cpp:1064][CapabilityGet][tid:901649] [TsdClient][logicDeviceId_=0][type=0][pidQos=1][ret=0]recv capability response finished.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:150:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.639.325 [engine.cc:81] 901649 Engine: Constructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:151:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.639.330 [stars_engine.cc:49] 901649 StarsEngine: Constructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:152:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.655.279 [npu_driver_res.cc:209] 902056 GetDeviceStatus: GetDeviceStatus status=1.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:153:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.655.356 [runtime.cc:5840] 901649 CreateReportRasThread: Start report ras thread success!
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:154:[INFO] ATRACE(901649,aiv_aicpu_bench):2026-06-13-17:00:33.659.696 [tracer_mgr_operate.c:66](tid:901649) create object RUNTIME_ATRACE_DEV0_TS0 successfully, exitSave(false), noLock(false).
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:155:[INFO] TDT(901649,aiv_aicpu_bench):2026-06-13-17:00:33.659.711 [client_manager.cpp:213][SetProfilingCallback][tid:901649] [TsdClient] set profiling callback successfully
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:156:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.665.615 [runtime.cc:5725] 902058 SetWatchDogDevStatus: There is errInfo of devId=0, tsId=0
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:157:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.686.237 [stream_mem_pool.cc:360] 901649 ~PoolRegistry: SOMA PoolRegistry destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:158:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.686.254 [runtime_adapt.cc:56] 901649 ~Runtime: runtime destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:159:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.693.269 [runtime.cc:5881] 902057 ReportRasRun: Report ras thread exits normally!
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:160:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.693.434 [runtime.cc:5850] 901649 DestroyReportRasThread: Join report ras thread OK.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:161:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.693.442 [runtime.cc:1389] 901649 WaitMonitorExit: wait monitor success, used count=0.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:162:[INFO] ATRACE(901649,aiv_aicpu_bench):2026-06-13-17:00:34.706.641 [tracer_mgr_operate.c:248](tid:901649) destroy object RUNTIME_ATRACE_DEV0_TS0, exitSave(false).
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:163:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.116 [stars_engine.cc:56] 901649 ~StarsEngine: Destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:164:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.127 [engine.cc:87] 901649 ~Engine: Destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:165:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.167 [queue_interface.c:662][ascend][curpid:901649,901649][drv][queuemng][queueDeviceClose]queue close finish. (devId=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:166:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.172 [devmm_svm.c:2615][ascend][curpid:901649,901649][drv][devmm][drvMemDeviceCloseInner]DrvMemDeviceClose. (devid=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:167:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.178 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]HOST_MEM Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=11341824; alloc_cnt=3; free_cnt=3)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:168:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.183 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]HOST_MEM Cached_size:12582912Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:169:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.187 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=28672; allocated_peak_size=118784; alloc_cnt=14; free_cnt=7)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:170:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.191 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Mem stats (Bytes). (module_name=APP; module_id=33; current_alloced_size=126976; allocated_peak_size=126976; alloc_cnt=7; free_cnt=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:171:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.195 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Cached_size:1941504Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:172:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.198 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_DDR dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=16384; alloc_cnt=1; free_cnt=1)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:173:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.202 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_DDR dev0 Cached_size:2097152Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:174:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.209 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=20971520; alloc_cnt=6; free_cnt=6)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:175:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.212 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_HBM dev0 Cached_size:20971520Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:176:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.215 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_DDR dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=2097152; alloc_cnt=1; free_cnt=1)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:177:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.219 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_DDR dev0 Cached_size:2097152Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:178:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.222 [svm_mem_statistics.c:150][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_P2P_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=6291456; alloc_cnt=2; free_cnt=2)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:179:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.718.226 [svm_mem_statistics.c:156][ascend][curpid:901649,901649][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_P2P_HBM dev0 Cached_size:6291456Bytes
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:180:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.745.256 [drv_share_log.c:240][ascend][curpid:901649,901649][drv][devmm][share_log_read_in_single_module]Close logical_device. (logical_devid=0; phyid=0; vfid=0; hostpid=901649; devpid=31334)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:181:[INFO] DRV(901649,aiv_aicpu_bench):2026-06-13-17:00:34.757.597 [event_sched.c:1515][ascend][curpid:901649,901649][drv][event-sche][eschedDeviceClose]The devId close success. (devId=0)
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:182:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.850.318 [xpu_task_fail_callback_data_manager.cc:64] 901649 ~XpuTaskFailCallBackManager: XpuTaskFailCallBackManager Destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:183:[INFO] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:34.850.325 [task_fail_callback_data_manager.cc:98] 901649 ~TaskFailCallBackManager: Destructor.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:184:[INFO] ATRACE(901649,aiv_aicpu_bench):2026-06-13-17:00:34.850.398 [stacktrace_signal.c:194](tid:901649) unregister all signal handlers, can not capture signal.
/root/ascend/log/run/plog/plog-901649_20260613170025864.log:185:[INFO] ATRACE(901649,aiv_aicpu_bench):2026-06-13-17:00:35.941.279 [atrace_client_core.c:127](tid:901654) atrace receive thread exited, devId=0.
/root/ascend/log/debug/device-0/device-901649_20260613170036025.log:1:[ERROR] CCECPU(31334,aicpu_scheduler):2026-06-13-17:00:34.441.438 [ae_kernel_lib_aicpu.cc:177][CallKernelApi][tid:31346][AICPU_PROCESSER] Get PollFlags api from libaiv_aicpu_poll_kernel.so failed.
/root/ascend/log/debug/device-0/device-901649_20260613170036025.log:2:[ERROR] CCECPU(31334,aicpu_scheduler):2026-06-13-17:00:34.441.447 [aicpusd_event_process.cpp:906][PostProcessTsKernelTask][tid:31346] Aicpu engine process failed, result[11002], opName[null], streamId[45], taskId[0].
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:1:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.663.068 [stars_engine.cc:1548]902058 ProcLogicCqReport:Task run failed, device_id=0, stream_id=45, task_id=0, sqe_type=1(aicpu), errType=0x1(task exception), sqSwStatus=0x2afa
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:2:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.665.545 [device_error_proc.cc:1644]902058 ProcessStarsAicpuErrorInfo:The error from device(chipId:0, dieId:0), serial number is 103, an exception occurred during AICPU execution, stream_id:45, task_id:0, errcode:11002, msg:open so failed.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:3:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.665.610 [davinci_task.cc:209]902058 SetStarsResultForDavinciTask:AICPU Kernel task happen error, retCode=0x2a.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:4:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.044 [davinci_kernel_task.cc:1729]902058 PreCheckTaskErr:Kernel task happen error, retCode=0x2a, [aicpu exception].
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:5:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.070 [davinci_kernel_task.cc:1435]902058 PrintAicpuErrorInfo:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, soName=libaiv_aicpu_poll_kernel.so, funcName=PollFlags, kernelName=PollFlags, errorCode=0x2a.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:6:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.084 [davinci_kernel_task.cc:1445]902058 PrintAicpuErrorInfo:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, fault op_name=.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:7:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.112 [stream.cc:1472]901649 GetError:Stream Synchronize failed, stream_id=45, retCode=0x2a, [aicpu exception].
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:8:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.118 [stream.cc:1475]901649 GetError:AICPU Kernel task happen error, retCode=0x2a.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:9:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.143 [stream.cc:1475]901649 GetError:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, fault op_name=.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:10:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.155 [api_error.cc:1029]901649 StreamSynchronize:Stream synchronize failed, stream_id=45, timeout=-1ms.
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:11:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.168 [api_c_stream.cc:161]901649 rtStreamSynchronize:ErrCode=507018, desc=[aicpu exception], InnerCode=0x715002a
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:12:[ERROR] RUNTIME(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.174 [error_message_manage.cc:65]901649 FuncErrorReason:rtStreamSynchronize execution failed, reason=aicpu exception
/root/ascend/log/debug/plog/plog-901649_20260613170033666.log:13:[ERROR] ASCENDCL(901649,aiv_aicpu_bench):2026-06-13-17:00:33.666.644 [stream.cpp:139]901649 aclrtSynchronizeStreamImpl:synchronize stream failed, runtime result = 507018
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=aicpu_noop --aicpu-json=./libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 434: aclrtSynchronizeStream(aicpuStream) ret=507018
(base) [root@localhost build]# cat ./libaiv_aicpu_poll_kernel.json
{
    "PollFlags": {
        "opInfo": {
            "functionName": "PollFlags",
            "kernelSo": "libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "StampFlag": {
        "opInfo": {
            "functionName": "StampFlag",
            "kernelSo": "libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "StampOnly": {
        "opInfo": {
            "functionName": "StampOnly",
            "kernelSo": "libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "Noop": {
        "opInfo": {
            "functionName": "Noop",
            "kernelSo": "libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    }
}
(base) [root@localhost build]# nm -D ./libaiv_aicpu_poll_kernel.so | grep Noop
0000000000000a54 T Noop
(base) [root@localhost build]# ldd ./libaiv_aicpu_poll_kernel.so
        linux-vdso.so.1 (0x0000ffffb6cb8000)
        libstdc++.so.6 => /usr/lib64/libstdc++.so.6 (0x0000ffffb6a50000)
        libm.so.6 => /usr/lib64/libm.so.6 (0x0000ffffb69a0000)
        libgcc_s.so.1 => /usr/lib64/libgcc_s.so.1 (0x0000ffffb6960000)
        libc.so.6 => /usr/lib64/libc.so.6 (0x0000ffffb67b0000)
        /lib/ld-linux-aarch64.so.1 (0x0000ffffb6c7b000)
(base) [root@localhost build]#

(base) [root@localhost zhn]# cd aiv_aicpu_bench
(base) [root@localhost aiv_aicpu_bench]# rm -rf build
(base) [root@localhost aiv_aicpu_bench]# mkdir build
(base) [root@localhost aiv_aicpu_bench]# cd build
(base) [root@localhost build]# source /usr/local/Ascend/cann-9.0.0/set_env.sh
(base) [root@localhost build]# cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
-- System processer: aarch64
-- CMAKE_ASC_COMPILER: /usr/local/Ascend/cann-9.0.0/bin/bisheng
-- ASCEND_CANN_PACKAGE_LINUX_PATH: /usr/local/Ascend/cann-9.0.0/aarch64-linux
-- CMAKE_ASC_LLD_LINKER: /usr/local/Ascend/cann-9.0.0/aarch64-linux/ccec_compiler/bin/ld.lld
-- The CXX compiler identification is GNU 10.3.1
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /home/zhn/aiv_aicpu_bench/build
(base) [root@localhost build]# make -j
[ 50%] Building CXX object CMakeFiles/aiv_aicpu_poll_kernel.dir/aicpu_poll_kernel.cc.o
[ 25%] Built target aicpu_kernel_json
[ 50%] Building ASC object CMakeFiles/aiv_aicpu_bench.dir/bench_main.asc.o
[ 75%] Linking CXX shared library libaiv_aicpu_poll_kernel.so
[ 75%] Built target aiv_aicpu_poll_kernel
[100%] Linking ASC executable aiv_aicpu_bench
[100%] Built target aiv_aicpu_bench
(base) [root@localhost build]# cat ./libaiv_aicpu_poll_kernel.json
{
    "PollFlags": {
        "opInfo": {
            "functionName": "PollFlags",
            "kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "StampFlag": {
        "opInfo": {
            "functionName": "StampFlag",
            "kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "StampOnly": {
        "opInfo": {
            "functionName": "StampOnly",
            "kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    },
    "Noop": {
        "opInfo": {
            "functionName": "Noop",
            "kernelSo": "/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so",
            "opKernelLib": "AICPUKernel"
        }
    }
}
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=aicpu_noop --aicpu-json=./libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=./libaiv_aicpu_poll_kernel.json
FAIL line 434: aclrtSynchronizeStream(aicpuStream) ret=507018
(base) [root@localhost build]#

(base) [root@localhost build]# find /root/ascend/log /var/log/npu "$HOME/ascend/log" -type f -mmin -5 2>/dev/null     | xargs grep -n -E "aiv_aicpu_bench|libaiv_aicpu_poll_kernel|Noop|PollFlags|507018|11002| open so failed|AICPU|dlopen|undefined|exception" 2>/dev/null     | tail -n 100
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:48:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.661 [process_mode_manager.cpp:2219][LoadPackageConfigInfoToDevice][tid:1039509] Receive load package config response result:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:49:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.682 [process_mode_manager.cpp:231][IsSupportCommonSink][tid:1039509] mutexFile[/usr/local/Ascend/driver/lib64/driver/sink_file_mutex_0.cfg] found means supporting common sink
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:50:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.686 [process_mode_manager.cpp:249][LoadSysOpKernel][tid:1039509] [TsdClient][logicDeviceId_=0] use legacy package
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:51:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.812 [client_manager.cpp:289][CheckPackageExistsOnce][tid:1039509] [TsdClient][deviceId=0] pkg size[0] in path[/usr/local/Ascend/cann-9.0.0/opp/built-in/] must be only one, skip send package, packageType[6]
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:52:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.817 [process_mode_manager.cpp:254][LoadSysOpKernel][tid:1039509] [TsdClient][logicDeviceId_=0] can not find aicpu packages
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:53:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.832 [process_mode_manager.cpp:231][IsSupportCommonSink][tid:1039509] mutexFile[/usr/local/Ascend/driver/lib64/driver/sink_file_mutex_0.cfg] found means supporting common sink
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:54:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.854 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:Ascend-aicpu_legacy.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:55:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.085.860 [package_process_config.cpp:337][GetPkgHostAndDeviceDstPath][tid:1039509] get orgFile:/usr/local/Ascend/cann-9.0.0/opp/Ascend/aicpu/Ascend-aicpu_legacy.tar.gz, dstFile:/home/HwHiAiUser/hdcd/device0/aicpu/1039509_Ascend-aicpu_legacy.tar.gz
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:56:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.931 [process_mode_manager.cpp:2325][LoadPackageToDeviceByConfig][tid:1039509] current package:Ascend-aicpu_legacy.tar.gz is same as device, skip load
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:57:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.943 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:Ascend310P-aicpu_legacy.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:58:[WARNING] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.948 [process_mode_manager.cpp:2318][LoadPackageToDeviceByConfig][tid:1039509] cannot find package:Ascend310P-aicpu_legacy.tar.gz, optional is true skip
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:59:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.952 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:Ascend910-aicpu_legacy.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:60:[WARNING] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.954 [process_mode_manager.cpp:2318][LoadPackageToDeviceByConfig][tid:1039509] cannot find package:Ascend910-aicpu_legacy.tar.gz, optional is true skip
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:61:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.957 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:aicpu_hccl.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:62:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.389.967 [package_process_config.cpp:337][GetPkgHostAndDeviceDstPath][tid:1039509] get orgFile:/usr/local/Ascend/cann-9.0.0/opp/built-in/op_impl/aicpu/kernel/aicpu_hccl.tar.gz, dstFile:/home/HwHiAiUser/hdcd/device0/aicpu/1039509_aicpu_hccl.tar.gz
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:63:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.393.289 [process_mode_manager.cpp:2325][LoadPackageToDeviceByConfig][tid:1039509] current package:aicpu_hccl.tar.gz is same as device, skip load
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:64:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.393.296 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:aicpu_hcomm.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:65:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.393.302 [package_process_config.cpp:337][GetPkgHostAndDeviceDstPath][tid:1039509] get orgFile:/usr/local/Ascend/cann-9.0.0/opp/built-in/op_impl/aicpu/kernel/aicpu_hcomm.tar.gz, dstFile:/home/HwHiAiUser/hdcd/device0/aicpu/1039509_aicpu_hcomm.tar.gz
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:66:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.678 [process_mode_manager.cpp:2325][LoadPackageToDeviceByConfig][tid:1039509] current package:aicpu_hcomm.tar.gz is same as device, skip load
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:67:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.689 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:cann-hccd-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:68:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.693 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:1039509] current package package:cann-hccd-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:69:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.696 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:cann-hcomm-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:70:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.698 [process_mode_manager.cpp:2277][SupportLoadPkg][tid:1039509] current device chip:5 does not support package:cann-hcomm-compat.tar.gz
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:71:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.701 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:1039509] current package package:cann-hcomm-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:72:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.703 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:cann-tsch-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:73:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.705 [process_mode_manager.cpp:2277][SupportLoadPkg][tid:1039509] current device chip:5 does not support package:cann-tsch-compat.tar.gz
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:74:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.708 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:1039509] current package package:cann-tsch-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:75:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.710 [process_mode_manager.cpp:2309][LoadPackageToDeviceByConfig][tid:1039509] begin to load package:cann-udf-compat.tar.gz to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:76:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.712 [process_mode_manager.cpp:2311][LoadPackageToDeviceByConfig][tid:1039509] current package package:cann-udf-compat.tar.gz dose not need to load to device:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:77:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.722 [process_mode_manager.cpp:581][ConstructOpenMsg][tid:1039509] [TsdClient] tsd get process sign successfully, procpid[1039509] signSize[48]
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:78:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.731 [version_verify.cpp:117][SpecialFeatureCheck][tid:1039509] VersionVerify: previous type[6], supported
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:79:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.428.796 [process_mode_manager.cpp:124][OpenProcess][tid:1039509] [ProcessModeManager] deviceId[0] sessionId[1] rankSize[0], wait subprocess start response
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:80:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.627.408 [stub_process_mode_nowin.cpp:22][ProcessQueueForAdc][tid:1039509] [TsdClient] it is unnecessary for current mode[0] to grant queue auth to aicpusd
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:81:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.627.413 [process_mode_manager.cpp:162][OpenProcess][tid:1039509] [TsdClient][deviceId=0] [sessionId=1] start hccp and computer process success,whole process spent time as follows:phase1:[84]ms load sink package config to device,phase2:[0]ms load opkernel to device,phase3:[342]ms load sink package to device,phase4:[0]ms send open message to device,phase5:[198]ms receive open message to device,phase6:[0]ms process for adc on host,whole duration:[626]ms
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:82:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.627.437 [event_sched.c:1536][ascend][curpid:1039509,1039509][drv][event-sche][eschedDeviceOpen]The devId open success. (devId=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:83:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.631.992 [devmm_svm.c:157][ascend][curpid:1039509,1039509][drv][devmm][devmm_setup_device]DrvMemDeviceOpen. (devid=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:84:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.633.897 [devmm_virt_interface.c:462][ascend][curpid:1039509,1039509][drv][devmm][devmm_ioctl_init_process]Init_process details. (hostpid=1039509; ret=0; cnt=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:85:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.634.178 [drv_share_log.c:240][ascend][curpid:1039509,1039509][drv][devmm][share_log_read_in_single_module]Setup device succeeded. (logical_devid=0; devid=0; vfid=0; hostpid=1039509; devpid=5385)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:86:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.634.187 [devmm_svm.c:119][ascend][curpid:1039509,1039509][drv][devmm][devmm_set_device_info]Device info. (devid=0; dvpp_size=17179869184; support_bar_mem=1; support_dev_read_only=1; support_dev_mem_map_host=1; support_bar_huge_mem=1; allocated_by_malloc=0; host_rw_dev_ro=1)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:87:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.634.191 [devmm_svm.c:125][ascend][curpid:1039509,1039509][drv][devmm][devmm_set_device_info]Device info. (double_pgtable_offset=17592186044416; support_host_pin_pre_register=0; support_host_mem_pool=0; is_support_agent_giant_page=1; is_support_host_giant_page=1; support_remote_mmap=1; support_shmem_map_exbus=0; support_mem_host_uva=1)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:88:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.634.334 [queue_interface.c:655][ascend][curpid:1039509,1039509][drv][queuemng][queueDeviceOpen]queue open finish. (devId=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:89:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.634.406 [raw_device.cc:550] 1039509 Init: isAddrFlat:0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:90:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.635.234 [tsd_client.cpp:188][TsdCapabilityGet][tid:1039509] TsdCapabilityGet Begin.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:91:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.635.240 [process_mode_manager.cpp:1036][CapabilityGet][tid:1039509] [ProcessModeManager] enter into CapabilityGet process deviceId[0] type[0].
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:92:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.635.247 [version_verify.cpp:117][SpecialFeatureCheck][tid:1039509] VersionVerify: previous type[38], supported
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:93:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.635.294 [process_mode_manager.cpp:1059][CapabilityGet][tid:1039509] [TsdClient][deviceId=0] [sessionId=1] [type=0]send capability successfully.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:94:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.636.512 [process_mode_msg_parse.cpp:233][PidQosMsgProc][tid:1039509] [TsdClient] PidQosMsgProc recvMsg realDeviceId[0] msgType[39] localDevId[0] rspCode[0],pidqos[1]
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:95:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.636.535 [process_mode_manager.cpp:1064][CapabilityGet][tid:1039509] [TsdClient][logicDeviceId_=0][type=0][pidQos=1][ret=0]recv capability response finished.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:96:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.636.573 [engine.cc:81] 1039509 Engine: Constructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:97:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.636.578 [stars_engine.cc:49] 1039509 StarsEngine: Constructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:98:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.653.230 [runtime.cc:5840] 1039509 CreateReportRasThread: Start report ras thread success!
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:99:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.653.276 [npu_driver_res.cc:209] 1039518 GetDeviceStatus: GetDeviceStatus status=1.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:100:[INFO] ATRACE(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.657.542 [tracer_mgr_operate.c:66](tid:1039509) create object RUNTIME_ATRACE_DEV0_TS0 successfully, exitSave(false), noLock(false).
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:101:[INFO] TDT(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.657.560 [client_manager.cpp:213][SetProfilingCallback][tid:1039509] [TsdClient] set profiling callback successfully
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:102:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.141 [runtime.cc:5725] 1039520 SetWatchDogDevStatus: There is errInfo of devId=0, tsId=0
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:103:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.685.273 [runtime.cc:5881] 1039519 ReportRasRun: Report ras thread exits normally!
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:104:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.685.309 [stream_mem_pool.cc:360] 1039509 ~PoolRegistry: SOMA PoolRegistry destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:105:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.685.320 [runtime_adapt.cc:56] 1039509 ~Runtime: runtime destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:106:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.685.347 [runtime.cc:5850] 1039509 DestroyReportRasThread: Join report ras thread OK.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:107:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.685.351 [runtime.cc:1389] 1039509 WaitMonitorExit: wait monitor success, used count=0.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:108:[INFO] ATRACE(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.698.588 [tracer_mgr_operate.c:248](tid:1039509) destroy object RUNTIME_ATRACE_DEV0_TS0, exitSave(false).
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:109:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.619 [stars_engine.cc:56] 1039509 ~StarsEngine: Destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:110:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.629 [engine.cc:87] 1039509 ~Engine: Destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:111:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.665 [queue_interface.c:662][ascend][curpid:1039509,1039509][drv][queuemng][queueDeviceClose]queue close finish. (devId=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:112:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.670 [devmm_svm.c:2615][ascend][curpid:1039509,1039509][drv][devmm][drvMemDeviceCloseInner]DrvMemDeviceClose. (devid=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:113:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.676 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]HOST_MEM Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=11341824; alloc_cnt=3; free_cnt=3)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:114:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.681 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]HOST_MEM Cached_size:12582912Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:115:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.685 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=20480; allocated_peak_size=110592; alloc_cnt=12; free_cnt=7)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:116:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.689 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Mem stats (Bytes). (module_name=APP; module_id=33; current_alloced_size=126976; allocated_peak_size=126976; alloc_cnt=7; free_cnt=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:117:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.693 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_HBM dev0 Cached_size:1949696Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:118:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.696 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_DDR dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=16384; alloc_cnt=1; free_cnt=1)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:119:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.700 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_SMALL_DDR dev0 Cached_size:2097152Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:120:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.706 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=20971520; alloc_cnt=6; free_cnt=6)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:121:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.710 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_HBM dev0 Cached_size:20971520Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:122:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.712 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_DDR dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=2097152; alloc_cnt=1; free_cnt=1)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:123:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.717 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_DDR dev0 Cached_size:2097152Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:124:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.719 [svm_mem_statistics.c:150][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_P2P_HBM dev0 Mem stats (Bytes). (module_name=RUNTIME; module_id=7; current_alloced_size=0; allocated_peak_size=6291456; alloc_cnt=2; free_cnt=2)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:125:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.709.723 [svm_mem_statistics.c:156][ascend][curpid:1039509,1039509][drv][devmm][_svm_mem_stats_show]MEM_DEV_HUGE_P2P_HBM dev0 Cached_size:6291456Bytes
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:126:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.736.778 [drv_share_log.c:240][ascend][curpid:1039509,1039509][drv][devmm][share_log_read_in_single_module]Close logical_device. (logical_devid=0; phyid=0; vfid=0; hostpid=1039509; devpid=5385)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:127:[INFO] DRV(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.749.193 [event_sched.c:1515][ascend][curpid:1039509,1039509][drv][event-sche][eschedDeviceClose]The devId close success. (devId=0)
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:128:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.841.986 [xpu_task_fail_callback_data_manager.cc:64] 1039509 ~XpuTaskFailCallBackManager: XpuTaskFailCallBackManager Destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:129:[INFO] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.841.993 [task_fail_callback_data_manager.cc:98] 1039509 ~TaskFailCallBackManager: Destructor.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:130:[INFO] ATRACE(1039509,aiv_aicpu_bench):2026-06-13-17:41:59.842.055 [stacktrace_signal.c:194](tid:1039509) unregister all signal handlers, can not capture signal.
/root/ascend/log/run/plog/plog-1039509_20260613174157987.log:131:[INFO] ATRACE(1039509,aiv_aicpu_bench):2026-06-13-17:42:00.965.277 [atrace_client_core.c:127](tid:1039517) atrace receive thread exited, devId=0.
/root/ascend/log/debug/device-0/device-1039509_20260613174201266.log:1:[ERROR] CCECPU(5385,aicpu_scheduler):2026-06-13-17:41:59.451.880 [ae_so_manager.cc:670][CreateSingleSoMgr][tid:5400][AICPU_PROCESSER] So name /home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so is not invalid. Please check!
/root/ascend/log/debug/device-0/device-1039509_20260613174201266.log:2:[ERROR] CCECPU(5385,aicpu_scheduler):2026-06-13-17:41:59.451.955 [ae_kernel_lib_aicpu.cc:177][CallKernelApi][tid:5400][AICPU_PROCESSER] Get Noop api from /home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so failed.
/root/ascend/log/debug/device-0/device-1039509_20260613174201266.log:3:[ERROR] CCECPU(5385,aicpu_scheduler):2026-06-13-17:41:59.451.967 [aicpusd_event_process.cpp:906][PostProcessTsKernelTask][tid:5400] Aicpu engine process failed, result[11002], opName[null], streamId[45], taskId[0].
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:1:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.660.783 [stars_engine.cc:1548]1039520 ProcLogicCqReport:Task run failed, device_id=0, stream_id=45, task_id=0, sqe_type=1(aicpu), errType=0x1(task exception), sqSwStatus=0x2afa
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:2:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.080 [device_error_proc.cc:1644]1039520 ProcessStarsAicpuErrorInfo:The error from device(chipId:0, dieId:0), serial number is 106, an exception occurred during AICPU execution, stream_id:45, task_id:0, errcode:11002, msg:open so failed.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:3:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.137 [davinci_task.cc:209]1039520 SetStarsResultForDavinciTask:AICPU Kernel task happen error, retCode=0x2a.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:4:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.589 [davinci_kernel_task.cc:1729]1039520 PreCheckTaskErr:Kernel task happen error, retCode=0x2a, [aicpu exception].
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:5:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.615 [davinci_kernel_task.cc:1435]1039520 PrintAicpuErrorInfo:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, soName=/home/zhn/aiv_aicpu_bench/build/libaiv_aicpu_poll_kernel.so, funcName=Noop, kernelName=Noop, errorCode=0x2a.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:6:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.629 [davinci_kernel_task.cc:1445]1039520 PrintAicpuErrorInfo:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, fault op_name=.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:7:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.658 [stream.cc:1472]1039509 GetError:Stream Synchronize failed, stream_id=45, retCode=0x2a, [aicpu exception].
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:8:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.665 [stream.cc:1475]1039509 GetError:AICPU Kernel task happen error, retCode=0x2a.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:9:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.689 [stream.cc:1475]1039509 GetError:Aicpu kernel execute failed, device_id=0, stream_id=45, task_id=0, fault op_name=.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:10:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.700 [api_error.cc:1029]1039509 StreamSynchronize:Stream synchronize failed, stream_id=45, timeout=-1ms.
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:11:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.713 [api_c_stream.cc:161]1039509 rtStreamSynchronize:ErrCode=507018, desc=[aicpu exception], InnerCode=0x715002a
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:12:[ERROR] RUNTIME(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.663.719 [error_message_manage.cc:65]1039509 FuncErrorReason:rtStreamSynchronize execution failed, reason=aicpu exception
/root/ascend/log/debug/plog/plog-1039509_20260613174158663.log:13:[ERROR] ASCENDCL(1039509,aiv_aicpu_bench):2026-06-13-17:41:58.664.299 [stream.cpp:139]1039509 aclrtSynchronizeStreamImpl:synchronize stream failed, runtime result = 507018
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=aicpu_noop --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=aicpu_noop aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_noop status=0 host_total_us=90.31 host_per_task_us=90.31 aicpu_seen_interval_avg_us=0
iter=1
aicpu_noop status=0 host_total_us=49.09 host_per_task_us=49.09 aicpu_seen_interval_avg_us=0
iter=2
aicpu_noop status=0 host_total_us=55.86 host_per_task_us=55.86 aicpu_seen_interval_avg_us=0
summary: aicpu_noop_avg_us=65.09
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=173.72 host_per_task_us=86.86 aicpu_seen_interval_avg_us=74.58
iter=1
event_sync_pure status=0 host_total_us=136.58 host_per_task_us=68.29 aicpu_seen_interval_avg_us=41.30
iter=2
event_sync_pure status=0 host_total_us=101.28 host_per_task_us=50.64 aicpu_seen_interval_avg_us=24.56
summary: event_sync_pure_avg_us=137.19
last_event_sync_pure_detail status=0 host_total_us=101.28 host_per_task_us=50.64 aicpu_seen_interval_avg_us=24.56
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_with_flag_check --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_with_flag_check aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_with_flag_check status=0 host_total_us=144.77 host_per_task_us=72.39 aicpu_seen_interval_avg_us=42.52
iter=1
event_sync_with_flag_check status=0 host_total_us=140.12 host_per_task_us=70.06 aicpu_seen_interval_avg_us=39.78
iter=2
event_sync_with_flag_check status=0 host_total_us=108.13 host_per_task_us=54.06 aicpu_seen_interval_avg_us=23.02
summary: event_sync_with_flag_check_avg_us=131.01
last_event_sync_with_flag_check_detail status=0 host_total_us=108.13 host_per_task_us=54.06 aicpu_seen_interval_avg_us=23.02
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=4096 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=all --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=4096 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=all aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
shared status=0 host_total_us=66.37 host_per_task_us=33.19 aicpu_seen_interval_avg_us=0.12
event_sync_pure status=0 host_total_us=106.42 host_per_task_us=53.21 aicpu_seen_interval_avg_us=32.26
event_sync_with_flag_check status=0 host_total_us=100.54 host_per_task_us=50.27 aicpu_seen_interval_avg_us=37.52
iter=1
shared status=0 host_total_us=43.97 host_per_task_us=21.98 aicpu_seen_interval_avg_us=0.24
event_sync_pure status=0 host_total_us=101.29 host_per_task_us=50.65 aicpu_seen_interval_avg_us=32.94
event_sync_with_flag_check status=0 host_total_us=90.35 host_per_task_us=45.17 aicpu_seen_interval_avg_us=24.34
iter=2
shared status=0 host_total_us=43.03 host_per_task_us=21.52 aicpu_seen_interval_avg_us=0.26
event_sync_pure status=0 host_total_us=104.84 host_per_task_us=52.42 aicpu_seen_interval_avg_us=42.34
event_sync_with_flag_check status=0 host_total_us=109.40 host_per_task_us=54.70 aicpu_seen_interval_avg_us=42.10
summary: shared_avg_us=51.12 event_sync_pure_avg_us=104.18 event_sync_with_flag_check_avg_us=100.10 speedup_event_pure_over_shared=2.04 speedup_event_flag_check_over_shared=1.96
last_shared_detail status=0 host_total_us=43.03 host_per_task_us=21.52 aicpu_seen_interval_avg_us=0.26
last_event_sync_pure_detail status=0 host_total_us=104.84 host_per_task_us=52.42 aicpu_seen_interval_avg_us=42.34
last_event_sync_with_flag_check_detail status=0 host_total_us=109.40 host_per_task_us=54.70 aicpu_seen_interval_avg_us=42.10
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=8192 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=8192 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=179.18 host_per_task_us=89.59 aicpu_seen_interval_avg_us=75.18
iter=1
event_sync_pure status=0 host_total_us=130.38 host_per_task_us=65.19 aicpu_seen_interval_avg_us=42.24
iter=2
event_sync_pure status=0 host_total_us=97.47 host_per_task_us=48.73 aicpu_seen_interval_avg_us=24.92
summary: event_sync_pure_avg_us=135.68
last_event_sync_pure_detail status=0 host_total_us=97.47 host_per_task_us=48.73 aicpu_seen_interval_avg_us=24.92
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=12288 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=12288 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=125.43 host_per_task_us=62.72 aicpu_seen_interval_avg_us=42.00
iter=1
event_sync_pure status=0 host_total_us=108.53 host_per_task_us=54.27 aicpu_seen_interval_avg_us=33.62
iter=2
event_sync_pure status=0 host_total_us=83.89 host_per_task_us=41.95 aicpu_seen_interval_avg_us=21.62
summary: event_sync_pure_avg_us=105.95
last_event_sync_pure_detail status=0 host_total_us=83.89 host_per_task_us=41.95 aicpu_seen_interval_avg_us=21.62
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=16384 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=16384 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=121.20 host_per_task_us=60.60 aicpu_seen_interval_avg_us=35.06
iter=1
event_sync_pure status=0 host_total_us=124.26 host_per_task_us=62.13 aicpu_seen_interval_avg_us=44.70
iter=2
event_sync_pure status=0 host_total_us=87.81 host_per_task_us=43.91 aicpu_seen_interval_avg_us=23.46
summary: event_sync_pure_avg_us=111.09
last_event_sync_pure_detail status=0 host_total_us=87.81 host_per_task_us=43.91 aicpu_seen_interval_avg_us=23.46


(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=2 --elements=65536 --tile=1024 --repeat=1 --warmup=1 --iters=3 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=2 elements=65536 tile=1024 repeat=1 iters=3 warmup=1 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=167.36 host_per_task_us=83.68 aicpu_seen_interval_avg_us=67.26
iter=1
event_sync_pure status=0 host_total_us=134.02 host_per_task_us=67.01 aicpu_seen_interval_avg_us=40.72
iter=2
event_sync_pure status=0 host_total_us=109.95 host_per_task_us=54.98 aicpu_seen_interval_avg_us=24.60
summary: event_sync_pure_avg_us=137.11
last_event_sync_pure_detail status=0 host_total_us=109.95 host_per_task_us=54.98 aicpu_seen_interval_avg_us=24.60
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --tile=1024 --repeat=1 --warmup=5 --iters=20 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=1 iters=20 warmup=5 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=196.87 host_per_task_us=24.61 aicpu_seen_interval_avg_us=20.88
iter=1
event_sync_pure status=0 host_total_us=216.73 host_per_task_us=27.09 aicpu_seen_interval_avg_us=21.01
iter=2
event_sync_pure status=0 host_total_us=211.68 host_per_task_us=26.46 aicpu_seen_interval_avg_us=22.42
iter=3
event_sync_pure status=0 host_total_us=193.65 host_per_task_us=24.21 aicpu_seen_interval_avg_us=20.33
iter=4
event_sync_pure status=0 host_total_us=200.81 host_per_task_us=25.10 aicpu_seen_interval_avg_us=21.36
iter=5
event_sync_pure status=0 host_total_us=205.81 host_per_task_us=25.73 aicpu_seen_interval_avg_us=22.09
iter=6
event_sync_pure status=0 host_total_us=193.67 host_per_task_us=24.21 aicpu_seen_interval_avg_us=20.26
iter=7
event_sync_pure status=0 host_total_us=200.07 host_per_task_us=25.01 aicpu_seen_interval_avg_us=21.03
iter=8
event_sync_pure status=0 host_total_us=203.72 host_per_task_us=25.46 aicpu_seen_interval_avg_us=21.74
iter=9
event_sync_pure status=0 host_total_us=202.63 host_per_task_us=25.33 aicpu_seen_interval_avg_us=21.04
iter=10
event_sync_pure status=0 host_total_us=201.06 host_per_task_us=25.13 aicpu_seen_interval_avg_us=21.37
iter=11
event_sync_pure status=0 host_total_us=200.04 host_per_task_us=25.00 aicpu_seen_interval_avg_us=21.38
iter=12
event_sync_pure status=0 host_total_us=203.93 host_per_task_us=25.49 aicpu_seen_interval_avg_us=21.84
iter=13
event_sync_pure status=0 host_total_us=201.54 host_per_task_us=25.19 aicpu_seen_interval_avg_us=21.65
iter=14
event_sync_pure status=0 host_total_us=205.63 host_per_task_us=25.70 aicpu_seen_interval_avg_us=22.19
iter=15
event_sync_pure status=0 host_total_us=195.13 host_per_task_us=24.39 aicpu_seen_interval_avg_us=20.50
iter=16
event_sync_pure status=0 host_total_us=198.16 host_per_task_us=24.77 aicpu_seen_interval_avg_us=21.32
iter=17
event_sync_pure status=0 host_total_us=204.65 host_per_task_us=25.58 aicpu_seen_interval_avg_us=21.89
iter=18
event_sync_pure status=0 host_total_us=196.45 host_per_task_us=24.56 aicpu_seen_interval_avg_us=20.45
iter=19
event_sync_pure status=0 host_total_us=195.81 host_per_task_us=24.48 aicpu_seen_interval_avg_us=20.98
summary: event_sync_pure_avg_us=201.40
last_event_sync_pure_detail status=0 host_total_us=195.81 host_per_task_us=24.48 aicpu_seen_interval_avg_us=20.98
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=327680 --tile=1024 --repeat=1 --warmup=5 --iters=20 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=327680 tile=1024 repeat=1 iters=20 warmup=5 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=871.43 host_per_task_us=108.93 aicpu_seen_interval_avg_us=94.75
iter=1
event_sync_pure status=0 host_total_us=799.23 host_per_task_us=99.90 aicpu_seen_interval_avg_us=95.42
iter=2
event_sync_pure status=0 host_total_us=791.39 host_per_task_us=98.92 aicpu_seen_interval_avg_us=94.15
iter=3
event_sync_pure status=0 host_total_us=790.81 host_per_task_us=98.85 aicpu_seen_interval_avg_us=94.09
iter=4
event_sync_pure status=0 host_total_us=794.63 host_per_task_us=99.33 aicpu_seen_interval_avg_us=95.12
iter=5
event_sync_pure status=0 host_total_us=790.81 host_per_task_us=98.85 aicpu_seen_interval_avg_us=93.85
iter=6
event_sync_pure status=0 host_total_us=787.63 host_per_task_us=98.45 aicpu_seen_interval_avg_us=94.39
iter=7
event_sync_pure status=0 host_total_us=794.22 host_per_task_us=99.28 aicpu_seen_interval_avg_us=94.91
iter=8
event_sync_pure status=0 host_total_us=796.07 host_per_task_us=99.51 aicpu_seen_interval_avg_us=95.47
iter=9
event_sync_pure status=0 host_total_us=795.91 host_per_task_us=99.49 aicpu_seen_interval_avg_us=94.71
iter=10
event_sync_pure status=0 host_total_us=784.66 host_per_task_us=98.08 aicpu_seen_interval_avg_us=93.92
iter=11
event_sync_pure status=0 host_total_us=786.06 host_per_task_us=98.26 aicpu_seen_interval_avg_us=93.98
iter=12
event_sync_pure status=0 host_total_us=786.98 host_per_task_us=98.37 aicpu_seen_interval_avg_us=94.34
iter=13
event_sync_pure status=0 host_total_us=783.99 host_per_task_us=98.00 aicpu_seen_interval_avg_us=93.84
iter=14
event_sync_pure status=0 host_total_us=782.93 host_per_task_us=97.87 aicpu_seen_interval_avg_us=93.49
iter=15
event_sync_pure status=0 host_total_us=786.13 host_per_task_us=98.27 aicpu_seen_interval_avg_us=93.84
iter=16
event_sync_pure status=0 host_total_us=800.21 host_per_task_us=100.03 aicpu_seen_interval_avg_us=94.39
iter=17
event_sync_pure status=0 host_total_us=806.64 host_per_task_us=100.83 aicpu_seen_interval_avg_us=93.77
iter=18
event_sync_pure status=0 host_total_us=804.36 host_per_task_us=100.55 aicpu_seen_interval_avg_us=93.88
iter=19
event_sync_pure status=0 host_total_us=793.34 host_per_task_us=99.17 aicpu_seen_interval_avg_us=93.90
summary: event_sync_pure_avg_us=796.37
last_event_sync_pure_detail status=0 host_total_us=793.34 host_per_task_us=99.17 aicpu_seen_interval_avg_us=93.90
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=393216 --tile=1024 --repeat=1 --warmup=5 --iters=20 --mode=event_sync_pure --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=393216 tile=1024 repeat=1 iters=20 warmup=5 timeout_ms=5000 mode=event_sync_pure aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
event_sync_pure status=0 host_total_us=1025.70 host_per_task_us=128.21 aicpu_seen_interval_avg_us=113.97
iter=1
event_sync_pure status=0 host_total_us=1025.29 host_per_task_us=128.16 aicpu_seen_interval_avg_us=113.24
iter=2
event_sync_pure status=0 host_total_us=1019.56 host_per_task_us=127.44 aicpu_seen_interval_avg_us=113.48
iter=3
event_sync_pure status=0 host_total_us=1013.88 host_per_task_us=126.73 aicpu_seen_interval_avg_us=113.13
iter=4
event_sync_pure status=0 host_total_us=1013.63 host_per_task_us=126.70 aicpu_seen_interval_avg_us=114.27
iter=5
event_sync_pure status=0 host_total_us=993.22 host_per_task_us=124.15 aicpu_seen_interval_avg_us=114.21
iter=6
event_sync_pure status=0 host_total_us=1007.56 host_per_task_us=125.94 aicpu_seen_interval_avg_us=113.66
iter=7
event_sync_pure status=0 host_total_us=1024.70 host_per_task_us=128.09 aicpu_seen_interval_avg_us=113.83
iter=8
event_sync_pure status=0 host_total_us=1004.35 host_per_task_us=125.54 aicpu_seen_interval_avg_us=114.24
iter=9
event_sync_pure status=0 host_total_us=1009.57 host_per_task_us=126.20 aicpu_seen_interval_avg_us=114.40
iter=10
event_sync_pure status=0 host_total_us=1008.39 host_per_task_us=126.05 aicpu_seen_interval_avg_us=111.81
iter=11
event_sync_pure status=0 host_total_us=1010.67 host_per_task_us=126.33 aicpu_seen_interval_avg_us=112.05
iter=12
event_sync_pure status=0 host_total_us=1010.89 host_per_task_us=126.36 aicpu_seen_interval_avg_us=113.02
iter=13
event_sync_pure status=0 host_total_us=1001.37 host_per_task_us=125.17 aicpu_seen_interval_avg_us=111.63
iter=14
event_sync_pure status=0 host_total_us=1002.95 host_per_task_us=125.37 aicpu_seen_interval_avg_us=112.77
iter=15
event_sync_pure status=0 host_total_us=1005.60 host_per_task_us=125.70 aicpu_seen_interval_avg_us=112.37
iter=16
event_sync_pure status=0 host_total_us=1001.61 host_per_task_us=125.20 aicpu_seen_interval_avg_us=111.75
iter=17
event_sync_pure status=0 host_total_us=1007.75 host_per_task_us=125.97 aicpu_seen_interval_avg_us=112.44
iter=18
event_sync_pure status=0 host_total_us=1015.59 host_per_task_us=126.95 aicpu_seen_interval_avg_us=112.30
iter=19
event_sync_pure status=0 host_total_us=995.58 host_per_task_us=124.45 aicpu_seen_interval_avg_us=112.29
summary: event_sync_pure_avg_us=1009.89
last_event_sync_pure_detail status=0 host_total_us=995.58 host_per_task_us=124.45 aicpu_seen_interval_avg_us=112.29
(base) [root@localhost build]#

(base) [root@localhost build]# msprof --output=./prof_shared \
>     --application="./aiv_aicpu_bench --device=0 --tasks=4 --elements=65536 --tile=1024 --repeat=1 --warmup=0 --iters=1 --mode=shared
>     --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json"
[INFO] Start profiling....
usage: /home/zhn/aiv_aicpu_bench/build/aiv_aicpu_bench [--device=0] [--tasks=4] [--elements=262144]
       [--tile=1024] [--repeat=1] [--iters=5] [--warmup=1]
       [--timeout-ms=5000] [--mode=all|shared|event_sync_pure|event_sync_with_flag_check|aicpu_noop]
       [--aicpu-json=build/libaiv_aicpu_poll_kernel.json]
[WARNING] An exception has occurred in process App, return code: Operation not permitted.
[ERROR] Failed to find profiling data, please check that the application executes AI-related business, or ensure that aclInit/GEInitialize is invoked in the application
[ERROR] Running profiling failed. Please check log for more info.
(base) [root@localhost build]#


(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=1024 --tile=1024 --repeat=1 --warmup=5 --iters=20 --mode=all --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=1024 tile=1024 repeat=1 iters=20 warmup=5 timeout_ms=5000 mode=all aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
shared status=0 host_total_us=42.03 host_per_task_us=5.25 aicpu_seen_interval_avg_us=0.13
event_sync_pure status=0 host_total_us=280.15 host_per_task_us=35.02 aicpu_seen_interval_avg_us=30.37
event_sync_with_flag_check status=0 host_total_us=281.98 host_per_task_us=35.25 aicpu_seen_interval_avg_us=30.45
iter=1
shared status=0 host_total_us=53.39 host_per_task_us=6.67 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=272.77 host_per_task_us=34.10 aicpu_seen_interval_avg_us=15.78
event_sync_with_flag_check status=0 host_total_us=284.06 host_per_task_us=35.51 aicpu_seen_interval_avg_us=31.45
iter=2
shared status=0 host_total_us=33.66 host_per_task_us=4.21 aicpu_seen_interval_avg_us=0.10
event_sync_pure status=0 host_total_us=283.47 host_per_task_us=35.43 aicpu_seen_interval_avg_us=28.41
event_sync_with_flag_check status=0 host_total_us=269.27 host_per_task_us=33.66 aicpu_seen_interval_avg_us=29.49
iter=3
shared status=0 host_total_us=37.04 host_per_task_us=4.63 aicpu_seen_interval_avg_us=0.08
event_sync_pure status=0 host_total_us=280.52 host_per_task_us=35.06 aicpu_seen_interval_avg_us=29.73
event_sync_with_flag_check status=0 host_total_us=284.82 host_per_task_us=35.60 aicpu_seen_interval_avg_us=32.59
iter=4
shared status=0 host_total_us=38.79 host_per_task_us=4.85 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=266.06 host_per_task_us=33.26 aicpu_seen_interval_avg_us=27.68
event_sync_with_flag_check status=0 host_total_us=280.24 host_per_task_us=35.03 aicpu_seen_interval_avg_us=30.06
iter=5
shared status=0 host_total_us=43.62 host_per_task_us=5.45 aicpu_seen_interval_avg_us=0.60
event_sync_pure status=0 host_total_us=290.73 host_per_task_us=36.34 aicpu_seen_interval_avg_us=30.96
event_sync_with_flag_check status=0 host_total_us=299.19 host_per_task_us=37.40 aicpu_seen_interval_avg_us=31.68
iter=6
shared status=0 host_total_us=35.11 host_per_task_us=4.39 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=266.79 host_per_task_us=33.35 aicpu_seen_interval_avg_us=29.00
event_sync_with_flag_check status=0 host_total_us=278.25 host_per_task_us=34.78 aicpu_seen_interval_avg_us=27.86
iter=7
shared status=0 host_total_us=46.00 host_per_task_us=5.75 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=272.94 host_per_task_us=34.12 aicpu_seen_interval_avg_us=29.55
event_sync_with_flag_check status=0 host_total_us=255.18 host_per_task_us=31.90 aicpu_seen_interval_avg_us=27.67
iter=8
shared status=0 host_total_us=34.25 host_per_task_us=4.28 aicpu_seen_interval_avg_us=0.07
event_sync_pure status=0 host_total_us=292.83 host_per_task_us=36.60 aicpu_seen_interval_avg_us=29.33
event_sync_with_flag_check status=0 host_total_us=281.55 host_per_task_us=35.19 aicpu_seen_interval_avg_us=31.62
iter=9
shared status=0 host_total_us=39.38 host_per_task_us=4.92 aicpu_seen_interval_avg_us=0.07
event_sync_pure status=0 host_total_us=289.12 host_per_task_us=36.14 aicpu_seen_interval_avg_us=31.45
event_sync_with_flag_check status=0 host_total_us=285.22 host_per_task_us=35.65 aicpu_seen_interval_avg_us=31.13
iter=10
shared status=0 host_total_us=37.43 host_per_task_us=4.68 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=281.60 host_per_task_us=35.20 aicpu_seen_interval_avg_us=29.89
event_sync_with_flag_check status=0 host_total_us=268.10 host_per_task_us=33.51 aicpu_seen_interval_avg_us=29.36
iter=11
shared status=0 host_total_us=36.11 host_per_task_us=4.51 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=285.24 host_per_task_us=35.66 aicpu_seen_interval_avg_us=30.49
event_sync_with_flag_check status=0 host_total_us=269.24 host_per_task_us=33.66 aicpu_seen_interval_avg_us=30.89
iter=12
shared status=0 host_total_us=38.25 host_per_task_us=4.78 aicpu_seen_interval_avg_us=0.10
event_sync_pure status=0 host_total_us=300.54 host_per_task_us=37.57 aicpu_seen_interval_avg_us=32.06
event_sync_with_flag_check status=0 host_total_us=295.54 host_per_task_us=36.94 aicpu_seen_interval_avg_us=30.73
iter=13
shared status=0 host_total_us=54.35 host_per_task_us=6.79 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=279.21 host_per_task_us=34.90 aicpu_seen_interval_avg_us=27.63
event_sync_with_flag_check status=0 host_total_us=277.29 host_per_task_us=34.66 aicpu_seen_interval_avg_us=30.04
iter=14
shared status=0 host_total_us=47.06 host_per_task_us=5.88 aicpu_seen_interval_avg_us=0.09
event_sync_pure status=0 host_total_us=288.38 host_per_task_us=36.05 aicpu_seen_interval_avg_us=30.45
event_sync_with_flag_check status=0 host_total_us=273.28 host_per_task_us=34.16 aicpu_seen_interval_avg_us=30.74
iter=15
shared status=0 host_total_us=40.61 host_per_task_us=5.08 aicpu_seen_interval_avg_us=0.08
event_sync_pure status=0 host_total_us=289.85 host_per_task_us=36.23 aicpu_seen_interval_avg_us=30.77
event_sync_with_flag_check status=0 host_total_us=289.17 host_per_task_us=36.15 aicpu_seen_interval_avg_us=30.58
iter=16
shared status=0 host_total_us=39.73 host_per_task_us=4.97 aicpu_seen_interval_avg_us=0.10
event_sync_pure status=0 host_total_us=277.13 host_per_task_us=34.64 aicpu_seen_interval_avg_us=29.31
event_sync_with_flag_check status=0 host_total_us=268.38 host_per_task_us=33.55 aicpu_seen_interval_avg_us=27.93
iter=17
shared status=0 host_total_us=38.57 host_per_task_us=4.82 aicpu_seen_interval_avg_us=0.10
event_sync_pure status=0 host_total_us=274.64 host_per_task_us=34.33 aicpu_seen_interval_avg_us=28.81
event_sync_with_flag_check status=0 host_total_us=279.83 host_per_task_us=34.98 aicpu_seen_interval_avg_us=29.88
iter=18
shared status=0 host_total_us=41.78 host_per_task_us=5.22 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=309.08 host_per_task_us=38.63 aicpu_seen_interval_avg_us=31.80
event_sync_with_flag_check status=0 host_total_us=298.69 host_per_task_us=37.34 aicpu_seen_interval_avg_us=31.52
iter=19
shared status=0 host_total_us=39.98 host_per_task_us=5.00 aicpu_seen_interval_avg_us=0.11
event_sync_pure status=0 host_total_us=278.86 host_per_task_us=34.86 aicpu_seen_interval_avg_us=28.68
event_sync_with_flag_check status=0 host_total_us=335.91 host_per_task_us=41.99 aicpu_seen_interval_avg_us=39.26
summary: shared_avg_us=40.86 event_sync_pure_avg_us=283.00 event_sync_with_flag_check_avg_us=282.76 speedup_event_pure_over_shared=6.93 speedup_event_flag_check_over_shared=6.92
last_shared_detail status=0 host_total_us=39.98 host_per_task_us=5.00 aicpu_seen_interval_avg_us=0.11
last_event_sync_pure_detail status=0 host_total_us=278.86 host_per_task_us=34.86 aicpu_seen_interval_avg_us=28.68
last_event_sync_with_flag_check_detail status=0 host_total_us=335.91 host_per_task_us=41.99 aicpu_seen_interval_avg_us=39.26
(base) [root@localhost build]#

(base) [root@localhost aiv_aicpu_bench]# rm -rf build
(base) [root@localhost aiv_aicpu_bench]# mkdir build
(base) [root@localhost aiv_aicpu_bench]# cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
CMake Error: The source directory "/home/zhn" does not appear to contain CMakeLists.txt.
Specify --help for usage, or press the help button on the CMake GUI.
(base) [root@localhost aiv_aicpu_bench]# source /usr/local/Ascend/cann-9.0.0/set_env.sh
(base) [root@localhost aiv_aicpu_bench]# cmake .. -DNPU_ARCH=dav-c220 -DASC_ARCH_FLAG=--cce-aicore-arch
CMake Error: The source directory "/home/zhn" does not appear to contain CMakeLists.txt.
Specify --help for usage, or press the help button on the CMake GUI.
(base) [root@localhost aiv_aicpu_bench]#

config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=1 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=47876.74 aicpu_elapsed_us=47789.00 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=48339.21 aicpu_elapsed_us=48227.44 ns_per_read=5.75 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=47859.29 aicpu_elapsed_us=47771.52 ns_per_read=5.69 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=47982.67 aicpu_elapsed_us=47897.24 ns_per_read=5.71 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=47866.14 aicpu_elapsed_us=47794.70 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=47984.81
last_aicpu_read_bench_detail status=0 host_total_us=47866.14 aicpu_elapsed_us=47794.70 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
(base) [root@localhost build]#




config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=1 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=47876.74 aicpu_elapsed_us=47789.00 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=48339.21 aicpu_elapsed_us=48227.44 ns_per_read=5.75 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=47859.29 aicpu_elapsed_us=47771.52 ns_per_read=5.69 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=47982.67 aicpu_elapsed_us=47897.24 ns_per_read=5.71 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=47866.14 aicpu_elapsed_us=47794.70 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=47984.81
last_aicpu_read_bench_detail status=0 host_total_us=47866.14 aicpu_elapsed_us=47794.70 ns_per_read=5.70 read_count=8388608 data_words=524288 read_stride_words=1 checksum=8936830510563328
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=16 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json           config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=16 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=62374.78 aicpu_elapsed_us=62290.10 ns_per_read=7.43 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=67392.66 aicpu_elapsed_us=67268.66 ns_per_read=8.02 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=65209.68 aicpu_elapsed_us=65101.06 ns_per_read=7.76 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=64644.36 aicpu_elapsed_us=64528.70 ns_per_read=7.69 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=64241.52 aicpu_elapsed_us=64132.80 ns_per_read=7.65 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=64772.60
last_aicpu_read_bench_detail status=0 host_total_us=64241.52 aicpu_elapsed_us=64132.80 ns_per_read=7.65 read_count=8388608 data_words=524288 read_stride_words=16 checksum=8936830510563328
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=1024 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=1024 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=71804.93 aicpu_elapsed_us=71709.64 ns_per_read=8.55 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=89236.46 aicpu_elapsed_us=89112.62 ns_per_read=10.62 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=82918.59 aicpu_elapsed_us=82829.30 ns_per_read=9.87 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=81787.21 aicpu_elapsed_us=81686.14 ns_per_read=9.74 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=79383.01 aicpu_elapsed_us=79300.42 ns_per_read=9.45 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=81026.04
last_aicpu_read_bench_detail status=0 host_total_us=79383.01 aicpu_elapsed_us=79300.42 ns_per_read=9.45 read_count=8388608 data_words=524288 read_stride_words=1024 checksum=8936830510563328
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=16384 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=16384 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=160960.39 aicpu_elapsed_us=160879.54 ns_per_read=19.18 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=238001.94 aicpu_elapsed_us=237821.00 ns_per_read=28.35 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=194253.95 aicpu_elapsed_us=194148.48 ns_per_read=23.14 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=227461.06 aicpu_elapsed_us=227354.30 ns_per_read=27.10 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=190727.95 aicpu_elapsed_us=190648.86 ns_per_read=22.73 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=202281.06
last_aicpu_read_bench_detail status=0 host_total_us=190727.95 aicpu_elapsed_us=190648.86 ns_per_read=22.73 read_count=8388608 data_words=524288 read_stride_words=16384 checksum=8936830510563328
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=16385 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=16385 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=147113.83 aicpu_elapsed_us=147028.08 ns_per_read=17.53 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=183698.88 aicpu_elapsed_us=183504.88 ns_per_read=21.88 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=173551.47 aicpu_elapsed_us=173398.18 ns_per_read=20.67 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=172293.39 aicpu_elapsed_us=172160.24 ns_per_read=20.52 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=170046.77 aicpu_elapsed_us=169929.62 ns_per_read=20.26 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=169340.87
last_aicpu_read_bench_detail status=0 host_total_us=170046.77 aicpu_elapsed_us=169929.62 ns_per_read=20.26 read_count=8388608 data_words=524288 read_stride_words=16385 checksum=8936830510563328
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=1025 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=1025 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=49436.95 aicpu_elapsed_us=49361.52 ns_per_read=5.88 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=49416.76 aicpu_elapsed_us=49313.06 ns_per_read=5.88 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=49442.36 aicpu_elapsed_us=49351.46 ns_per_read=5.88 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=49381.32 aicpu_elapsed_us=49306.24 ns_per_read=5.88 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=49466.31 aicpu_elapsed_us=49391.88 ns_per_read=5.89 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=49428.74
last_aicpu_read_bench_detail status=0 host_total_us=49466.31 aicpu_elapsed_us=49391.88 ns_per_read=5.89 read_count=8388608 data_words=524288 read_stride_words=1025 checksum=8936830510563328
(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=65536 --repeat=16 --read-stride=17 --warmup=1 --iters=5 --mode=aicpu_read_bench --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=65536 tile=1024 repeat=16 iters=5 warmup=1 timeout_ms=5000 read_stride=17 mode=aicpu_read_bench aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_bench status=0 host_total_us=64202.47 aicpu_elapsed_us=64124.18 ns_per_read=7.64 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
iter=1
aicpu_read_bench status=0 host_total_us=70692.22 aicpu_elapsed_us=70586.56 ns_per_read=8.41 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
iter=2
aicpu_read_bench status=0 host_total_us=67842.42 aicpu_elapsed_us=67753.98 ns_per_read=8.08 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
iter=3
aicpu_read_bench status=0 host_total_us=67007.68 aicpu_elapsed_us=66928.66 ns_per_read=7.98 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
iter=4
aicpu_read_bench status=0 host_total_us=66633.36 aicpu_elapsed_us=66564.96 ns_per_read=7.94 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
summary: aicpu_read_bench_host_avg_us=67275.63
last_aicpu_read_bench_detail status=0 host_total_us=66633.36 aicpu_elapsed_us=66564.96 ns_per_read=7.94 read_count=8388608 data_words=524288 read_stride_words=17 checksum=8936830510563328
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench \
>     --device=0 \
>     --tasks=64 \
>     --elements=1048576 \
>     --repeat=1 \
>     --mode=aicpu_read_chase \
>     --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=64 elements=1048576 tile=1024 repeat=1 iters=5 warmup=1 timeout_ms=5000 read_stride=1 mode=aicpu_read_chase aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
aicpu_read_chase status=0 host_total_us=13273270.97 aicpu_elapsed_us=13273098.44 ns_per_chase_read=197.78 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
iter=1
aicpu_read_chase status=0 host_total_us=13246451.58 aicpu_elapsed_us=13246303.80 ns_per_chase_read=197.39 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
iter=2
aicpu_read_chase status=0 host_total_us=13237362.88 aicpu_elapsed_us=13237216.86 ns_per_chase_read=197.25 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
iter=3
aicpu_read_chase status=0 host_total_us=13222302.06 aicpu_elapsed_us=13222124.30 ns_per_chase_read=197.03 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
iter=4
aicpu_read_chase status=0 host_total_us=13268176.54 aicpu_elapsed_us=13268012.84 ns_per_chase_read=197.71 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
summary: aicpu_read_chase_host_avg_us=13249512.81
last_aicpu_read_chase_detail status=0 host_total_us=13268176.54 aicpu_elapsed_us=13268012.84 ns_per_chase_read=197.71 chase_steps=67108864 data_words=67108864 checksum=18302628885633695744
(base) [root@localhost build]#

(base) [root@localhost build]# ./aiv_aicpu_bench --device=0 --tasks=8 --elements=327680 --tile=1024 --repeat=1 --warmup=5 --iters=20 --mode=all --aicpu-json=$ASCEND_HOME_PATH/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
config: device=0 tasks=8 elements=327680 tile=1024 repeat=1 iters=20 warmup=5 timeout_ms=5000 mode=all aicpu_json=/usr/local/Ascend/cann-9.0.0/opp/vendors/cust/aicpu/config/libaiv_aicpu_poll_kernel.json
iter=0
shared status=0 host_total_us=800.22 host_per_task_us=100.03 aicpu_seen_interval_avg_us=93.85
event_sync_pure status=0 host_total_us=853.30 host_per_task_us=106.66 aicpu_seen_interval_avg_us=96.05
event_sync_with_flag_check status=0 host_total_us=862.56 host_per_task_us=107.82 aicpu_seen_interval_avg_us=95.83
iter=1
shared status=0 host_total_us=832.51 host_per_task_us=104.06 aicpu_seen_interval_avg_us=93.86
event_sync_pure status=0 host_total_us=853.56 host_per_task_us=106.69 aicpu_seen_interval_avg_us=93.85
event_sync_with_flag_check status=0 host_total_us=803.57 host_per_task_us=100.45 aicpu_seen_interval_avg_us=95.95
iter=2
shared status=0 host_total_us=791.90 host_per_task_us=98.99 aicpu_seen_interval_avg_us=95.43
event_sync_pure status=0 host_total_us=802.93 host_per_task_us=100.37 aicpu_seen_interval_avg_us=95.34
event_sync_with_flag_check status=0 host_total_us=861.10 host_per_task_us=107.64 aicpu_seen_interval_avg_us=96.15
iter=3
shared status=0 host_total_us=824.44 host_per_task_us=103.06 aicpu_seen_interval_avg_us=95.52
event_sync_pure status=0 host_total_us=860.37 host_per_task_us=107.55 aicpu_seen_interval_avg_us=94.27
event_sync_with_flag_check status=0 host_total_us=860.45 host_per_task_us=107.56 aicpu_seen_interval_avg_us=95.47
iter=4
shared status=0 host_total_us=829.87 host_per_task_us=103.73 aicpu_seen_interval_avg_us=95.49
event_sync_pure status=0 host_total_us=860.45 host_per_task_us=107.56 aicpu_seen_interval_avg_us=95.75
event_sync_with_flag_check status=0 host_total_us=866.69 host_per_task_us=108.34 aicpu_seen_interval_avg_us=96.40
iter=5
shared status=0 host_total_us=827.96 host_per_task_us=103.50 aicpu_seen_interval_avg_us=95.45
event_sync_pure status=0 host_total_us=864.51 host_per_task_us=108.06 aicpu_seen_interval_avg_us=95.39
event_sync_with_flag_check status=0 host_total_us=811.85 host_per_task_us=101.48 aicpu_seen_interval_avg_us=95.38
iter=6
shared status=0 host_total_us=790.65 host_per_task_us=98.83 aicpu_seen_interval_avg_us=95.44
event_sync_pure status=0 host_total_us=864.33 host_per_task_us=108.04 aicpu_seen_interval_avg_us=94.80
event_sync_with_flag_check status=0 host_total_us=860.24 host_per_task_us=107.53 aicpu_seen_interval_avg_us=96.56
iter=7
shared status=0 host_total_us=834.89 host_per_task_us=104.36 aicpu_seen_interval_avg_us=95.44
event_sync_pure status=0 host_total_us=864.74 host_per_task_us=108.09 aicpu_seen_interval_avg_us=95.30
event_sync_with_flag_check status=0 host_total_us=807.42 host_per_task_us=100.93 aicpu_seen_interval_avg_us=95.63
iter=8
shared status=0 host_total_us=788.28 host_per_task_us=98.53 aicpu_seen_interval_avg_us=95.44
event_sync_pure status=0 host_total_us=856.03 host_per_task_us=107.00 aicpu_seen_interval_avg_us=95.15
event_sync_with_flag_check status=0 host_total_us=859.29 host_per_task_us=107.41 aicpu_seen_interval_avg_us=96.05
iter=9
shared status=0 host_total_us=828.86 host_per_task_us=103.61 aicpu_seen_interval_avg_us=95.46
event_sync_pure status=0 host_total_us=857.47 host_per_task_us=107.18 aicpu_seen_interval_avg_us=94.22
event_sync_with_flag_check status=0 host_total_us=795.19 host_per_task_us=99.40 aicpu_seen_interval_avg_us=95.03
iter=10
shared status=0 host_total_us=790.04 host_per_task_us=98.75 aicpu_seen_interval_avg_us=95.51
event_sync_pure status=0 host_total_us=880.87 host_per_task_us=110.11 aicpu_seen_interval_avg_us=94.16
event_sync_with_flag_check status=0 host_total_us=805.11 host_per_task_us=100.64 aicpu_seen_interval_avg_us=95.37
iter=11
shared status=0 host_total_us=790.99 host_per_task_us=98.87 aicpu_seen_interval_avg_us=95.44
event_sync_pure status=0 host_total_us=858.38 host_per_task_us=107.30 aicpu_seen_interval_avg_us=94.80
event_sync_with_flag_check status=0 host_total_us=858.80 host_per_task_us=107.35 aicpu_seen_interval_avg_us=95.01
iter=12
shared status=0 host_total_us=828.68 host_per_task_us=103.58 aicpu_seen_interval_avg_us=95.45
event_sync_pure status=0 host_total_us=787.36 host_per_task_us=98.42 aicpu_seen_interval_avg_us=94.06
event_sync_with_flag_check status=0 host_total_us=852.88 host_per_task_us=106.61 aicpu_seen_interval_avg_us=95.48
iter=13
shared status=0 host_total_us=828.09 host_per_task_us=103.51 aicpu_seen_interval_avg_us=93.86
event_sync_pure status=0 host_total_us=852.41 host_per_task_us=106.55 aicpu_seen_interval_avg_us=94.41
event_sync_with_flag_check status=0 host_total_us=840.00 host_per_task_us=105.00 aicpu_seen_interval_avg_us=96.20
iter=14
shared status=0 host_total_us=839.27 host_per_task_us=104.91 aicpu_seen_interval_avg_us=93.82
event_sync_pure status=0 host_total_us=863.68 host_per_task_us=107.96 aicpu_seen_interval_avg_us=95.18
event_sync_with_flag_check status=0 host_total_us=798.14 host_per_task_us=99.77 aicpu_seen_interval_avg_us=95.51
iter=15
shared status=0 host_total_us=773.30 host_per_task_us=96.66 aicpu_seen_interval_avg_us=93.89
event_sync_pure status=0 host_total_us=861.34 host_per_task_us=107.67 aicpu_seen_interval_avg_us=94.13
event_sync_with_flag_check status=0 host_total_us=861.19 host_per_task_us=107.65 aicpu_seen_interval_avg_us=95.35
iter=16
shared status=0 host_total_us=830.38 host_per_task_us=103.80 aicpu_seen_interval_avg_us=95.49
event_sync_pure status=0 host_total_us=853.33 host_per_task_us=106.67 aicpu_seen_interval_avg_us=94.66
event_sync_with_flag_check status=0 host_total_us=860.48 host_per_task_us=107.56 aicpu_seen_interval_avg_us=95.45
iter=17
shared status=0 host_total_us=837.27 host_per_task_us=104.66 aicpu_seen_interval_avg_us=93.87
event_sync_pure status=0 host_total_us=851.27 host_per_task_us=106.41 aicpu_seen_interval_avg_us=93.89
event_sync_with_flag_check status=0 host_total_us=800.62 host_per_task_us=100.08 aicpu_seen_interval_avg_us=95.41
iter=18
shared status=0 host_total_us=771.00 host_per_task_us=96.38 aicpu_seen_interval_avg_us=93.84
event_sync_pure status=0 host_total_us=856.84 host_per_task_us=107.11 aicpu_seen_interval_avg_us=94.69
event_sync_with_flag_check status=0 host_total_us=797.70 host_per_task_us=99.71 aicpu_seen_interval_avg_us=96.18
iter=19
shared status=0 host_total_us=787.37 host_per_task_us=98.42 aicpu_seen_interval_avg_us=95.45
event_sync_pure status=0 host_total_us=850.47 host_per_task_us=106.31 aicpu_seen_interval_avg_us=93.95
event_sync_with_flag_check status=0 host_total_us=799.07 host_per_task_us=99.88 aicpu_seen_interval_avg_us=95.17
summary: shared_avg_us=811.30 event_sync_pure_avg_us=852.68 event_sync_with_flag_check_avg_us=833.12 speedup_event_pure_over_shared=1.05 speedup_event_flag_check_over_shared=1.03
last_shared_detail status=0 host_total_us=787.37 host_per_task_us=98.42 aicpu_seen_interval_avg_us=95.45
last_event_sync_pure_detail status=0 host_total_us=850.47 host_per_task_us=106.31 aicpu_seen_interval_avg_us=93.95
last_event_sync_with_flag_check_detail status=0 host_total_us=799.07 host_per_task_us=99.88 aicpu_seen_interval_avg_us=95.17


