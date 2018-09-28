#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_NRF_BT OR
	CONFIG_BT_LL_NRFXLIB)
	add_subdirectory(bluetooth)
endif()

add_subdirectory_ifdef(CONFIG_NRF_ESB enhanced_shockburst)

add_subdirectory_if_kconfig(event_manager)

add_subdirectory_ifdef(CONFIG_PROFILER profiler)
