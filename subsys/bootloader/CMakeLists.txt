#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory(c_runtime_setup)
add_subdirectory(provision)
add_subdirectory(fprotect)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_CRYPTO bl_crypto)
add_subdirectory_ifdef(CONFIG_SECURE_BOOT_DEBUG debug)

if(IMAGE)
  set(KERNEL_ELF ${IMAGE}kernel_elf)
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
    )
  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    ${KERNEL_ELF}
    )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/fw_info_magic.cmake)
zephyr_include_directories(include)
