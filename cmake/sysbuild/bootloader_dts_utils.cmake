# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file contains utility functions used for handling
# data coming from the devicetree for configurations using
# a bootloader.
# It contains helper functions used to get the addresses of
# specific bootloader patitions as well as
# functions that are used to verify the
# that devicetrees of the images are consistent with each other
# and with the current bootloader configuration.

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

#
# Verify that the code partition for a given image matches a specific node label.
#
function(code_partition_matches_label_verify label image)
  dt_nodelabel(label_node TARGET ${image} NODELABEL ${label})
  dt_chosen(code_partition_node TARGET ${image} PROPERTY "zephyr,code-partition")

  if (NOT "${code_partition_node}" STREQUAL "${label_node}")
    message(FATAL_ERROR "
      ERROR: zephyr,code-partition for image ${image}
      does not match label ${label}.
      This is required by the current bootloader configuration.
      zephyr,code-partition node:
      ${code_partition_node}
      ${label} node:
      ${label_node}
      \n"
    )
  endif()
endfunction()

#
# Verify that the required node labels for all images point to the same nodes
# as for the default image
#
function(labels_match_for_all_images_verify labels)
  foreach(label ${labels})
    dt_nodelabel(default_image_node TARGET ${DEFAULT_IMAGE} NODELABEL ${label} REQUIRED)
    foreach(image ${IMAGES})
      dt_nodelabel(node TARGET ${image} NODELABEL ${label} REQUIRED)
      if (NOT "${node}" STREQUAL "${default_image_node}")
        message(FATAL_ERROR "
          ERROR: Node pointed by ${label} for image ${image}
          does not match the node for the default image ${DEFAULT_IMAGE}.
          Node for default image ${DEFAULT_IMAGE}: ${default_image_node}
          Node for image ${image}: ${label_node}
          \n"
        )
      endif()
    endforeach()
  endforeach()
endfunction()

function(verify_bootloader_dts_configuration)
  set(required_labels_list)
  set(nordic_bootloader_vars_names)

  # Create a list of all devicetree labels that are required for
  # the current bootloader configuration.
  if(SB_CONFIG_SECURE_BOOT)
    list(APPEND required_labels_list "b0_partition" "provision_partition" "s0_partition")
  endif()
  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    list(APPEND required_labels_list "boot_partition")
    list(APPEND required_labels_list "slot0_partition")
    list(APPEND required_labels_list "slot1_partition")
  endif()
  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    list(APPEND required_labels_list "s1_partition")
  endif()

  # Check that the labels are present for all images and that they point to the same nodes
  # for each of the images.
  # This way we can treat a devicetree from any of the images as the source of truth for
  # the whole system when it comes to the required node labels.
  labels_match_for_all_images_verify("${required_labels_list}")

  # Verify that the zephyr,code-partition chosen node for each of the images
  # points to the expected node labels
  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    code_partition_matches_label_verify("slot0_partition" "${DEFAULT_IMAGE}")
  elseif(SB_CONFIG_SECURE_BOOT)
    code_partition_matches_label_verify("s0_partition" "${DEFAULT_IMAGE}")
  endif()

  if(SB_CONFIG_SECURE_BOOT)
    code_partition_matches_label_verify("b0_partition" "b0")
  endif()
  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    code_partition_matches_label_verify("s1_partition" "s1_image")
  endif()

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    code_partition_matches_label_verify("boot_partition" "mcuboot")
    if(SB_CONFIG_SECURE_BOOT)
      # In the B0 + MCUBOOT configuration s0_partition points to boot_partition
      code_partition_matches_label_verify("s0_partition" "mcuboot")
    endif()
  endif()
endfunction()

function(nsib_get_s0_address address)
  set(s0_label "s0_partition")
  get_address_from_dt_partition_nodelabel(${s0_label} ${address})
  set(${address} ${${address}} PARENT_SCOPE)
endfunction()

function(nsib_get_s1_address address)
  set(s1_label "s1_partition")
  get_address_from_dt_partition_nodelabel(${s1_label} ${address})
  set(${address} ${${address}} PARENT_SCOPE)
endfunction()

function(nsib_get_provision_address address)
  get_address_from_dt_partition_nodelabel("provision_partition" ${address})
  set(${address} ${${address}} PARENT_SCOPE)
endfunction()
