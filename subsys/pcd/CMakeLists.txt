#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  src/pcd.c
)

if(CONFIG_PCD_APP)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

  # Read s0 address from b0n image
  add_custom_target(b0n_target)
  zephyr_dt_import(EDT_PICKLE_FILE ${CMAKE_BINARY_DIR}/../b0n/zephyr/edt.pickle TARGET b0n_target)
  dt_nodelabel(s0_partition_path NODELABEL s0_partition REQUIRED TARGET b0n_target)
  dt_partition_addr(s0_partition_offset PATH ${s0_partition_path} TARGET b0n_target REQUIRED)
  zephyr_library_compile_options("-DPCD_NET_CORE_APP_OFFSET=${s0_partition_offset}")
endif()
