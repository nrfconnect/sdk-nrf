#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_MPSL_CX_THREAD OR CONFIG_MPSL_CX_BT_1WIRE OR CONFIG_MPSL_CX_BT_3WIRE OR CONFIG_MPSL_CX_GENERIC_3PIN)
    zephyr_library()

    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_THREAD thread/mpsl_cx_thread.c)
    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_BT_1WIRE bluetooth/mpsl_cx_1w_bluetooth.c)
    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_BT_3WIRE bluetooth/mpsl_cx_3w_bluetooth.c)
    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_GENERIC_3PIN generic_3pin/mpsl_cx_generic_3pin.c)
endif()
