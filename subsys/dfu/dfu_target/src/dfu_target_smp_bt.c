/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#include <bluetooth/services/dfu_smp.h>
#include <dfu/dfu_target_smp_bt.h>

LOG_MODULE_REGISTER(dfu_target_smp_bt, CONFIG_DFU_TARGET_LOG_LEVEL);

/* ATT Write Without Response header overhead (opcode + handle). */
#define ATT_WRITE_HEADER_SIZE 3U

static struct smp_transport ble_smpt;
static struct smp_client_transport_entry ble_smpt_entry;
static struct bt_dfu_smp *smp_bt_client;
static struct net_buf *rx_net_buf;

static bool transport_registered;

/* Sequence number of the SMP request currently handed to the DFU SMP client,
 * used to tell a retransmission of that request from a new one. Since the
 * sequence number rolls over at 255, we need the extra flag to signal if
 * sequence 0 is an outstanding request or the initial state.
 */
static uint8_t outstanding_seq;
static bool outstanding_seq_valid;

/* Handle received SMP command responses. The SMP response frame may
 * be split into multiple GATT notifications, in which case this
 * function will be called multiple times to receive the full response frame
 * before it is passed to the SMP client.
 */
static void ble_smp_rsp(struct bt_dfu_smp *dfu_smp)
{
	const struct bt_dfu_smp_rsp_state *rsp_state = bt_dfu_smp_rsp_state(dfu_smp);

	if (rsp_state->offset == 0) {
		/* Start of a new response: drop any stale reassembly buffer left
		 * over from an aborted/failed previous response.
		 */
		if (rx_net_buf != NULL) {
			smp_packet_free(rx_net_buf);
			rx_net_buf = NULL;
		}

		rx_net_buf = smp_packet_alloc();
		if (rx_net_buf == NULL) {
			LOG_ERR("No SMP packet buffer for response");
			return;
		}
	}

	if (rx_net_buf == NULL) {
		/* Buffer allocation failed on the first chunk. Ignore the rest of
		 * this response. The SMP client command times out and retries.
		 */
		return;
	}

	if (rsp_state->chunk_size > net_buf_tailroom(rx_net_buf)) {
		LOG_ERR("SMP response exceeds net_buf size (%u > %zu)",
			rsp_state->chunk_size, net_buf_tailroom(rx_net_buf));
		smp_packet_free(rx_net_buf);
		rx_net_buf = NULL;
		return;
	}

	net_buf_add_mem(rx_net_buf, rsp_state->data, rsp_state->chunk_size);

	if (bt_dfu_smp_rsp_total_check(dfu_smp)) {
		/* Whole response received. Pass frame to SMP client */
		smp_rx_req(&ble_smpt, rx_net_buf);
		rx_net_buf = NULL;
	}
}

static int ble_smp_output(struct net_buf *nb)
{
	int rc;
	uint8_t seq;

	if (smp_bt_client == NULL) {
		smp_packet_free(nb);
		return MGMT_ERR_ENOENT;
	}

	seq = ((const struct bt_dfu_smp_header *)nb->data)->seq;

	rc = bt_dfu_smp_command(smp_bt_client, ble_smp_rsp, nb->len, nb->data);

	if (rc == -EBUSY && outstanding_seq_valid && seq != outstanding_seq) {
		/* A previous request was abandoned without a response, leaving the
		 * DFU SMP client busy. This is a new request, not a retransmission,
		 * so clear the stale transaction and send it.
		 */
		bt_dfu_smp_reset(smp_bt_client);
		rc = bt_dfu_smp_command(smp_bt_client, ble_smp_rsp, nb->len, nb->data);
	}

	smp_packet_free(nb);

	if (rc == -EBUSY) {
		/* A busy error indicates that we are still awaiting a response to
		 * a previous SMP request. This can happen if the SMP client retry
		 * timer (CONFIG_SMP_CMD_RETRY_TIME) is triggering too early.
		 */
		LOG_DBG("An existing SMP request is still pending, retransmission dropped.");
		return MGMT_ERR_EOK;
	}

	if (rc) {
		LOG_ERR("Failed to send SMP command over BLE, err: %d", rc);
		return MGMT_ERR_EUNKNOWN;
	}

	outstanding_seq = seq;
	outstanding_seq_valid = true;

	return MGMT_ERR_EOK;
}

static void transport_reset(void)
{
	/* Clear any partially reassembled response after a disconnection. */
	if (rx_net_buf != NULL) {
		smp_packet_free(rx_net_buf);
		rx_net_buf = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if ((smp_bt_client == NULL) || (conn != bt_dfu_smp_conn(smp_bt_client))) {
		return;
	}

	bt_dfu_smp_reset(smp_bt_client);
	transport_reset();
}

BT_CONN_CB_DEFINE(dfu_target_smp_bt_conn_callbacks) = {
	.disconnected = disconnected,
};

/* Returns the maximum payload size for a single GATT write */
static uint16_t ble_smp_get_mtu(const struct net_buf *nb)
{
	struct bt_conn *conn;

	ARG_UNUSED(nb);

	if (smp_bt_client == NULL) {
		return 0;
	}

	conn = bt_dfu_smp_conn(smp_bt_client);
	if (conn == NULL) {
		return 0;
	}

	return (bt_gatt_get_mtu(conn) - ATT_WRITE_HEADER_SIZE);
}

int dfu_target_smp_bt_transport_register(struct bt_dfu_smp *smp_client)
{
	int rc;

	if (smp_client == NULL) {
		return -EINVAL;
	}

	smp_bt_client = smp_client;

	if (transport_registered) {
		/* Transport is already registered */
		return 0;
	}

	ble_smpt.functions.output = ble_smp_output;
	ble_smpt.functions.get_mtu = ble_smp_get_mtu;

	rc = smp_transport_init(&ble_smpt);
	if (rc) {
		LOG_ERR("Failed to init SMP BLE transport, err: %d", rc);
		smp_bt_client = NULL;
		return rc;
	}

	ble_smpt_entry.smpt = &ble_smpt;
	ble_smpt_entry.smpt_type = SMP_USER_DEFINED_TRANSPORT;
	smp_client_transport_register(&ble_smpt_entry);

	transport_registered = true;

	return 0;
}
