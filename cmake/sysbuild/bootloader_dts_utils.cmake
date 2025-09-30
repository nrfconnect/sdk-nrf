# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file contains utility functions used for handling
# data coming from the devicetree for configurations using
# a bootloader.

function(dt_get_parent node)
  string(FIND "${${node}}" "/" pos REVERSE)

  if(pos EQUAL -1)
    message(FATAL_ERROR "Unable to get parent of node: ${${node}}")
  endif()

  string(SUBSTRING "${${node}}" 0 ${pos} ${node})
  set(${node} "${${node}}" PARENT_SCOPE)
endfunction()

# Usage:
#   dt_partition_addr(<var> [LABEL <label>|PATH <path>] [TARGET <target>] [ABSOLUTE])
#
# Get the partition address, based on the path or label.
# The value will be returned in the <var> parameter.
# The returned address is either relative to the memory controller, or absolute if ABSOLUTE is
# specified.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# Results can be:
# - The base address of the register block
# - <var>-NOTFOUND if node does not exists or does not have a register block at the requested index
#   or with the requested name
#
# <var>          : Return variable where the address value will be stored
# LABEL <label>  : Node label
# PATH <path>    : Node path
# TARGET <target>: Optional target to retrieve devicetree information from
# ABSOLUTE       : Return absolute address rather than address relative to the memory controller
# REQUIRED       : Generate a fatal error if the node is not found
function(dt_partition_addr var)
  cmake_parse_arguments(arg_DT_PARTITION "ABSOLUTE;REQUIRED" "LABEL;PATH;TARGET" "" ${ARGN})

  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} arg_DT_PARTITION PATH LABEL)
  zephyr_check_arguments_exclusive(${CMAKE_CURRENT_FUNCTION} arg_DT_PARTITION PATH LABEL)

  if(NOT arg_DT_PARTITION_PATH)
    # Calculate the partition path, based on the label.
    dt_nodelabel(arg_DT_PARTITION_PATH NODELABEL "${arg_DT_PARTITION_LABEL}" TARGET
      "${arg_DT_PARTITION_TARGET}")
    if(NOT DEFINED arg_DT_PARTITION_PATH)
      if(arg_DT_PARTITION_REQUIRED)
        message(FATAL_ERROR "dt_partition_addr: Unable to find a node with label: "
                            "${arg_DT_PARTITION_LABEL} for target: ${arg_DT_PARTITION_TARGET}")
      else()
        set(${var} "${var}-NOTFOUND" PARENT_SCOPE)
        return()
      endif()
    endif()
  else()
    # Check if a partition with the given path exists.
    dt_node_exists(arg_DT_PARTITION_EXISTS PATH "${arg_DT_PARTITION_PATH}" TARGET
      "${arg_DT_PARTITION_TARGET}")
    if(NOT arg_DT_PARTITION_EXISTS)
      if(arg_DT_PARTITION_REQUIRED)
        message(FATAL_ERROR "dt_partition_addr: Unable to find a node with path: "
                            "${arg_DT_PARTITION_PATH} for target: ${arg_DT_PARTITION_TARGET}")
      else()
        set(${var} "${var}-NOTFOUND" PARENT_SCOPE)
        return()
      endif()
    endif()
  endif()

  # Get the list of partitions and subpartitions.
  dt_comp_path(fixed_partitions COMPATIBLE "fixed-partitions" TARGET "${arg_DT_PARTITION_TARGET}")
  dt_comp_path(fixed_subpartitions COMPATIBLE "fixed-subpartitions" TARGET
    "${arg_DT_PARTITION_TARGET}")

  # Read the partition offset.
  dt_reg_addr(dt_partition_offset PATH "${arg_DT_PARTITION_PATH}" TARGET
    "${arg_DT_PARTITION_TARGET}")

  # The partition parent should be either a fixed-partitions or a fixed-subpartitions node.
  set(dt_partition_parent "${arg_DT_PARTITION_PATH}")
  dt_get_parent(dt_partition_parent)

  if("${dt_partition_parent}" IN_LIST fixed_subpartitions)
    # If the parent is a subpartition, add the parent partition address.
    dt_reg_addr(parent_addr PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}")
    math(EXPR dt_partition_offset "${dt_partition_offset} + ${parent_addr}" OUTPUT_FORMAT
      HEXADECIMAL)

    # Get the parent of the subpartition node, which should be a fixed-partitions node.
    dt_get_parent(dt_partition_parent)
  elseif(NOT "${dt_partition_parent}" IN_LIST fixed_partitions)
    message(FATAL_ERROR "dt_partition_addr: Node is not a partition or subpartition: "
                        "${arg_DT_PARTITION_PATH} in target: ${arg_DT_PARTITION_TARGET}")
  endif()

  if(arg_DT_PARTITION_ABSOLUTE)
    # A parent of the "fixed-partitions" node should be the memory controller.
    dt_get_parent(dt_partition_parent)
    # Add the memory controller base address to get an absolute address.
    dt_reg_addr(parent_addr PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}")
    math(EXPR dt_partition_offset "${dt_partition_offset} + ${parent_addr}" OUTPUT_FORMAT
      HEXADECIMAL)
  endif()

  set(${var} "${dt_partition_offset}" PARENT_SCOPE)
endfunction()

function(get_address_from_dt_partition_nodelabel label address)
  dt_partition_addr(${address} LABEL "${label}" TARGET "${DEFAULT_IMAGE}" ABSOLUTE REQUIRED)
  set(${address} "${${address}}" PARENT_SCOPE)
endfunction()
