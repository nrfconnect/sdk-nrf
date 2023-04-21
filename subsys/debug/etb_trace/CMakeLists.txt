#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(etb_trace.c)
zephyr_sources_ifdef(CONFIG_ETB_TRACE_PM etb_trace_pm.c)
