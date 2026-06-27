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
    list(APPEND CMAKE_PREFIX_PATH "${CANN_INSTALL_PATH}/cmake")
endif()

find_package(ASC REQUIRED)
project(aiv_aicpu_bench LANGUAGES ASC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NPU_ARCH "dav-c220" CACHE STRING "Ascend C AICore arch value, for example dav-c220 for Ascend 910B3")
set(ASC_ARCH_FLAG "--cce-aicore-arch" CACHE STRING "Ascend C architecture option, for example --cce-aicore-arch or --npu-arch")
set(AICPU_VENDOR "cust" CACHE STRING "CANN custom vendor name used under opp/vendors")
set(AICPU_PACKAGE_NAME "aicpu_aiv_aicpu_bench.tar.gz" CACHE STRING "AICPU package name registered in ascend_package_load.ini")

add_library(aiv_aicpu_poll_kernel SHARED
    aicpu_poll_kernel.cc
)

target_include_directories(aiv_aicpu_poll_kernel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(aiv_aicpu_bench
    bench_main.asc
)

target_include_directories(aiv_aicpu_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CANN_INSTALL_PATH}/include
)

target_link_directories(aiv_aicpu_bench PRIVATE
    ${CANN_INSTALL_PATH}/lib64
)

target_link_libraries(aiv_aicpu_bench PRIVATE
    ascendcl
)

target_compile_options(aiv_aicpu_bench PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:${ASC_ARCH_FLAG}=${NPU_ARCH}>
)

set(AICPU_KERNEL_JSON "${CMAKE_CURRENT_BINARY_DIR}/libaiv_aicpu_poll_kernel.json")
file(WRITE "${AICPU_KERNEL_JSON}"
"{
    \"PollFlags\": {
        \"opInfo\": {
            \"functionName\": \"PollFlags\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    },
    \"StampFlag\": {
        \"opInfo\": {
            \"functionName\": \"StampFlag\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    },
    \"StampOnly\": {
        \"opInfo\": {
            \"functionName\": \"StampOnly\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    },
    \"ReadBench\": {
        \"opInfo\": {
            \"functionName\": \"ReadBench\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    },
    \"ReadChase\": {
        \"opInfo\": {
            \"functionName\": \"ReadChase\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    },
    \"Noop\": {
        \"opInfo\": {
            \"functionName\": \"Noop\",
            \"kernelSo\": \"libaiv_aicpu_poll_kernel.so\",
            \"opKernelLib\": \"AICPUKernel\"
        }
    }
}
")

add_custom_target(aicpu_kernel_json ALL
    DEPENDS "${AICPU_KERNEL_JSON}"
)

set(AICPU_PACKAGE_STAGE "${CMAKE_CURRENT_BINARY_DIR}/_aicpu_package_stage")
set(AICPU_PACKAGE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${AICPU_PACKAGE_NAME}")

add_custom_command(
    OUTPUT "${AICPU_PACKAGE_FILE}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${AICPU_PACKAGE_STAGE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${AICPU_PACKAGE_STAGE}/aicpu_kernels_device"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:aiv_aicpu_poll_kernel>" "${AICPU_PACKAGE_STAGE}/aicpu_kernels_device/"
    COMMAND ${CMAKE_COMMAND} -E chdir "${AICPU_PACKAGE_STAGE}" ${CMAKE_COMMAND} -E tar cfz "${AICPU_PACKAGE_FILE}" aicpu_kernels_device
    DEPENDS aiv_aicpu_poll_kernel
    COMMENT "Packing AICPU kernel package ${AICPU_PACKAGE_NAME}"
    VERBATIM
)

add_custom_target(aicpu_kernel_package ALL
    DEPENDS "${AICPU_PACKAGE_FILE}" "${AICPU_KERNEL_JSON}"
)

add_custom_target(install_aicpu_package
    COMMAND ${CMAKE_COMMAND}
        "-DCANN_INSTALL_PATH=${CANN_INSTALL_PATH}"
        "-DAICPU_VENDOR=${AICPU_VENDOR}"
        "-DAICPU_PACKAGE_NAME=${AICPU_PACKAGE_NAME}"
        "-DAICPU_PACKAGE_FILE=${AICPU_PACKAGE_FILE}"
        "-DAICPU_KERNEL_JSON=${AICPU_KERNEL_JSON}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_aicpu_package.cmake"
    DEPENDS aicpu_kernel_package
    COMMENT "Installing AICPU package into CANN opp/vendors/${AICPU_VENDOR}/aicpu"
    VERBATIM
)

