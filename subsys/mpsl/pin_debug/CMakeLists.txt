#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MPSL_PIN_DEBUG_APP_CORE mpsl_pin_debug_app_core.c)
zephyr_library_sources_ifdef(CONFIG_MPSL_PIN_DEBUG_RADIO_CORE mpsl_pin_debug_radio_core.c)
