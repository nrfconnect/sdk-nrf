#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

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
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/log_minimal.conf
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
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/empty_app_core"
    DOMAIN "CPUAPP"
    BOARD ${CONFIG_DOMAIN_CPUAPP_BOARD}
    )
endif()

if (CONFIG_NCS_SAMPLE_PROVISIONING_NET_CORE_CHILD_IMAGE)
  add_child_image(
    NAME "provisioning_net_core"
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/tfm/provisioning_image_net_core"
    DOMAIN "CPUNET"
    BOARD ${CONFIG_DOMAIN_CPUNET_BOARD}
    )
endif()

if (CONFIG_NCS_SAMPLE_REMOTE_SHELL_CHILD_IMAGE)
  add_child_image(
    NAME "remote_shell"
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/remote_shell"
    DOMAIN "CPUAPP"
    BOARD ${CONFIG_DOMAIN_CPUAPP_BOARD}
    )
endif()

if (CONFIG_NCS_SAMPLE_DTM_REMOTE_HCI_CHILD_IMAGE)
  add_child_image(
    NAME "remote_hci"
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/direct_test_mode/remote_hci"
    DOMAIN "CPUAPP"
    BOARD ${CONFIG_DOMAIN_CPUAPP_BOARD}
    )
endif()

if (CONFIG_NCS_INCLUDE_RPMSG_CHILD_IMAGE OR CONFIG_NCS_SAMPLE_EMPTY_NET_CORE_CHILD_IMAGE)

  if (CONFIG_NCS_SAMPLE_EMPTY_NET_CORE_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "empty_net_core")
    set(CHILD_IMAGE_PATH "${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/empty_net_core")

  elseif (CONFIG_NCS_SAMPLE_MULTIPROTOCOL_RPMSG_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "multiprotocol_rpmsg")
    set(CHILD_IMAGE_PATH "${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/multiprotocol_rpmsg")

  elseif (CONFIG_NCS_SAMPLE_HCI_IPC_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "hci_ipc")
    set(CHILD_IMAGE_PATH "${ZEPHYR_BASE}/samples/bluetooth/hci_ipc")

  elseif (CONFIG_NCS_SAMPLE_802154_RPMSG_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "802154_rpmsg")
    set(CHILD_IMAGE_PATH "${ZEPHYR_BASE}/samples/boards/nordic/ieee802154/802154_rpmsg")

  elseif (CONFIG_NCS_SAMPLE_RPC_HOST_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "rpc_host")
    set(CHILD_IMAGE_PATH "${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/rpc_host")

  elseif (CONFIG_NCS_SAMPLE_PERIPHERAL_RADIO_TEST_CHILD_IMAGE)
    set(CHILD_IMAGE_NAME "peripheral_radio_test")
    set(CHILD_IMAGE_PATH "${ZEPHYR_NRF_MODULE_DIR}/samples/peripheral/radio_test")

  else()
    # If we get here, something is wrong with the Kconfig resolution
    message(WARNING "CONFIG_NCS_INCLUDE_RPMSG_CHILD_IMAGE is set but "
      "couldn't resolve which RPMsg sample to add.")
  endif()

  if (CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP)
    # By passing this value we don't require the user to pass any overlay files to the
    # network core child image. Bluetooth buffer sizes are aligned automatically.
    if (CONFIG_NCS_SAMPLE_HCI_IPC_CHILD_IMAGE OR CONFIG_NCS_SAMPLE_MULTIPROTOCOL_RPMSG_CHILD_IMAGE)
      add_overlay_config(
        ${CHILD_IMAGE_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/common/mcumgr_bt_ota_dfu/rpmsg_child_image_overlay.conf
      )
    endif()
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

    if (CONFIG_BT_MESH)
      # Inject this configuration from the parent image to enable
      # the necessary amount of advertising sets in the Bluetooth controller
      # on the network core required by the Bluetooth Mesh.
      add_overlay_config(
        ${CHILD_IMAGE_NAME}
        "${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/hci_ipc_child_image_overlay.conf"
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

add_subdirectory(common)
