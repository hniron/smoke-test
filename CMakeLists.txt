cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CANN_INSTALL_PATH)
    if(DEFINED ENV{CANN_INSTALL_PATH})
        set(CANN_INSTALL_PATH "$ENV{CANN_INSTALL_PATH}")
    elseif(DEFINED ENV{ASCEND_HOME_PATH})
        set(CANN_INSTALL_PATH "$ENV{ASCEND_HOME_PATH}")
    else()
        set(CANN_INSTALL_PATH "/usr/local/Ascend/cann")
    endif()
endif()

if(EXISTS "${CANN_INSTALL_PATH}")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/aarch64-linux")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/aarch64-linux/asc")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/cmake")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/tools/ascend_project/cmake")
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/tools/op_project_templates/ascendc/customize/cmake")
endif()

find_package(ASC REQUIRED)
project(simt_test LANGUAGES ASC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NPU_ARCH "dav-3510" CACHE STRING "Ascend C NPU arch, for example dav-3510 for A5/Atlas 350")
set(ASC_ARCH_FLAG "--npu-arch" CACHE STRING "Ascend C arch option, for example --npu-arch or --cce-aicore-arch")
set(AICPU_VENDOR "cust" CACHE STRING "CANN custom OPP vendor name")
option(ENABLE_SIMT "Compile ASC sources with --enable-simt" ON)

function(append_existing out_var)
    foreach(_dir ${ARGN})
        if(EXISTS "${_dir}")
            list(APPEND ${out_var} "${_dir}")
        endif()
    endforeach()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

set(CANN_ROOT_CANDIDATES "${CANN_INSTALL_PATH}")
if(DEFINED ASCEND_CANN_PACKAGE_LINUX_PATH)
    list(APPEND CANN_ROOT_CANDIDATES "${ASCEND_CANN_PACKAGE_LINUX_PATH}")
    get_filename_component(_CANN_ROOT_FROM_LINUX "${ASCEND_CANN_PACKAGE_LINUX_PATH}" DIRECTORY)
    list(APPEND CANN_ROOT_CANDIDATES "${_CANN_ROOT_FROM_LINUX}")
endif()
list(REMOVE_DUPLICATES CANN_ROOT_CANDIDATES)

set(CANN_INCLUDE_DIRS "")
set(SIMT_INCLUDE_DIRS "")
set(CANN_LIB_DIRS "")
foreach(_cann_root IN LISTS CANN_ROOT_CANDIDATES)
    append_existing(CANN_INCLUDE_DIRS
        "${_cann_root}/include"
        "${_cann_root}/aarch64-linux/include"
        "${_cann_root}/compiler/ascendc/include"
        "${_cann_root}/compiler/ascendc/include/basic_api"
        "${_cann_root}/compiler/ascendc/include/basic_api/impl"
        "${_cann_root}/compiler/asc/include"
        "${_cann_root}/compiler/asc/impl/basic_api"
        "${_cann_root}/tools/tikcpp/tikcfw/interface"
    )
    append_existing(SIMT_INCLUDE_DIRS
        "${_cann_root}/aarch64-linux/asc/include"
        "${_cann_root}/aarch64-linux/asc"
        "${_cann_root}/asc/include"
        "${_cann_root}/asc"
    )
    append_existing(CANN_LIB_DIRS
        "${_cann_root}/lib64"
        "${_cann_root}/aarch64-linux/lib64"
    )
endforeach()
list(REMOVE_DUPLICATES CANN_INCLUDE_DIRS)
list(REMOVE_DUPLICATES SIMT_INCLUDE_DIRS)
list(REMOVE_DUPLICATES CANN_LIB_DIRS)
message(STATUS "CANN include dirs: ${CANN_INCLUDE_DIRS}")
message(STATUS "SIMT include dirs: ${SIMT_INCLUDE_DIRS}")
message(STATUS "CANN lib dirs: ${CANN_LIB_DIRS}")

function(configure_visibility_target target_name)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CANN_INCLUDE_DIRS}
    )
    target_link_directories(${target_name} PRIVATE
        ${CANN_LIB_DIRS}
    )
    target_link_libraries(${target_name} PRIVATE
        ascendcl
        acl_op_compiler
    )
    target_compile_options(${target_name} PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:${ASC_ARCH_FLAG}=${NPU_ARCH}>
    )
endfunction()

