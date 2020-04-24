#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SPM)
  add_child_image(
    NAME spm
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/nrf9160/spm
    )

  if (CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS AND
      NOT CONFIG_SPM_BUILD_STRATEGY_FROM_SOURCE AND
      NOT EXISTS ${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})
    message(WARNING "NOTE: SPM is not built from source, and the firmware use secure "
      "entry functions. However, the configured library file is not found.
      Ensure that the configuration 'ARM_ENTRY_VENEERS_LIB_NAME'
      points to the .a file (default 'spm/libspmsecureentries.a')
      generated alongside the flashed or merged SPM hex file")
  endif()
endif()

if (CONFIG_SECURE_BOOT)
  add_child_image(
    NAME b0
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bootloader
    )
endif ()

# Special handling for the nRF53 when using RPMSG HCI.
# Automatically include the hci_rpmsg sample as child image, and change
# the board to be the network core.
if (CONFIG_BT_RPMSG_NRF53)
  if ((BOARD STREQUAL nrf5340pdk_nrf5340_cpuappns) OR
      (BOARD STREQUAL nrf5340pdk_nrf5340_cpuapp))
    add_child_image(
      NAME hci_rpmsg
      SOURCE_DIR ${ZEPHYR_BASE}/samples/bluetooth/hci_rpmsg
      DOMAIN nrf5340pdk_nrf5340_cpunet)
  else()
    message(FATAL_ERROR
      "Board ${BOARD} not supported for including hci_rpmsg as child image")
  endif()
endif()
