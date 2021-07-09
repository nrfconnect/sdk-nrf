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

if (CONFIG_NCS_ADD_CHILD_IMAGE_TO_DOMAIN)
  if (CONFIG_NRF53_UPGRADE_NETWORK_CORE)
    # Inject this configuration from parent image to enable
    # secure bootloader on the network core. This enables firmware update
    # of the network core application.
    add_overlay_config(
      ${CONFIG_NCS_CHILD_IMAGE_NAME}
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

  if (CONFIG_NCS_CHILD_IMAGE_OVERLAY)
    add_overlay_config(
      ${CONFIG_NCS_CHILD_IMAGE_NAME}
      ${CONFIG_NCS_CHILD_IMAGE_OVERLAY}
    )
  endif()

  add_child_image(
    NAME ${CONFIG_NCS_CHILD_IMAGE_NAME}
    SOURCE_DIR ${CONFIG_NCS_CHILD_IMAGE_PATH}
    DOMAIN ${CONFIG_NCS_DOMAIN_TO_ADD_CHILD_IMAGE_TO}
    BOARD ${CONFIG_DOMAIN_${CONFIG_NCS_DOMAIN_TO_ADD_CHILD_IMAGE_TO}_BOARD}
    )
endif()
