#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(
    volume
)

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/bt_rendering_and_capture.c)

if (CONFIG_BT_VCP_VOL_CTLR AND CONFIG_BT_VCP_VOL_REND)
	message(FATAL_ERROR "No support for vol controller and renderer on same device")
endif()

if (CONFIG_BT_VCP_VOL_REND)
target_sources(app PRIVATE
	        ${CMAKE_CURRENT_SOURCE_DIR}/volume/bt_vol_rend.c)
endif()

if (CONFIG_BT_VCP_VOL_CTLR)
	target_sources(app PRIVATE
	        ${CMAKE_CURRENT_SOURCE_DIR}/volume/bt_vol_ctlr.c)
endif()
