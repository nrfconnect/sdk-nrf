#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_FMDN_PF)
  target_sources(fmdn PRIVATE oob_msg.c)
  target_sources(fmdn PRIVATE oob_msg_ble_cs.c)
  target_sources(fmdn PRIVATE pf.c)
endif()

target_include_directories(fmdn PRIVATE include)
