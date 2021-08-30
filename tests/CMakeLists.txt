#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NCS_TEST_INCLUDE_HELLO_WORLD_CHILD_IMAGE)
  if (CONFIG_NCS_TEST_INCLUDE_HELLO_WORLD_B0N)
    # Add overlay for enabling B0N
    add_overlay_config(
      hello_world
      "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
      )
  endif()

  add_child_image(
    NAME hello_world
    SOURCE_DIR ${ZEPHYR_BASE}/samples/hello_world
    DOMAIN CPUNET
    BOARD ${CONFIG_DOMAIN_CPUNET_BOARD})

endif()

add_subdirectory_ifdef(CONFIG_UNITY	unity)
