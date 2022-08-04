/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_sample, LOG_LEVEL_INF);

#include "bt_tx_power_adv.h"


size_t bt_tx_power_adv_data_size(void)
{
	uint8_t tx_power;

	return sizeof(tx_power);
}

int bt_tx_power_adv_data_fill(struct bt_data *adv_data, uint8_t *buf, size_t buf_size)
{
	if (buf_size < bt_tx_power_adv_data_size()) {
		return -EINVAL;
	}

	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct bt_hci_rp_vs_read_tx_power_level *rp;
	struct net_buf *rsp = NULL;
	struct net_buf *cmd_buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_TX_POWER_LEVEL, sizeof(*cp));

	if (!cmd_buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(cmd_buf, sizeof(*cp));
	cp->handle_type = BT_HCI_VS_LL_HANDLE_TYPE_ADV;
	cp->handle = sys_cpu_to_le16(0);
	int err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL, cmd_buf, &rsp);

	if (err) {
		uint8_t reason = rsp ?
				((struct bt_hci_rp_vs_read_tx_power_level *) rsp->data)->status : 0;
		LOG_ERR("Read Tx power err: %d reason 0x%02x", err, reason);
	} else {
		rp = (void *)rsp->data;
		buf[0] = rp->tx_power_level;
		net_buf_unref(rsp);

		adv_data->type = BT_DATA_TX_POWER;
		adv_data->data_len = bt_tx_power_adv_data_size();
		adv_data->data = buf;
	}

	return err;
}
