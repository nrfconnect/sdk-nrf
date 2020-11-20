#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_BT_GATT_POOL gatt_pool.c)
zephyr_sources_ifdef(CONFIG_BT_GATT_DM gatt_dm.c)
zephyr_sources_ifdef(CONFIG_BT_SCAN scan.c)
zephyr_sources_ifdef(CONFIG_BT_CONN_CTX conn_ctx.c)
zephyr_sources_ifdef(CONFIG_BT_ENOCEAN enocean)

add_subdirectory_ifdef(CONFIG_BT_LL_SOFTDEVICE controller)
add_subdirectory_ifdef(CONFIG_BT_MESH_NRF_MODELS mesh)

add_subdirectory_ifdef(CONFIG_BT_NRF_SERVICES services)

if(CONFIG_BT_EATT OR
   CONFIG_BT_L2CAP_ECRED OR
   CONFIG_BT_PER_ADV OR
   CONFIG_BT_AUDIO)
  message(WARNING "
  One or more of the options have been enabled:
  CONFIG_BT_EATT, CONFIG_BT_L2CAP_ECRED, CONFIG_BT_PER_ADV, CONFIG_BT_AUDIO
  These bluetooth features have not been qualified, supported for development only."
  )
endif()

if(CONFIG_BT_LL_SW_SPLIT)
  message(WARNING "
  Software-based BLE Link Layer (BT_LL_SW_SPLIT) option has been enabled.
  This feature has not been qualified, supported for development only."
  )
endif()

if(CONFIG_BT_MESH)
  message(WARNING "
  Bluetooth Mesh support (BT_MESH) option has been enabled.
  This feature has not been qualified, supported for development only."
  )
endif()
