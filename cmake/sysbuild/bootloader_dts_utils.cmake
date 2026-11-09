# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file contains utility functions used for handling
# data coming from the devicetree for configurations using
# a bootloader.

# Takes a node path to a partition and goes up until a parent with the soc-nv-flash compatible
# is found, then returns the path for that device
function(dt_get_nvm_device node nvm_device dt_source)
  string(FIND "${node}" "/" node_first_slash)
  string(FIND "${node}" "/" node_final_slash REVERSE)
  string(SUBSTRING "${node}" 0 ${node_final_slash} current_path)

  while(${node_final_slash} GREATER ${node_first_slash})
    set(current_compatible)
    dt_prop(current_compatible PATH "${current_path}" PROPERTY "compatible" TARGET ${dt_source})

    if("soc-nv-flash" IN_LIST current_compatible)
      set(${nvm_device} ${current_path} PARENT_SCOPE)
      break()
    elseif("fixed-partitions" IN_LIST current_compatible)
      string(FIND "${current_path}" "/" node_final_slash REVERSE)
      string(SUBSTRING "${current_path}" 0 ${node_final_slash} current_path)
      set(${nvm_device} ${current_path} PARENT_SCOPE)
      break()
    endif()

    string(FIND "${current_path}" "/" node_final_slash REVERSE)
    string(SUBSTRING "${current_path}" 0 ${node_final_slash} current_path)
  endwhile()
endfunction()

# Takes a node path to a partition and goes up until a parent with the soc-nv-flash compatible
# is found, then reads the erase and write block sizes from it
function(dt_get_erase_write_block_sizes node erase_block_size write_block_size dt_source)
  dt_get_nvm_device(${node} nvm_path "${dt_source}")
  if(DEFINED nvm_path)
    dt_prop(erase_size PATH "${nvm_path}" PROPERTY "erase-block-size" TARGET ${dt_source})
    dt_prop(write_size PATH "${nvm_path}" PROPERTY "write-block-size" TARGET ${dt_source})
    set(${erase_block_size} ${erase_size} PARENT_SCOPE)
    set(${write_block_size} ${write_size} PARENT_SCOPE)
  endif()
endfunction()

