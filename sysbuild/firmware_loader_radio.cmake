# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Include network core image if enabled
if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NONE AND DEFINED
   SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME)
  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME}
    SOURCE_DIR ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_PATH}
    APP_TYPE FIRMWARE_LOADER
    BOARD ${SB_CONFIG_FIRMWARE_LOADER_RADIO_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  # Include ipc_radio overlays if ipc_radio is enabled.
  if(SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_IPC_RADIO)
    if(SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_IPC_RADIO_BT_HCI_IPC)
      add_overlay_config(
        ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME}
        ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_PATH}/overlay-bt_hci_ipc.conf
      )
    endif()

    # Check if the FW loader defines a custom overlay file for ipc_radio image
    if(EXISTS "${SB_CONFIG_FIRMWARE_LOADER_IMAGE_PATH}/sysbuild/ipc_radio.conf")
      add_overlay_config(
        ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME}
        ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_PATH}/sysbuild/ipc_radio.conf
      )
    endif()
  endif()
endif()