add_executable(aiv_flag_visibility
    bench_main.asc
    aiv_flags.asc
)
target_compile_definitions(aiv_flag_visibility PRIVATE ENABLE_AIV_PRODUCER)
configure_visibility_target(aiv_flag_visibility)

add_executable(simt_store_visibility
    bench_main.asc
    simt_store_flags.asc
)
target_compile_definitions(simt_store_visibility PRIVATE ENABLE_SIMT_STORE_PRODUCER)
configure_visibility_target(simt_store_visibility)

add_executable(simt_atomic_visibility
    bench_main.asc
    simt_atomic_flags.asc
)
target_compile_definitions(simt_atomic_visibility PRIVATE ENABLE_SIMT_ATOMIC_PRODUCER)
configure_visibility_target(simt_atomic_visibility)

if(ENABLE_SIMT)
    set(_SIMT_FLAGS "--enable-simt")
    foreach(_simt_include_dir IN LISTS SIMT_INCLUDE_DIRS)
        if(EXISTS "${_simt_include_dir}")
            string(APPEND _SIMT_FLAGS " -idirafter ${_simt_include_dir}")
        endif()
    endforeach()
    set_source_files_properties(simt_store_flags.asc simt_atomic_flags.asc PROPERTIES COMPILE_FLAGS "${_SIMT_FLAGS}")
endif()

set(CANN_LINUX_PATHS "")
if(DEFINED ASCEND_CANN_PACKAGE_LINUX_PATH)
    list(APPEND CANN_LINUX_PATHS "${ASCEND_CANN_PACKAGE_LINUX_PATH}")
endif()
list(APPEND CANN_LINUX_PATHS
    "${CANN_INSTALL_PATH}"
    "${CANN_INSTALL_PATH}/aarch64-linux"
)
list(REMOVE_DUPLICATES CANN_LINUX_PATHS)

set(AICPU_INCLUDE_CANDIDATES "")
foreach(_base IN LISTS CANN_LINUX_PATHS)
    list(APPEND AICPU_INCLUDE_CANDIDATES
        "${_base}/include"
        "${_base}/include/aicpu"
        "${_base}/include/aicpu_common/context"
        "${_base}/include/aicpu_common/context/common"
        "${_base}/include/aicpu_common/context/cpu_proto"
        "${_base}/include/aicpu_common/context/utils"
        "${_base}/include/experiment"
        "${_base}/include/experiment/cce"
        "${_base}/include/experiment/msprof"
        "${_base}/pkg_inc"
        "${_base}/pkg_inc/aicpu"
        "${_base}/pkg_inc/aicpu_common/context"
        "${_base}/pkg_inc/aicpu_common/context/common"
        "${_base}/pkg_inc/aicpu_common/context/cpu_proto"
        "${_base}/pkg_inc/aicpu_common/context/utils"
    )
endforeach()

set(AICPU_INCLUDE_DIRS "")
foreach(_dir IN LISTS AICPU_INCLUDE_CANDIDATES)
    if(EXISTS "${_dir}")
        list(APPEND AICPU_INCLUDE_DIRS "${_dir}")
    endif()
endforeach()
list(REMOVE_DUPLICATES AICPU_INCLUDE_DIRS)

find_file(AICPU_CPU_KERNEL_H
    NAMES cpu_kernel.h
    PATHS ${AICPU_INCLUDE_DIRS}
    NO_DEFAULT_PATH
)
if(NOT AICPU_CPU_KERNEL_H)
    message(FATAL_ERROR "cpu_kernel.h was not found under ${CANN_LINUX_PATHS}")
endif()

set(AICPU_LIB_CANDIDATES "")
foreach(_base IN LISTS CANN_LINUX_PATHS)
    list(APPEND AICPU_LIB_CANDIDATES
        "${_base}/lib64"
        "${_base}/ops_base/lib64"
    )
endforeach()
list(REMOVE_DUPLICATES AICPU_LIB_CANDIDATES)

find_library(AICPU_CONTEXT_LIB
    NAMES aicpu_context
    PATHS ${AICPU_LIB_CANDIDATES}
    NO_DEFAULT_PATH
)
find_library(BASE_ASCEND_PROTOBUF_LIB
    NAMES base_ascend_protobuf
    PATHS ${AICPU_LIB_CANDIDATES}
    NO_DEFAULT_PATH
)
if(NOT AICPU_CONTEXT_LIB OR NOT BASE_ASCEND_PROTOBUF_LIB)
    message(FATAL_ERROR
        "Required AICPU libs were not found. "
        "AICPU_CONTEXT_LIB=${AICPU_CONTEXT_LIB}, "
        "BASE_ASCEND_PROTOBUF_LIB=${BASE_ASCEND_PROTOBUF_LIB}, "
        "searched=${AICPU_LIB_CANDIDATES}")