# Calculate the maximum number of sectors required to update the firmware
function(dt_get_max_sectors var)
  set(image 0)
  set(auto_min_sectors 0)

  if("mcuboot" IN_LIST IMAGES)
    set(dt_source "mcuboot")
  else()
    set(dt_source "${DEFAULT_IMAGE}")
  endif()

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    # If sysbuild configs are available, use them to determine the number of updateable images
    math(EXPR mcuboot_total_images "${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES} +
      ${SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES}")
  elseif(CONFIG_UPDATEABLE_IMAGE_NUMBER)
    # If image-level config is available, use it to determine the number of updateable images
    set(mcuboot_total_images ${CONFIG_UPDATEABLE_IMAGE_NUMBER})
  elseif(CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER)
    set(mcuboot_total_images 1)
  else()
    set(${var} 0 PARENT_SCOPE)
    return()
  endif()

  while(${image} LESS ${mcuboot_total_images})
    set(secondary_slot_flash)
    set(secondary_slot_size)
    set(erase_size_secondary_slot)
    set(write_size_secondary_slot)
    math(EXPR primary_slot "${image} * 2")

    dt_nodelabel(primary_slot_flash NODELABEL "slot${primary_slot}_partition" TARGET ${dt_source})
    if(NOT DEFINED primary_slot_flash)
      message(STATUS
        "slot${primary_slot}_partition not found. Skip reading erase/write block sizes.")
      math(EXPR image "${image} + 1")
      continue()
    endif()

    dt_get_erase_write_block_sizes(${primary_slot_flash} erase_size_primary_slot
      write_size_primary_slot "${dt_source}")

    dt_prop(primary_slot_size PATH "${primary_slot_flash}" PROPERTY "reg" INDEX 1 TARGET
      ${dt_source})
    if(DEFINED primary_slot_size AND DEFINED erase_size_primary_slot)
      math(EXPR remainder "${primary_slot_size} % ${erase_size_primary_slot}")
      if(NOT remainder EQUAL 0)
        message(FATAL_ERROR "slot${primary_slot}_partition size (${primary_slot_size}) is not "
          "divisible by erase block size (${erase_size_primary_slot})"
        )
      endif()
      math(EXPR slot_min_sectors "${primary_slot_size} / ${erase_size_primary_slot}")

      if(${slot_min_sectors} GREATER ${auto_min_sectors})
        set(auto_min_sectors ${slot_min_sectors})
      endif()
    else()
      message(WARNING "Unable to determine erase size/total size of slot${primary_slot} partition")
    endif()

    # Check if the selected bootloader mode requires a secondary slot.
    if(NOT CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP AND NOT
      CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP_RAM_LOAD AND NOT
      CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY
    )
      math(EXPR secondary_slot "${primary_slot} + 1")

      dt_nodelabel(secondary_slot_flash NODELABEL "slot${secondary_slot}_partition" TARGET
        ${dt_source})
      if(NOT DEFINED secondary_slot_flash)
        message(STATUS
          "slot${secondary_slot}_partition not found. Skip reading erase/write block sizes.")
        math(EXPR image "${image} + 1")
        continue()
      endif()

      dt_get_erase_write_block_sizes(${secondary_slot_flash} erase_size_secondary_slot
        write_size_secondary_slot "${dt_source}")

      dt_prop(secondary_slot_size PATH "${secondary_slot_flash}" PROPERTY "reg" INDEX 1 TARGET
        ${dt_source})
      if(DEFINED secondary_slot_size AND DEFINED erase_size_secondary_slot)
        math(EXPR remainder "${secondary_slot_size} % ${erase_size_secondary_slot}")
        if(NOT remainder EQUAL 0)
          message(FATAL_ERROR "slot${secondary_slot}_partition size (${secondary_slot_size}) is "
            "not divisible by erase block size (${erase_size_secondary_slot})"
          )
        endif()
        math(EXPR slot_min_sectors "${secondary_slot_size} / ${erase_size_secondary_slot}")

        if(${slot_min_sectors} GREATER ${auto_min_sectors})
          set(auto_min_sectors ${slot_min_sectors})
        endif()
      else()
        message(WARNING
          "Unable to determine erase size/total size of slot${secondary_slot} partition")
      endif()
    endif()

    math(EXPR image "${image} + 1")
  endwhile()

  set(${var} ${auto_min_sectors} PARENT_SCOPE)
endfunction()

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

  # Read compatible property to adjust the offset and address caluculation for subpartitions.
  dt_prop(partition_compat PATH "${arg_DT_PARTITION_PATH}" TARGET "${arg_DT_PARTITION_TARGET}"
    PROPERTY compatible)

  # The partition parent should be either a fixed-partitions or a fixed-subpartitions node.
  set(dt_partition_parent "${arg_DT_PARTITION_PATH}")
  dt_get_parent(dt_partition_parent)

  if("${partition_compat}" STREQUAL "zephyr,mapped-partition")
    dt_prop(parent_compat PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}"
      PROPERTY compatible)
    # Go up in the tree until the NVM memory controller is reached.
    while("${parent_compat}" STREQUAL "zephyr,mapped-partition")
      dt_get_parent(dt_partition_parent)
      dt_prop(parent_compat PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}"
        PROPERTY compatible)
    endwhile()
  elseif("${dt_partition_parent}" IN_LIST fixed_subpartitions)
    # Get the parent of the subpartition node, which should be a fixed-partitions node.
    dt_get_parent(dt_partition_parent)
  elseif(NOT "${dt_partition_parent}" IN_LIST fixed_partitions)
    message(FATAL_ERROR "dt_partition_addr: Node is not a (mapped) partition or subpartition: "
                        "${arg_DT_PARTITION_PATH} in target: ${arg_DT_PARTITION_TARGET}")
  endif()

  if("${partition_compat}" STREQUAL "zephyr,mapped-partition")
    if(NOT arg_DT_PARTITION_ABSOLUTE)
      # Skip the "partitions" node.
      dt_get_parent(dt_partition_parent)
      # A grandparent of the upmost "mapped-partition" node should be the memory controller.
      dt_reg_addr(mem_ctrl_addr PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}")
      # Subtract the memory controller base address to get a relative address.
      math(EXPR dt_partition_offset "${dt_partition_offset} - ${mem_ctrl_addr}" OUTPUT_FORMAT
        HEXADECIMAL)
    endif()
  elseif(arg_DT_PARTITION_ABSOLUTE)
    # A parent of the "fixed-partitions" should be the memory controller.
    dt_get_parent(dt_partition_parent)
    # Add the memory controller base address to get an absolute address.
    dt_reg_addr(parent_addr PATH "${dt_partition_parent}" TARGET "${arg_DT_PARTITION_TARGET}")
    math(EXPR dt_partition_offset "${dt_partition_offset} + ${parent_addr}" OUTPUT_FORMAT
      HEXADECIMAL)
  endif()

  set(${var} "${dt_partition_offset}" PARENT_SCOPE)
endfunction()

# Usage:
#   dt_partition_size(<var> [LABEL <label>|PATH <path>] [TARGET <target>])
#
# Get the partition size, based on the path or label.
# The value will be returned in the <var> parameter.
#
# The <path> value may be any of these:
#
# - absolute path to a node, like '/foo/bar'
# - a node alias, like 'my-alias'
# - a node alias followed by a path to a child node, like 'my-alias/child-node'
#
# Results can be:
# - The size in bytes.
# - <var>-NOTFOUND if node does not exists or does not have a register block at the requested index
#   or with the requested name
#
# <var>          : Return variable where the size value will be stored
# LABEL <label>  : Node label
# PATH <path>    : Node path
# TARGET <target>: Optional target to retrieve devicetree information from
# REQUIRED       : Generate a fatal error if the node is not found
function(dt_partition_size var)
  cmake_parse_arguments(arg_DT_PARTITION "REQUIRED" "LABEL;PATH;TARGET" "" ${ARGN})

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

  # Read the partition size
  dt_prop(dt_partition_size PATH "${arg_DT_PARTITION_PATH}" PROPERTY "reg" INDEX 1 TARGET
    "${arg_DT_PARTITION_TARGET}")

  set(${var} "${dt_partition_size}" PARENT_SCOPE)
endfunction()

function(get_address_from_dt_partition_nodelabel label address)
  dt_partition_addr(${address} LABEL "${label}" TARGET "${DEFAULT_IMAGE}" ABSOLUTE REQUIRED)
  set(${address} "${${address}}" PARENT_SCOPE)
endfunction()
