#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_BT_FAST_PAIR_PROV_DATA AND NOT SB_CONFIG_PARTITION_MANAGER)
  include(image_flasher.cmake)

  add_image_flasher(NAME bt_fast_pair HEX_FILE
    "${CMAKE_BINARY_DIR}/modules/nrf/subsys/bluetooth/services/fast_pair/fp_provisioning_data.hex"
  )
endif()