endif()

add_library(simt_test_aicpu_poll_kernel_obj OBJECT
    aicpu_poll_kernel.cc
)
set_target_properties(simt_test_aicpu_poll_kernel_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(simt_test_aicpu_poll_kernel_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AICPU_INCLUDE_DIRS}
)
target_compile_definitions(simt_test_aicpu_poll_kernel_obj PRIVATE
    _FORTIFY_SOURCE=2
    _GLIBCXX_USE_CXX11_ABI=1
    google=ascend_private
)
target_compile_options(simt_test_aicpu_poll_kernel_obj PRIVATE
    -O2
    -DDISABLE_COMPILE_V1
    -Dgoogle=ascend_private
    -fvisibility=hidden
)

set(AICPU_KERNEL_SO "${CMAKE_CURRENT_BINARY_DIR}/libsimt_test_aicpu_poll_kernel.so")
add_custom_command(
    OUTPUT "${AICPU_KERNEL_SO}"
    COMMAND ${CMAKE_CXX_COMPILER} -shared $<TARGET_OBJECTS:simt_test_aicpu_poll_kernel_obj>
        -Wl,--whole-archive
        "${AICPU_CONTEXT_LIB}"
        "${BASE_ASCEND_PROTOBUF_LIB}"
        -Wl,--no-whole-archive
        -Wl,-Bsymbolic
        -Wl,--exclude-libs=libbase_ascend_protobuf.a
        -s
        -o "${AICPU_KERNEL_SO}"
    DEPENDS simt_test_aicpu_poll_kernel_obj
    COMMENT "Linking CUST AICPU kernel ${AICPU_KERNEL_SO}"
    VERBATIM
)
add_custom_target(simt_test_aicpu_poll_kernel ALL
    DEPENDS "${AICPU_KERNEL_SO}"
)

