if(NOT DEFINED CANN_INSTALL_PATH OR CANN_INSTALL_PATH STREQUAL "")
    message(FATAL_ERROR "CANN_INSTALL_PATH is required")
endif()

if(NOT DEFINED AICPU_VENDOR OR AICPU_VENDOR STREQUAL "")
    message(FATAL_ERROR "AICPU_VENDOR is required")
endif()

if(NOT EXISTS "${AICPU_PACKAGE_FILE}")
    message(FATAL_ERROR "AICPU package does not exist: ${AICPU_PACKAGE_FILE}")
endif()

if(NOT EXISTS "${AICPU_KERNEL_JSON}")
    message(FATAL_ERROR "AICPU json does not exist: ${AICPU_KERNEL_JSON}")
endif()

set(aicpu_root "${CANN_INSTALL_PATH}/opp/vendors/${AICPU_VENDOR}/aicpu")
set(aicpu_kernel_dir "${aicpu_root}/kernel")
set(aicpu_config_dir "${aicpu_root}/config")

file(MAKE_DIRECTORY "${aicpu_kernel_dir}")
file(MAKE_DIRECTORY "${aicpu_config_dir}")

file(INSTALL
    DESTINATION "${aicpu_kernel_dir}"
    TYPE FILE
    FILES "${AICPU_PACKAGE_FILE}"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

file(INSTALL
    DESTINATION "${aicpu_config_dir}"
    TYPE FILE
    FILES "${AICPU_KERNEL_JSON}"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

set(load_ini "${CANN_INSTALL_PATH}/conf/ascend_package_load.ini")
set(package_path "opp/vendors/${AICPU_VENDOR}/aicpu/kernel")
set(load_entry "name:${AICPU_PACKAGE_NAME}\ninstall_path:2\noptional:true\npackage_path:${package_path}\nload_as_per_soc:false\n")

if(EXISTS "${load_ini}")
    file(READ "${load_ini}" load_ini_content)
else()
    set(load_ini_content "")
endif()

string(FIND "${load_ini_content}" "name:${AICPU_PACKAGE_NAME}" existing_entry_pos)
if(existing_entry_pos EQUAL -1)
    file(APPEND "${load_ini}" "\n${load_entry}")
    message(STATUS "Appended AICPU package entry to ${load_ini}")
else()
    message(STATUS "AICPU package entry already exists in ${load_ini}")
endif()

message(STATUS "Installed ${AICPU_PACKAGE_NAME} to ${aicpu_kernel_dir}")
message(STATUS "Installed ${AICPU_KERNEL_JSON} to ${aicpu_config_dir}")
