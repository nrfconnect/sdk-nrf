/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/l2cap_fixed.h>
#include "conn_internal.h"
#include "l2cap_internal.h"

static int l2cap_fixed_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan);

BT_L2CAP_CHANNEL_DEFINE(custom_fixed_chan, CONFIG_BT_L2CAP_FIXED_CID, l2cap_fixed_accept, NULL);

const static struct bt_l2cap_chan_ops *registered_ops;
static struct bt_l2cap_le_chan fixed_chans[CONFIG_BT_MAX_CONN];

static int l2cap_fixed_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	if (!registered_ops) {
		return -ENOMEM;
	}

	uint8_t conn_index = bt_conn_index(conn);

	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(fixed_chans));

	struct bt_l2cap_le_chan *fixed_chan = &fixed_chans[conn_index];

	__ASSERT_NO_MSG(!fixed_chan->chan.conn);
	memset(fixed_chan, 0, sizeof(*fixed_chan));

	fixed_chan->chan.ops = registered_ops;
	*chan = &fixed_chan->chan;

	return 0;
}

static void sent_callback(struct bt_conn *conn, void *user_data, int err)
{
	uint8_t conn_index = bt_conn_index(conn);

	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(fixed_chans));

	struct bt_l2cap_le_chan *fixed_chan = &fixed_chans[conn_index];

	registered_ops->sent(&fixed_chan->chan);
}

void bt_l2cap_fixed_register(const struct bt_l2cap_chan_ops *ops)
{
	__ASSERT(registered_ops == NULL, "Ops have already been registered");
	registered_ops = ops;
}

static inline enum bt_conn_state bt_conn_get_state(const struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	__ASSERT_NO_MSG(!err);

	return info.state;
}

int bt_l2cap_fixed_send(struct bt_conn *conn, const uint8_t *data, size_t data_len)
{
	int err;
	struct net_buf *buf;

	if (!conn || !data) {
		return -EINVAL;
	}

	if (data_len > CONFIG_BT_L2CAP_TX_MTU) {
		return -EMSGSIZE;
	}

	buf = bt_l2cap_create_pdu(NULL, 0);
	net_buf_add_mem(buf, data, data_len);

	uint8_t conn_index = bt_conn_index(conn);

	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(fixed_chans));

	struct bt_l2cap_le_chan *fixed_chan = &fixed_chans[conn_index];

	k_sched_lock();
	if (bt_conn_get_state(conn) == BT_CONN_STATE_CONNECTED) {
		err = bt_l2cap_send_pdu(fixed_chan, buf, sent_callback, NULL);
	} else {
		err = -ENOTCONN;
	}
	k_sched_unlock();

	if (err) {
		net_buf_unref(buf);
	}

	return err;
}
