#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BOOT_IMAGE_ACCESS_HOOKS)
  if(CONFIG_BOOT_IMAGE_ACCESS_HOOK_NRF5340)
    zephyr_library()
    zephyr_library_sources(nrf53_hooks.c)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)

if(NOT CONFIG_PARTITION_MANAGER_ENABLED)
  # Read s0 address from b0n image
  if(NOT TARGET b0n_target)
    add_custom_target(b0n_target)
    zephyr_dt_import(EDT_PICKLE_FILE ${CMAKE_BINARY_DIR}/../b0n/zephyr/edt.pickle TARGET b0n_target)
  endif()

  dt_nodelabel(s0_partition_path NODELABEL s0_partition REQUIRED TARGET b0n_target)
  dt_reg_addr(s0_partition_address PATH ${s0_partition_path} TARGET b0n_target)
  dt_reg_size(s0_partition_size PATH ${s0_partition_path} TARGET b0n_target)
  zephyr_library_compile_options("-DPCD_NET_CORE_APP_ADDRESS=${s0_partition_address}")
  zephyr_library_compile_options("-DPCD_NET_CORE_APP_SIZE=${s0_partition_size}")
endif()
  endif()
endif()

if(CONFIG_FIND_NEXT_SLOT_HOOKS)
  if(CONFIG_FIND_NEXT_SLOT_HOOK_BOOT_REQ)
    zephyr_library()
    zephyr_library_sources(hooks_find_next_slot.c)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
  endif()
endif()
