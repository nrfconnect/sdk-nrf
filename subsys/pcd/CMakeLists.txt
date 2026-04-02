#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  src/pcd.c
)

if(CONFIG_PCD_APP AND NOT CONFIG_PARTITION_MANAGER_ENABLED)
  # Read s0 address from b0n image
  add_custom_target(b0n_target)
  zephyr_dt_import(EDT_PICKLE_FILE ${CMAKE_BINARY_DIR}/../b0n/zephyr/edt.pickle TARGET b0n_target)
  dt_nodelabel(s0_partition_path NODELABEL s0_partition REQUIRED TARGET b0n_target)
  dt_reg_addr(s0_partition_address PATH ${s0_partition_path} TARGET b0n_target)
  # TODO: Likely need rework when mapped-partition is available/used
  dt_chosen(flash_path PROPERTY "zephyr,flash" TARGET b0n_target)
  dt_reg_addr(flash_address PATH ${flash_path} TARGET b0n_target)
  math(EXPR s0_offset_address "${s0_partition_address} - ${flash_address}" OUTPUT_FORMAT HEXADECIMAL)
  zephyr_library_compile_options("-DPCD_NET_CORE_APP_OFFSET=${s0_offset_address}")
endif()
