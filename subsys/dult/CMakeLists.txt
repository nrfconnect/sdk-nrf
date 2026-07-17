#
# Copyright (c) 2024-2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(dult)

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_DULT_BATTERY battery.c)
zephyr_library_sources_ifdef(CONFIG_DULT_BT_ADV bt/adv.c)
zephyr_library_sources_ifdef(CONFIG_DULT_BT_ANOS bt/anos.c)
zephyr_library_sources_ifdef(CONFIG_DULT_ID id.c)
zephyr_library_sources_ifdef(CONFIG_DULT_MOTION_DETECTOR motion_detector.c)
zephyr_library_sources_ifdef(CONFIG_DULT_NEAR_OWNER_STATE near_owner_state.c)
zephyr_library_sources_ifdef(CONFIG_DULT_SOUND sound.c)
zephyr_library_sources_ifdef(CONFIG_DULT_USER user.c)
zephyr_library_sources_ifdef(CONFIG_DULT_USER user_slot.c)

if(CONFIG_DULT_TEST)
  message(WARNING "
    --------------------------------------------------------------
    --- WARNING: DULT test mode (CONFIG_DULT_TEST) is enabled. ---
    --- This mode is not intended for production use.          ---
    --------------------------------------------------------------
    ")
endif()
