/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <bluetooth/adv_prov.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_le_adv_prov, CONFIG_BT_ADV_PROV_LOG_LEVEL);


static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(state);
	ARG_UNUSED(fb);

	static uint8_t tx_power;

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
		LOG_ERR("Read Tx power err: %d", err);
	} else {
		rp = (struct bt_hci_rp_vs_read_tx_power_level *)rsp->data;

		int readout = rp->tx_power_level;

		readout += CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL;
		__ASSERT_NO_MSG((readout >= INT8_MIN) && (readout <= INT8_MAX));
		tx_power = (uint8_t)readout;

		net_buf_unref(rsp);

		ad->type = BT_DATA_TX_POWER;
		ad->data_len = sizeof(tx_power);
		ad->data = &tx_power;
	}

	return err;
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(tx_power, get_data);
