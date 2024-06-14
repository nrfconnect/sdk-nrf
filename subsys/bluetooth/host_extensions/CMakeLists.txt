#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(host_extensions.c)
zephyr_sources_ifdef(CONFIG_BT_CONN_EVT_CB conn_evt_cb.c)
