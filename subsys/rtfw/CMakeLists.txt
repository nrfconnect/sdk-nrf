#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(rtfw.c)
zephyr_library_sources_ifdef(CONFIG_RTFW_NRF_EGU rtfw_egu.c)
zephyr_library_sources_ifdef(CONFIG_RTFW_TEST rtfw_doorbell_test.c)
