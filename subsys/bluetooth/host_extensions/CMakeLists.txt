#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(host_extensions.c)
zephyr_sources_ifdef(CONFIG_BT_RADIO_NOTIFICATION_CONN_CB radio_notification_conn_cb.c)
