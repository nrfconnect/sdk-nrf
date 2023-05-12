#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(
    volume
)

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/bt_rend.c)

if (CONFIG_BT_VCP_VOL_CTLR OR CONFIG_BT_VCP_VOL_REND)
target_sources(app PRIVATE
	        ${CMAKE_CURRENT_SOURCE_DIR}/volume/bt_rend_vol.c)
endif()
