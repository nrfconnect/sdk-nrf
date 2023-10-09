#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NCS_REMOTE_RADIO_CHILD_IMAGE)
  set(REMOTE_RADIO_PATH "${CMAKE_CURRENT_LIST_DIR}/remote_radio")

  if(CONFIG_BT_RPC_CLIENT)
    add_overlay_config(
      "remote_radio"
      "${REMOTE_RADIO_PATH}/overlay-bt_rpc_host.conf"
    )
  endif()

  if(CONFIG_BT_RPMSG)
    add_overlay_config(
      "remote_radio"
      "${REMOTE_RADIO_PATH}/overlay-bt_rpmsg.conf"
    )
  endif()

  if(CONFIG_NRF_802154_SER_HOST)
    add_overlay_config(
      "remote_radio"
      "${REMOTE_RADIO_PATH}/overlay-802154.conf"
    )
  endif()

  add_child_image(
    NAME "remote_radio"
    SOURCE_DIR ${REMOTE_RADIO_PATH}
    DOMAIN "CPUNET"
    BOARD ${CONFIG_DOMAIN_CPUNET_BOARD}
    )
endif()
