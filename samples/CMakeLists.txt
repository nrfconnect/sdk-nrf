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

  if (CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT)
      add_overlay_config(
        spm
        ${NRF_DIR}/samples/spm/spm_ns_handler_from_spm_fault.conf
        )

      add_overlay_dts(
        mcuboot
        ${NRF_DIR}/samples/spm/spm_uart.overlay
      )
  endif()

  if (CONFIG_BOARD_ENABLE_CPUNET)
      add_overlay_config(
        spm
        ${NRF_DIR}/samples/spm/spm_enable_cpunet.conf
        )
  endif()

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

if (CONFIG_SECURE_BOOT)
  if (CONFIG_SOC_NRF5340_CPUNET)
    # Share some information which is used when generating the zip file
    # with the update binaries.
    set_shared(IMAGE net_core PROPERTY SOC ${CONFIG_SOC})
    set_shared(IMAGE net_core PROPERTY VERSION ${CONFIG_FW_INFO_FIRMWARE_VERSION})
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
  if (CONFIG_BUILD_S1_VARIANT AND "${CONFIG_S1_VARIANT_IMAGE_NAME}" STREQUAL "app")
    add_child_image(
      NAME s1_image
      PRELOAD_IMAGE app
      )
  endif()
endif()

if (CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE)
  add_child_image(
    NAME "empty_app_core"
    SOURCE_DIR "${NRF_DIR}/samples/nrf5340/empty_app_core"
    DOMAIN "CPUAPP"
    BOARD ${CONFIG_DOMAIN_CPUAPP_BOARD}
    )
endif()

if (CONFIG_NCS_INCLUDE_RPMSG_CHILD_IMAGE)

  if (CONFIG_NCS_SAMPLE_MULTIPROTOCOL_RPMSG_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "multiprotocol_rpmsg")
    set(CHILD_IMAGE_PATH "${NRF_DIR}/samples/nrf5340/multiprotocol_rpmsg")

  elseif (CONFIG_NCS_SAMPLE_HCI_RPMSG_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "hci_rpmsg")
    set(CHILD_IMAGE_PATH "${ZEPHYR_BASE}/samples/bluetooth/hci_rpmsg")

  elseif (CONFIG_NCS_SAMPLE_802154_RPMSG_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "802154_rpmsg")
    set(CHILD_IMAGE_PATH "${ZEPHYR_BASE}/samples/boards/nrf/ieee802154/802154_rpmsg")

  elseif (CONFIG_NCS_SAMPLE_RPC_HOST_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "rpc_host")
    set(CHILD_IMAGE_PATH "${NRF_DIR}/samples/bluetooth/rpc_host")

  else()
    # If we get here, something is wrong with the Kconfig resolution
    message(WARNING "CONFIG_NCS_INCLUDE_RPMSG_CHILD_IMAGE is set but "
      "couldn't resolve which RPMsg sample to add.")
  endif()

  # Only include a child image if both image name and path have been defined.
  # Prevents e.g. menuconfig from becoming inaccessible, among others.
  if(CHILD_IMAGE_NAME AND CHILD_IMAGE_PATH)

    if (CONFIG_NRF53_UPGRADE_NETWORK_CORE)
      # Inject this configuration from parent image to enable
      # secure bootloader on the network core. This enables firmware update
      # of the network core application.
      add_overlay_config(
        ${CHILD_IMAGE_NAME}
        "${NRF_DIR}/subsys/bootloader/image/secure_boot.conf"
      )

      # Inject this configuration from parent image mcuboot to enable
      # the PCD subsystem which is used to communicate the firmware update
      # to the network core bootloader.
      add_overlay_config(
        mcuboot
        "${NRF_DIR}/subsys/pcd/pcd.conf"
      )
    endif()

    add_child_image(
      NAME ${CHILD_IMAGE_NAME}
      SOURCE_DIR ${CHILD_IMAGE_PATH}
      DOMAIN "CPUNET"
      BOARD ${CONFIG_DOMAIN_CPUNET_BOARD}
      )
  endif()

endif()
