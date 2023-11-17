/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Purpose of this file is to add nordic-LL only vendor-specific commands.
 * Adding them in nrf is better maintainable.
 */

#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <../subsys/bluetooth/host/conn_internal.h>

#include "hci_types_host_extensions.h"
#include <bluetooth/nrf/host_extensions.h>

/* Write Remote Transmit Power Level HCI command */
int bt_conn_set_remote_tx_power_level(struct bt_conn *conn,
				       enum bt_conn_le_tx_power_phy phy, int8_t delta)
{
	struct bt_hci_set_remote_tx_power_level *cp;
	struct net_buf *buf;

	if (!phy) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_REMOTE_TX_POWER, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->phy = phy;
	cp->delta = delta;

	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_REMOTE_TX_POWER, buf, NULL);
}

/* Set LE Power Control Feature Parameters */
int bt_conn_set_power_control_request_params(struct bt_conn_set_pcr_params *params)
{
	struct bt_hci_cp_set_power_control_request_param *cp;
	struct net_buf *buf;

	if (!params->wait_period_ms) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_POWER_CONTROL_REQUEST_PARAMS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->auto_enable = params->auto_enable;
	cp->apr_enable = params->apr_enable;
	cp->beta = sys_cpu_to_le16(params->beta);
	cp->lower_limit = params->lower_limit;
	cp->upper_limit = params->upper_limit;
	cp->lower_target_rssi = params->lower_target_rssi;
	cp->upper_target_rssi = params->upper_target_rssi;
	cp->wait_period_ms = sys_cpu_to_le16(params->wait_period_ms);
	cp->apr_margin = params->apr_margin;

	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_POWER_CONTROL_REQUEST_PARAMS, buf, NULL);
}
#endif /* CONFIG_BT_TRANSMIT_POWER_CONTROL */
