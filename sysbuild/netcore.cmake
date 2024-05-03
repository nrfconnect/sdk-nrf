#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# Include network core image if enabled
if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_NONE AND DEFINED SB_CONFIG_NETCORE_IMAGE_NAME)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  if(DEFINED BOARD_REVISION)
    set(board_target_netcore "${BOARD}@${BOARD_REVISION}/${target_soc}/${SB_CONFIG_NETCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}")
  else()
    set(board_target_netcore "${BOARD}/${target_soc}/${SB_CONFIG_NETCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}")
  endif()

  set(target_soc)
  set(target_cpucluster)

  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_NETCORE_IMAGE_NAME}
    SOURCE_DIR ${SB_CONFIG_NETCORE_IMAGE_PATH}
    BOARD ${board_target_netcore}
  )

  if(NOT "${SB_CONFIG_NETCORE_IMAGE_DOMAIN}" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS ${SB_CONFIG_NETCORE_IMAGE_DOMAIN})
  endif()

  set_property(GLOBAL APPEND PROPERTY
               PM_${SB_CONFIG_NETCORE_IMAGE_DOMAIN}_IMAGES
               "${SB_CONFIG_NETCORE_IMAGE_NAME}"
  )

  set_property(GLOBAL PROPERTY DOMAIN_APP_${SB_CONFIG_NETCORE_IMAGE_DOMAIN}
               "${SB_CONFIG_NETCORE_IMAGE_NAME}"
  )

  set(${SB_CONFIG_NETCORE_IMAGE_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
      ${SB_CONFIG_NETCORE_IMAGE_NAME} CACHE INTERNAL ""
  )

  # Include ipc_radio overlays if ipc_radio is enabled.
  if(SB_CONFIG_NETCORE_IPC_RADIO)
    if(SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC AND SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC)
      message(FATAL_ERROR "HCI IPC can't be used together with BT RPC as ipc_radio configuration.")
    endif()

    if(SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_IMAGE_NAME}
        ${SB_CONFIG_NETCORE_IMAGE_PATH}/overlay-bt_rpc.conf
      )
    endif()

    if(SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_IMAGE_NAME}
        ${SB_CONFIG_NETCORE_IMAGE_PATH}/overlay-bt_hci_ipc.conf
      )
    endif()

    if(SB_CONFIG_NETCORE_IPC_RADIO_IEEE802154)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_IMAGE_NAME}
        ${SB_CONFIG_NETCORE_IMAGE_PATH}/overlay-802154.conf
      )
    endif()
  endif()

  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
