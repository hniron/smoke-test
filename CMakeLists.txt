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
project(rdma_urma_bench LANGUAGES ASC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NPU_ARCH "dav-3510" CACHE STRING "Ascend C NPU arch, for example dav-3510 for A5/Atlas 350")
set(ASC_ARCH_FLAG "--npu-arch" CACHE STRING "Ascend C arch option, for example --npu-arch or --cce-aicore-arch")
option(ENABLE_HIXL "Enable HIXL based host/device transfer comm ops" OFF)
set(DEFAULT_HIXL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../a5-sendrecv-transport-hacking/hixl")
if(NOT EXISTS "${DEFAULT_HIXL_ROOT}/include/hixl/hixl.h")
    set(DEFAULT_HIXL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../hixl/hixl-master/hixl-master")
endif()
set(HIXL_ROOT "${DEFAULT_HIXL_ROOT}" CACHE PATH
    "Path to HIXL source/install root")
option(ENABLE_HCOMM "Enable HCOMM endpoint/memory smoke test and primitive comm ops" OFF)
set(HCOMM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../hcomm/hcomm-master/hcomm-master" CACHE PATH
    "Path to HCOMM source/install root")

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
    append_existing(CANN_LIB_DIRS
        "${_cann_root}/lib64"
        "${_cann_root}/aarch64-linux/lib64"
    )
endforeach()
list(REMOVE_DUPLICATES CANN_INCLUDE_DIRS)
list(REMOVE_DUPLICATES CANN_LIB_DIRS)

find_library(ASCEND_RUNTIME_LIB
    NAMES runtime
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
find_library(PROFAPI_LIB
    NAMES profapi
    PATHS ${CANN_LIB_DIRS}
    NO_DEFAULT_PATH
)
set(ASCEND_HOST_LAUNCH_LIBS "")
foreach(_host_launch_lib IN ITEMS ASCEND_RUNTIME_LIB PROFAPI_LIB)
    if(${_host_launch_lib})
        list(APPEND ASCEND_HOST_LAUNCH_LIBS "${${_host_launch_lib}}")
    endif()
endforeach()

add_executable(rdma_urma_bench
    bench_main.asc
)

target_include_directories(rdma_urma_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CANN_INCLUDE_DIRS}
)

target_link_directories(rdma_urma_bench PRIVATE
    ${CANN_LIB_DIRS}
)

target_link_libraries(rdma_urma_bench PRIVATE
    ascendcl
    acl_op_compiler
    ${ASCEND_HOST_LAUNCH_LIBS}
)

if(ENABLE_HIXL)
    set(HIXL_INCLUDE_SEARCH_DIRS
        "${HIXL_ROOT}/include"
        "${HIXL_ROOT}/output/include"
        "${HIXL_ROOT}/install/include"
        ${CANN_INCLUDE_DIRS}
    )
    set(HIXL_LIB_SEARCH_DIRS
        "${HIXL_ROOT}/build/src/hixl"
        "${HIXL_ROOT}/build/lib"
        "${HIXL_ROOT}/output/lib"
        "${HIXL_ROOT}/install/lib"
        "${HIXL_ROOT}/lib"
        "${HIXL_ROOT}/lib64"
        ${CANN_LIB_DIRS}
    )
    find_path(HIXL_INCLUDE_DIR
        NAMES hixl/hixl.h
        PATHS ${HIXL_INCLUDE_SEARCH_DIRS}
        NO_DEFAULT_PATH
    )
    find_library(HIXL_CANN_HIXL_LIB
        NAMES cann_hixl
        PATHS ${HIXL_LIB_SEARCH_DIRS}
        NO_DEFAULT_PATH
    )
    set(HIXL_DEP_LIBS "")
    foreach(_hixl_dep IN ITEMS
        graph
        ge_common
        metadef
        error_manager
        hcomm
        ascend_hal
        mmpa
        ascendalog
        c_sec
    )
        find_library(_hixl_dep_lib_${_hixl_dep}
            NAMES ${_hixl_dep}
            PATHS ${HIXL_LIB_SEARCH_DIRS}
            NO_DEFAULT_PATH
        )
        if(_hixl_dep_lib_${_hixl_dep})
            list(APPEND HIXL_DEP_LIBS "${_hixl_dep_lib_${_hixl_dep}}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES HIXL_DEP_LIBS)
    if(NOT HIXL_INCLUDE_DIR)
        message(FATAL_ERROR
            "ENABLE_HIXL=ON but hixl/hixl.h was not found. "
            "Set -DHIXL_ROOT=/path/to/hixl source tree, or install HIXL into CANN. "
            "Checked include dirs: ${HIXL_INCLUDE_SEARCH_DIRS}")
    endif()
    if(NOT HIXL_CANN_HIXL_LIB)
        message(FATAL_ERROR
            "ENABLE_HIXL=ON but libcann_hixl.so was not found. "
            "Build/install HIXL first or set -DHIXL_ROOT. "
            "Checked library dirs: ${HIXL_LIB_SEARCH_DIRS}")
    endif()
    message(STATUS "HIXL_INCLUDE_DIR=${HIXL_INCLUDE_DIR}")
    message(STATUS "HIXL_CANN_HIXL_LIB=${HIXL_CANN_HIXL_LIB}")
    message(STATUS "HIXL_DEP_LIBS=${HIXL_DEP_LIBS}")
    target_compile_definitions(rdma_urma_bench PRIVATE
        ENABLE_HIXL=1
        _GLIBCXX_USE_CXX11_ABI=0
    )
    target_include_directories(rdma_urma_bench PRIVATE
        "${HIXL_INCLUDE_DIR}"
    )
    target_link_libraries(rdma_urma_bench PRIVATE
        "${HIXL_CANN_HIXL_LIB}"
        ${HIXL_DEP_LIBS}
        pthread
        rt
        dl
    )
endif()

if(ENABLE_HCOMM)
    set(HCOMM_INCLUDE_SEARCH_DIRS
        "${HCOMM_ROOT}/include"
        "${HCOMM_ROOT}/output/include"
        "${HCOMM_ROOT}/install/include"
        ${CANN_INCLUDE_DIRS}
    )
    set(HCOMM_LIB_SEARCH_DIRS
        "${HCOMM_ROOT}/build"
        "${HCOMM_ROOT}/build/lib"
        "${HCOMM_ROOT}/build/lib64"
        "${HCOMM_ROOT}/build/src/framework"
        "${HCOMM_ROOT}/build/src"
        "${HCOMM_ROOT}/output/lib"
        "${HCOMM_ROOT}/output/lib64"
        "${HCOMM_ROOT}/install/lib"
        "${HCOMM_ROOT}/install/lib64"
        "${HCOMM_ROOT}/lib"
        "${HCOMM_ROOT}/lib64"
        ${CANN_LIB_DIRS}
    )
    find_path(HCOMM_INCLUDE_DIR
        NAMES hcomm_res.h
        PATHS ${HCOMM_INCLUDE_SEARCH_DIRS}
        NO_DEFAULT_PATH
    )
    find_path(HCOMM_PRIMITIVES_INCLUDE_DIR
        NAMES hcomm_primitives.h
        PATHS ${HCOMM_INCLUDE_SEARCH_DIRS}
        NO_DEFAULT_PATH
    )
    find_library(HCOMM_LIB
        NAMES hcomm
        PATHS ${HCOMM_LIB_SEARCH_DIRS}
        NO_DEFAULT_PATH
    )
    set(HCOMM_DEP_LIBS "")
    foreach(_hcomm_dep IN ITEMS
        mmpa
        ascendalog
        c_sec
        error_manager
        runtime
    )
        find_library(_hcomm_dep_lib_${_hcomm_dep}
            NAMES ${_hcomm_dep}
            PATHS ${HCOMM_LIB_SEARCH_DIRS}
            NO_DEFAULT_PATH
        )
        if(_hcomm_dep_lib_${_hcomm_dep})
            list(APPEND HCOMM_DEP_LIBS "${_hcomm_dep_lib_${_hcomm_dep}}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES HCOMM_DEP_LIBS)
    if(NOT HCOMM_INCLUDE_DIR)
        message(FATAL_ERROR
            "ENABLE_HCOMM=ON but hcomm_res.h was not found. "
            "Set -DHCOMM_ROOT=/path/to/hcomm source tree/install root. "
            "Checked include dirs: ${HCOMM_INCLUDE_SEARCH_DIRS}")
    endif()
    if(NOT HCOMM_PRIMITIVES_INCLUDE_DIR)
        message(FATAL_ERROR
            "ENABLE_HCOMM=ON but hcomm_primitives.h was not found. "
            "Set -DHCOMM_ROOT=/path/to/hcomm source tree/install root. "
            "Checked include dirs: ${HCOMM_INCLUDE_SEARCH_DIRS}")
    endif()
    if(NOT HCOMM_LIB)
        message(FATAL_ERROR
            "ENABLE_HCOMM=ON but libhcomm.so was not found. "
            "Build/install HCOMM first or set -DHCOMM_ROOT. "
            "Checked library dirs: ${HCOMM_LIB_SEARCH_DIRS}")
    endif()
    message(STATUS "HCOMM_INCLUDE_DIR=${HCOMM_INCLUDE_DIR}")
    message(STATUS "HCOMM_PRIMITIVES_INCLUDE_DIR=${HCOMM_PRIMITIVES_INCLUDE_DIR}")
    message(STATUS "HCOMM_LIB=${HCOMM_LIB}")
    message(STATUS "HCOMM_DEP_LIBS=${HCOMM_DEP_LIBS}")
    target_compile_definitions(rdma_urma_bench PRIVATE
        ENABLE_HCOMM=1
    )
    target_include_directories(rdma_urma_bench PRIVATE
        "${HCOMM_INCLUDE_DIR}"
        "${HCOMM_PRIMITIVES_INCLUDE_DIR}"
    )
    target_link_libraries(rdma_urma_bench PRIVATE
        "${HCOMM_LIB}"
        ${HCOMM_DEP_LIBS}
        pthread
        rt
        dl
    )
endif()

target_compile_options(rdma_urma_bench PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:${ASC_ARCH_FLAG}=${NPU_ARCH}>
)
