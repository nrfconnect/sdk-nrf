#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_ZTEST AND
    CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPP AND
    CONFIG_BOARD_ENABLE_CPUNET)
  # Add overlay for enabling B0N
  add_overlay_config(
    hello_world
    "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
    )

  # Add a child image in the network core.
  # Since we have enabled MCUBoot in the app core, the B0N bootloader is
  # automatically added. This can not be done from the test CMakeLists.txt
  # since it will not be considered when running partition_manager.cmake.
  add_child_image(
    NAME hello_world
    SOURCE_DIR ${ZEPHYR_BASE}/samples/hello_world
    DOMAIN CPUNET
    BOARD ${CONFIG_DOMAIN_CPUNET_BOARD})

endif()


add_subdirectory_ifdef(CONFIG_UNITY	unity)
