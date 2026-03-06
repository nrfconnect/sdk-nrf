#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_FHN_PF)
  target_sources(fhn PRIVATE
    oob_msg.c
    oob_msg_ble_cs.c
    pf.c
  )
endif()

target_include_directories(fhn PRIVATE include)
