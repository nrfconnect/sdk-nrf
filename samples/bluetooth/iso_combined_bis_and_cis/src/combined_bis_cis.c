/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CIS central implementation for the time sync sample
 *
 * This file implements a device that acts as a CIS central.
 * The central can either be configured as a transmitter or a receiver.
 *
 * It connects to as many peripherals as it has available ISO channels.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/ring_buffer.h>

#include "iso_combined_bis_and_cis.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_bis_cis, LOG_LEVEL_INF);

#define ADV_NAME_STR_LEN (sizeof(CONFIG_BT_DEVICE_NAME))

#define SDU_INTERVAL_US			(10000u) /* 10 ms */
#define ACL_INTERVAL_BLE_UNITS		(16u)	 /* 20 ms in 1.25 ms BLE units*/
#define PERIODIC_ADV_INTERVAL_BLE_UNITS (16u)	 /* 20 ms in 1.25 ms BLE units*/
#define RETRANSMISSION_NUMBER		(10u)
#define MAX_TRANSPORT_LATENCY_MS	(20u)
#define SDU_RING_BUFFER_SIZE		(4u)

/* Ring buffer contains length of SDU before each SDU, so allocate 1 additional byte */
#define RING_BUF_ELEMENT_SIZE (1 + CONFIG_BT_ISO_TX_MTU)
RING_BUF_DECLARE(sdu_rb, SDU_RING_BUFFER_SIZE * (RING_BUF_ELEMENT_SIZE));
static uint8_t rcvd_sdu_cnt;

static void scan_start(void)
{
	int err;

	err = bt_le_scan_start(
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_FILTER_DUPLICATE,
				 BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL),
		NULL);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}
}

static struct bt_iso_chan *free_iso_chan_get(void)
{
	struct bt_iso_chan **iso_channels;

	iso_channels = iso_rx_channels_get();

	struct bt_iso_chan *iso_chan = NULL;

	if (iso_channels[0]->state == BT_ISO_STATE_DISCONNECTED) {
		iso_chan = iso_channels[0];
	}

	return iso_chan;
}

