#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# If a device support network core as a separate image, include it manually,
# so the sysbuild logic that counts the number of updatable binaries is not
# affected.
if(SB_CONFIG_SUPPORT_NETCORE)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)
  set(board_target_netcore "${BOARD}/${target_soc}/${SB_CONFIG_NETCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}")
  set(target_soc)
  set(target_cpucluster)
  set(netcore_image_path ${ZEPHYR_NRF_MODULE_DIR}/applications/ipc_radio)
  set(radio_image_name ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME}_ipc_radio)

  ExternalZephyrProject_Add(
    APPLICATION ${radio_image_name}
    SOURCE_DIR ${netcore_image_path}
    BOARD ${board_target_netcore}
    BOARD_REVISION ${BOARD_REVISION}
    APP_TYPE FIRMWARE_LOADER
  )
  add_overlay_config(
    ${radio_image_name}
    ${netcore_image_path}/overlay-bt_hci_ipc.conf
  )
endif()
