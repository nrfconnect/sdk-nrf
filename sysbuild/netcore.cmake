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

  # Include ipc_radio FILE_SUFFIX if ipc_radio_FILE_SUFFIX is not specified and the application is not using custom configuration.
  if(SB_CONFIG_NETCORE_IPC_RADIO AND NOT SB_CONFIG_NETCORE_IPC_RADIO_CUSTOM_CONF)
    if(SB_CONFIG_BT_RPC AND IEEE802154)
      message(FATAL_ERROR "Configuration bt_rpc + IEEE802.15.4 is not supported.")
    endif()

    set(ipc_radio_suffix "")

    if(SB_CONFIG_BT AND NOT SB_CONFIG_BT_RPC)
      list(APPEND ipc_radio_suffix "bt_hci_ipc")
    elseif(SB_CONFIG_BT_RPC)
      list(APPEND ipc_radio_suffix "bt_rpc")
    endif()

    if(SB_CONFIG_IEEE802154)
      list(APPEND ipc_radio_suffix "802154")
    endif()

    if(SB_CONFIG_NETCORE_IPC_RADIO_DEBUG)
     list(APPEND ipc_radio_suffix "debug")
    endif()

    string(REPLACE ";" "_" ipc_radio_suffix "${ipc_radio_suffix}")
    set(ipc_radio_FILE_SUFFIX ${ipc_radio_suffix} CACHE INTERNAL "")
  endif()

  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