static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, ADV_NAME_STR_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name_str[ADV_NAME_STR_LEN] = {0};

	bt_data_parse(buf, adv_data_parse_cb, name_str);

	if (strncmp(name_str, CONFIG_BT_DEVICE_NAME, ADV_NAME_STR_LEN) != 0) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	const struct bt_le_conn_param *conn_param =
		BT_LE_CONN_PARAM(ACL_INTERVAL_BLE_UNITS, ACL_INTERVAL_BLE_UNITS, 0, 400);

	struct bt_conn *conn = NULL;
	int err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN, conn_param, &conn);

	if (err) {
		LOG_ERR("Create conn to %s failed (%d)", name_str, err);
	} else {
		bt_conn_unref(conn);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void process_rcvd_sdu_cb(struct net_buf *buf)
{
	uint8_t sdu_buffer[CONFIG_BT_ISO_TX_MTU];
	uint8_t sdu_len = (uint8_t)buf->len;

	memcpy(sdu_buffer, buf->data, sdu_len);

	/* write length of SDU to ring-buffer */
	uint32_t wrote = ring_buf_put(&sdu_rb, &sdu_len, 1);

	__ASSERT_NO_MSG(wrote == 1);
	/* write SDU content to ring-buffer */
	wrote = ring_buf_put(&sdu_rb, sdu_buffer, sdu_len);
	__ASSERT_NO_MSG(wrote == sdu_len);
	if (rcvd_sdu_cnt < SDU_RING_BUFFER_SIZE)
		rcvd_sdu_cnt++;
}

static void set_sdu_data_cb(struct net_buf *buf)
{
   /* Start forwarding data when ring buffer is full.
    * It is needed to avoid possible packets loss due to
    * transport latency of CIS stream
    */
	if (rcvd_sdu_cnt >= SDU_RING_BUFFER_SIZE) {
		uint8_t sdu_len;
		uint8_t sdu_buffer[CONFIG_BT_ISO_TX_MTU];

		/* read length of SDU from ring-buffer */
		ring_buf_get(&sdu_rb, &sdu_len, 1);
		/* read SDU content from ring-buffer */
		ring_buf_get(&sdu_rb, sdu_buffer, sdu_len);
		memcpy(buf->data, sdu_buffer, sdu_len);
		buf->len = sdu_len;
	}
}

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void bis_transmitter_start(void)
{
	int err;
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (err) {
		LOG_INF("Failed to create advertising set (err %d)", err);
		return;
	}

	/* Set the advertising data */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_INF("failed to set advertising data (err %d)", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_PARAM(PERIODIC_ADV_INTERVAL_BLE_UNITS,
							       PERIODIC_ADV_INTERVAL_BLE_UNITS,
							       BT_LE_PER_ADV_OPT_NONE));
	if (err) {
		LOG_INF("Failed to set periodic advertising parameters"
			" (err %d)",
			err);
		return;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_INF("Failed to start extended advertising (err %d)", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		LOG_INF("Failed to enable periodic advertising (err %d)", err);
		return;
	}

	struct bt_iso_big_create_param big_create_param = {
		.num_bis = 1,
		.bis_channels = iso_tx_channels_get(),
		.interval = SDU_INTERVAL_US,
		.latency = MAX_TRANSPORT_LATENCY_MS,
		.packing = 0, /* 0 - sequential, 1 - interleaved */
		.framing = 0, /* 0 - unframed, 1 - framed */
	};

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		LOG_INF("Failed to create BIG (err %d)", err);
		return;
	}

	LOG_INF("BIS transmitter started");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_INF("Failed to connect to %s, 0x%02x %s", addr, err, bt_hci_err_to_str(err));

		scan_start();
		return;
	}

	LOG_INF("Connected: %s", addr);

	/* Find a free ISO channel */
	struct bt_iso_chan *iso_chan = free_iso_chan_get();

	if (iso_chan == NULL) {
		LOG_INF("No ISO channel available");
		return;
	}

	struct bt_iso_connect_param connect_param;

	connect_param.acl = conn;
	connect_param.iso_chan = iso_chan;

	LOG_INF("Connecting ISO channel");
	err = bt_iso_chan_connect(&connect_param, 1);
	if (err) {
		LOG_ERR("Failed to connect iso (%d)", err);
		return;
	}

	bis_transmitter_start();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	scan_start();
}

/* Always reject connection parameter update requests in order to keep ACL interval aligned with CIS
 * and BIS ISO-interval
 */
static bool le_param_req_reject(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	return false;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req_reject,
};

void combined_cis_bis_start(void)
{
	struct bt_iso_cig_param param;
	struct bt_iso_cig *cig;
	int err;

	iso_rx_init(RETRANSMISSION_NUMBER, process_rcvd_sdu_cb);
	iso_tx_init(RETRANSMISSION_NUMBER, set_sdu_data_cb);

	bt_le_scan_cb_register(&scan_callbacks);
	bt_conn_cb_register(&conn_callbacks);

	memset(&param, 0, sizeof(param));
	param.cis_channels = iso_rx_channels_get();
	param.num_cis = 1;
	param.sca = BT_GAP_SCA_UNKNOWN;
	param.packing = 0;
	param.framing = 0;
	param.c_to_p_latency = MAX_TRANSPORT_LATENCY_MS;
	param.p_to_c_latency = MAX_TRANSPORT_LATENCY_MS;
	param.c_to_p_interval = SDU_INTERVAL_US;
	param.p_to_c_interval = SDU_INTERVAL_US;

	err = bt_iso_cig_create(&param, &cig);
	if (err != 0) {
		LOG_ERR("Failed to create CIG (%d)", err);
		return;
	}

	scan_start();

	LOG_INF("CIS central started scanning for peripheral(s)");
}