find_library(C_SEC_LIB
    NAMES c_sec
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(REGISTER_LIB
    NAMES register
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(RT2_REGISTRY_STATIC_LIB
    NAMES rt2_registry_static
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(RT2_REGISTRY_LIB
    NAMES rt2_registry
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(EXE_GRAPH_LIB
    NAMES exe_graph
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(GRAPH_LIB
    NAMES graph
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(GRAPH_BASE_LIB
    NAMES graph_base
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(ASCENDALOG_LIB
    NAMES ascendalog
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(ERROR_MANAGER_LIB
    NAMES error_manager
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(PLATFORM_LIB
    NAMES platform
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
if(NOT C_SEC_LIB OR NOT REGISTER_LIB)
    message(FATAL_ERROR
        "Required CUST op proto libs were not found. "
        "C_SEC_LIB=${C_SEC_LIB}, REGISTER_LIB=${REGISTER_LIB}, searched=${CANN_LIB_DIRS}")
endif()

add_library(simt_test_aicpu_poll_proto SHARED
    aicpu_poll_proto.cpp
)
target_include_directories(simt_test_aicpu_poll_proto PRIVATE
    ${CANN_INCLUDE_DIRS}
)
target_compile_options(simt_test_aicpu_poll_proto PRIVATE
    -fvisibility=hidden
)
target_compile_definitions(simt_test_aicpu_poll_proto PRIVATE
    LOG_CPP
    PROCESS_LOG
)
target_link_libraries(simt_test_aicpu_poll_proto PRIVATE
    "${C_SEC_LIB}"
)
if(RT2_REGISTRY_STATIC_LIB)
    target_link_libraries(simt_test_aicpu_poll_proto PRIVATE
        -Wl,--whole-archive
        "${RT2_REGISTRY_STATIC_LIB}"
        -Wl,--no-whole-archive
    )
elseif(RT2_REGISTRY_LIB)
    target_link_libraries(simt_test_aicpu_poll_proto PRIVATE
        -Wl,--whole-archive
        "${RT2_REGISTRY_LIB}"
        -Wl,--no-whole-archive
    )
else()
    message(WARNING "Neither rt2_registry_static nor rt2_registry was found; op proto registration may fail.")
endif()
target_link_libraries(simt_test_aicpu_poll_proto PRIVATE
    -Wl,--no-as-needed
    "${REGISTER_LIB}"
)
foreach(_proto_lib IN ITEMS
    EXE_GRAPH_LIB GRAPH_LIB GRAPH_BASE_LIB ASCENDALOG_LIB ERROR_MANAGER_LIB PLATFORM_LIB)
    if(${_proto_lib})
        target_link_libraries(simt_test_aicpu_poll_proto PRIVATE "${${_proto_lib}}")
    endif()
endforeach()
target_link_libraries(simt_test_aicpu_poll_proto PRIVATE
    -Wl,--as-needed
)
set_target_properties(simt_test_aicpu_poll_proto PROPERTIES
    OUTPUT_NAME "cust_opsproto_rt2.0"
)

set(CUST_AICPU_KERNEL_JSON "${CMAKE_CURRENT_BINARY_DIR}/cust_aicpu_kernel.json")
file(WRITE "${CUST_AICPU_KERNEL_JSON}"
"{
    \"AivAicpuNoop\": {
        \"opInfo\": {
            \"computeCost\": \"100\",
            \"engine\": \"DNN_VM_AICPU\",
            \"flagAsync\": \"False\",
            \"flagPartial\": \"False\",
            \"functionName\": \"RunCpuKernel\",
            \"kernelSo\": \"libsimt_test_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"CUSTAICPUKernel\",
            \"userDefined\": \"True\",
            \"workspaceSize\": \"100\"
        }
    },
    \"AivAicpuPollFlags\": {
        \"opInfo\": {
            \"computeCost\": \"100\",
            \"engine\": \"DNN_VM_AICPU\",
            \"flagAsync\": \"False\",
            \"flagPartial\": \"False\",
            \"functionName\": \"RunCpuKernel\",
            \"kernelSo\": \"libsimt_test_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"CUSTAICPUKernel\",
            \"userDefined\": \"True\",
            \"workspaceSize\": \"100\"
        }
    }
}
")

set(CUST_OPP_ROOT "${CMAKE_CURRENT_BINARY_DIR}/custom_opp")
set(CUST_OPP_VENDOR_ROOT "${CUST_OPP_ROOT}/vendors/${AICPU_VENDOR}")
set(CUST_OPP_CPU_ROOT "${CUST_OPP_VENDOR_ROOT}/op_impl/cpu")
set(CUST_OPP_PROTO_ROOT "${CUST_OPP_VENDOR_ROOT}/op_proto")
set(CUST_OPP_JSON "${CUST_OPP_CPU_ROOT}/config/cust_aicpu_kernel.json")
set(CUST_OPP_AICPU_SO "${CUST_OPP_CPU_ROOT}/aicpu_kernel/impl/libsimt_test_aicpu_poll_kernel.so")
set(CUST_OPP_PROTO_SO "${CUST_OPP_PROTO_ROOT}/lib/linux/${CMAKE_SYSTEM_PROCESSOR}/libcust_opsproto_rt2.0.so")

add_custom_command(
    OUTPUT "${CUST_OPP_JSON}" "${CUST_OPP_AICPU_SO}" "${CUST_OPP_PROTO_SO}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CUST_OPP_CPU_ROOT}/config"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CUST_OPP_CPU_ROOT}/aicpu_kernel/impl"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CUST_OPP_PROTO_ROOT}/lib/linux/${CMAKE_SYSTEM_PROCESSOR}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CUST_AICPU_KERNEL_JSON}" "${CUST_OPP_JSON}"
    COMMAND ${CMAKE_COMMAND} -E copy "${AICPU_KERNEL_SO}" "${CUST_OPP_AICPU_SO}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:simt_test_aicpu_poll_proto>" "${CUST_OPP_PROTO_SO}"
    DEPENDS "${CUST_AICPU_KERNEL_JSON}" "${AICPU_KERNEL_SO}" simt_test_aicpu_poll_proto
    COMMENT "Staging CUST AICPU custom OPP under ${CUST_OPP_VENDOR_ROOT}"
    VERBATIM
)
add_custom_target(custom_opp ALL
    DEPENDS "${CUST_OPP_JSON}" "${CUST_OPP_AICPU_SO}" "${CUST_OPP_PROTO_SO}"
)
add_dependencies(custom_opp simt_test_aicpu_poll_kernel)

add_custom_target(install_aicpu_package
    COMMAND ${CMAKE_COMMAND} -E echo "Raw AICPU package install is disabled. Use custom_opp and set ASCEND_CUSTOM_OPP_PATH=${CUST_OPP_VENDOR_ROOT}."
    DEPENDS custom_opp
)
