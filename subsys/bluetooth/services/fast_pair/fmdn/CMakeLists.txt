#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fmdn)

if(CONFIG_BT_FAST_PAIR_FMDN_AUTH)
  target_sources(fmdn PRIVATE auth.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_BATTERY)
  target_sources(fmdn PRIVATE battery.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_BEACON_ACTIONS)
  target_sources(fmdn PRIVATE beacon_actions.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_CALLBACKS)
  target_sources(fmdn PRIVATE callbacks.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_CLOCK)
  target_sources(fmdn PRIVATE clock.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION)
  target_sources(fmdn PRIVATE dult_integration.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_READ_MODE)
  target_sources(fmdn PRIVATE read_mode.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_RING)
  target_sources(fmdn PRIVATE ring.c)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN_STATE)
  target_sources(fmdn PRIVATE state.c)
endif()

target_include_directories(fmdn PUBLIC include)
target_include_directories(fmdn PRIVATE include_priv)
target_include_directories(fmdn PRIVATE ../include/common)
target_link_libraries(fmdn PRIVATE fp_crypto)
target_link_libraries(fmdn PRIVATE fp_storage)
