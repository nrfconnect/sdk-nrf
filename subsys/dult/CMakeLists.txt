#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(dult)

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_DULT_BATTERY battery.c)
zephyr_library_sources_ifdef(CONFIG_DULT_BT_ANOS bt/anos.c)
zephyr_library_sources_ifdef(CONFIG_DULT_ID id.c)
zephyr_library_sources_ifdef(CONFIG_DULT_NEAR_OWNER_STATE near_owner_state.c)
zephyr_library_sources_ifdef(CONFIG_DULT_SOUND sound.c)
zephyr_library_sources_ifdef(CONFIG_DULT_USER user.c)
