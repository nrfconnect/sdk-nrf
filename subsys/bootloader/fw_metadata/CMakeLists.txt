#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

include(${NRF_DIR}/subsys/bootloader/cmake/fw_metadata.cmake)

target_compile_definitions(zephyr_interface
  INTERFACE
  FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
  POINTER_MAGIC=${POINTER_MAGIC}
  VALIDATION_INFO_MAGIC=${VALIDATION_INFO_MAGIC}
  )

add_library(fw_metadata fw_metadata.c)
target_include_directories(fw_metadata PUBLIC ../include)
target_link_libraries(fw_metadata PRIVATE zephyr_interface)

zephyr_sources(fw_metadata.c)
zephyr_include_directories(../include)
