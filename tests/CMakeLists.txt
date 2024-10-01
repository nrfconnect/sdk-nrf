#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_ZTEST AND
    (CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR CONFIG_BOARD_THINGY53_NRF5340_CPUAPP) AND
    CONFIG_SOC_NRF53_CPUNET_ENABLE AND CONFIG_ZTEST_MULTICORE_DEFAULT_SETTINGS)
  # Add overlay for enabling B0N
  add_overlay_config(
    hello_world
    "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
    )

  if(NOT DEFINED CPUNET_PM_DOMAIN_DYNAMIC_PARTITION
     OR "${CPUNET_PM_DOMAIN_DYNAMIC_PARTITION}" STREQUAL hello_world
  )
    # Add a child image in the network core if there is not an image already.
    # Since we have enabled MCUBoot in the app core, the B0N bootloader is
    # automatically added. This can not be done from the test CMakeLists.txt
    # since it will not be considered when running partition_manager.cmake.
    add_child_image(
      NAME hello_world
      SOURCE_DIR ${ZEPHYR_BASE}/samples/hello_world
      DOMAIN CPUNET
      BOARD ${CONFIG_DOMAIN_CPUNET_BOARD})

  endif()
endif()

zephyr_include_directories(include)

add_subdirectory_ifdef(CONFIG_UNITY	unity)
add_subdirectory(mocks)
