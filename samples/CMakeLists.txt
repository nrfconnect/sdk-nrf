#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_SPM)
  get_cmake_property(VARIABLES              VARIABLES)
  get_cmake_property(VARIABLES_CACHED CACHE_VARIABLES)

  set(regex "^CONFIG_SPM_SERVICE.+")

  list(FILTER VARIABLES        INCLUDE REGEX ${regex})
  list(FILTER VARIABLES_CACHED INCLUDE REGEX ${regex})

  foreach(var_name ${VARIABLES} ${VARIABLES_CACHED})
    set(spm_${var_name} ${${var_name}})
  endforeach()

  set(spm_CONFIG_SPM_SECURE_SERVICES ${CONFIG_SPM_SECURE_SERVICES})

  add_child_image(
    NAME spm
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/spm
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

if (CONFIG_BUILD_WITH_TFM AND CONFIG_UART_1_NRF_UARTE)
  message(FATAL_ERROR "
    Non-secure applications built with TF-M cannot use UART1, because this
    instance is used by the TF-M secure application. Disable the uart1 node
    in devicetree. See the following file:
    nrf/samples/tfm/tfm_hello_world/boards/nrf9160dk_nrf9160ns.overlay
    for an example how to do it.")
endif()

if (CONFIG_SECURE_BOOT)
  if (CONFIG_SOC_NRF5340_CPUNET)
    add_child_image(
      NAME b0n
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/nrf5340/netboot
      )
  else()
    add_child_image(
      NAME b0
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bootloader
      )
  endif()
  if (CONFIG_SOC_NRF5340_CPUAPP AND CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(
        mcuboot
        ${NRF_DIR}/subsys/bootloader/image/log_minimal.conf
        )
  endif()
endif()

# Special handling for the nRF53 when using RPMSG HCI.
# Automatically include the multiprotocol_rpmsg as child image when both RPMSG HCI
# and RPMSG 802.15.4 Serialization are enabled or the hci_rpmsg sample when
# only RPMSG HCI is enabled. For any of these samples change the board to be the network core.
if (CONFIG_BT_RPMSG_NRF53)
  if (CONFIG_SOC_NRF5340_CPUAPP)

    if (CONFIG_NRF_802154_SER_HOST)
      set(NETCORE_IMAGE "multiprotocol_rpmsg")
      set(NETCORE_IMAGE_PATH "${NRF_DIR}/samples/nrf5340/${NETCORE_IMAGE}")
      message("Adding 'multiprotocol_rpmsg' firmware as child image since "
      "CONFIG_BT_RPMSG_NRF53 and CONFIG_NRF_802154_SER_HOST are set to 'y'")
    else()
      set(NETCORE_IMAGE "hci_rpmsg")
      set(NETCORE_IMAGE_PATH "${ZEPHYR_BASE}/samples/bluetooth/${NETCORE_IMAGE}")
      message("Adding 'hci_rpmsg' firmware as child image since "
      "CONFIG_BT_RPMSG_NRF53 is set to 'y'")
    endif()

    if (CONFIG_BT_LL_SOFTDEVICE_DEFAULT)
      add_overlay_config(
        ${NETCORE_IMAGE}
        ${NRF_DIR}/subsys/bluetooth/controller/bt_ll_softdevice_hci_rpmsg.conf
        )
    endif()
    if (CONFIG_BOOTLOADER_MCUBOOT AND CONFIG_NRF53_UPGRADE_NETWORK_CORE)
      # Inject this configuration from parent image hci_rpmsg to enable
      # secure bootloader on the network core. This enables firmware update
      # of the network core application.
      add_overlay_config(
        ${NETCORE_IMAGE}
        "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
        )

      # Inject this configuration from parent image mcuboot to enable
      # the PCD subsystem which is used to communicate the firmware update
      # to the network core bootloader.
      add_overlay_config(
        mcuboot
        "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf"
        )
    endif()

    add_child_image(
      NAME ${NETCORE_IMAGE}
      SOURCE_DIR ${NETCORE_IMAGE_PATH}
      DOMAIN CPUNET
      BOARD ${CONFIG_DOMAIN_CPUNET_BOARD})
  else()
    message(FATAL_ERROR
      "SoC not supported for including hci_rpmsg as child image")
  endif()
elseif (CONFIG_NRF_802154_SER_HOST)
  if (CONFIG_SOC_NRF5340_CPUAPP)
    message("Adding '802154_rpmsg' firmware as child image since "
    "CONFIG_NRF_802154_SER_HOST is set to y")

    add_child_image(
      NAME 802154_rpmsg
      SOURCE_DIR ${ZEPHYR_BASE}/samples/boards/nrf/ieee802154/802154_rpmsg
      DOMAIN CPUNET
      BOARD ${CONFIG_DOMAIN_CPUNET_BOARD}
    )
  else()
    message(FATAL_ERROR
      "Board ${BOARD} not supported for including '802154_rpmsg' as child image")
  endif()
endif()

if (CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE)
  message("Adding 'empty_app_core' firmware as child image since "
    "CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE is set to 'y'")

  add_child_image(
    NAME empty_app_core
    SOURCE_DIR ${NRF_DIR}/samples/nrf5340/empty_app_core
    DOMAIN CPUAPP
    BOARD ${CONFIG_DOMAIN_CPUAPP_BOARD})
endif()
