# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file contains utility functions used for handling
# data coming from the devicetree for configurations using
# a bootloader.

function(get_address_from_dt_partition_nodelabel label address)
  dt_nodelabel(partition_node TARGET ${DEFAULT_IMAGE} NODELABEL ${label} REQUIRED)
  dt_reg_addr(partition_offset TARGET ${DEFAULT_IMAGE} PATH ${partition_node})

  # Get the parent "two levels up" (../../) of the partition node
  # This is the partition flash area node
  string(REPLACE "/" ";" partition_node_split ${partition_node})
  list(LENGTH partition_node_split child_path_length)
  math(EXPR parent_path_length "${child_path_length} - 2")
  list(SUBLIST partition_node_split 0 ${parent_path_length} parent_path_split)
  string(REPLACE ";" "/" flash_area_node "${parent_path_split}")

  dt_reg_addr(flash_area_addr TARGET ${DEFAULT_IMAGE} PATH ${flash_area_node})

  math(EXPR ${address} "${flash_area_addr} + ${partition_offset}")
  set(${address} ${${address}} PARENT_SCOPE)
endfunction()
