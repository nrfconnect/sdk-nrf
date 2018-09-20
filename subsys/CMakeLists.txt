#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_NRF_BT bluetooth)

add_subdirectory_ifdef(CONFIG_NRF_ESB enhanced_shockburst)

add_subdirectory_if_kconfig(event_manager)

add_subdirectory_ifdef(CONFIG_PROFILER profiler)
