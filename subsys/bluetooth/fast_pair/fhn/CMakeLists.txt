#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fhn)

if(CONFIG_BT_FAST_PAIR_FMDN)
  target_compile_options(fhn PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/include_priv/fp_fhn_kconfig_compat.h)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_AUTH OR CONFIG_BT_FAST_PAIR_FMDN_AUTH)
  target_sources(fhn PRIVATE auth.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_BATTERY OR CONFIG_BT_FAST_PAIR_FMDN_BATTERY)
  target_sources(fhn PRIVATE battery.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_BEACON_ACTIONS OR CONFIG_BT_FAST_PAIR_FMDN_BEACON_ACTIONS)
  target_sources(fhn PRIVATE beacon_actions.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_CALLBACKS OR CONFIG_BT_FAST_PAIR_FMDN_CALLBACKS)
  target_sources(fhn PRIVATE callbacks.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_CLOCK OR CONFIG_BT_FAST_PAIR_FMDN_CLOCK)
  target_sources(fhn PRIVATE clock.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_DULT_INTEGRATION OR CONFIG_BT_FAST_PAIR_FMDN_DULT_INTEGRATION)
  target_sources(fhn PRIVATE dult_integration.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_DULT_MOTION_DETECTOR OR CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR)
  target_sources(fhn PRIVATE motion_detector.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_READ_MODE OR CONFIG_BT_FAST_PAIR_FMDN_READ_MODE)
  target_sources(fhn PRIVATE read_mode.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_RING OR CONFIG_BT_FAST_PAIR_FMDN_RING)
  target_sources(fhn PRIVATE ring.c)
endif()

if(CONFIG_BT_FAST_PAIR_FHN_STATE OR CONFIG_BT_FAST_PAIR_FMDN_STATE)
  target_sources(fhn PRIVATE state.c)
endif()

target_include_directories(fhn PUBLIC include)
target_include_directories(fhn PRIVATE include_priv)
target_include_directories(fhn PRIVATE ../include/common)
target_link_libraries(fhn PRIVATE fp_crypto)
target_link_libraries(fhn PRIVATE fp_storage)

add_subdirectory(pf)
