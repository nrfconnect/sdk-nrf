/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <zephyr/bluetooth/buf.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

#define CMD_BUF_SIZE MAX(BT_BUF_EVT_RX_SIZE, BT_BUF_CMD_TX_SIZE)
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, CONFIG_BT_BUF_CMD_TX_COUNT, CMD_BUF_SIZE, 8, NULL);

bool bt_addr_le_is_bonded(uint8_t id, const bt_addr_le_t *addr)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;
	size_t buffer_size_max = 5;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ADDR_LE_IS_BONDED_CMD,
				&ctx, ser_rsp_decode_bool, &result);

	return result;
}

struct bt_hci_cmd_send_sync_res {
	int result;
	struct net_buf *buf;
	struct net_buf **rsp;
};

static void decode_net_buf(struct ser_scratchpad *scratchpad, struct net_buf *data)
{
	size_t len;
	void *buf;

	buf = ser_decode_buffer_into_scratchpad(scratchpad, &len);
	net_buf_add_mem(data, buf, len);
}

static void bt_hci_cmd_send_sync_rsp(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_hci_cmd_send_sync_res *res;
	struct ser_scratchpad scratchpad;

	res = (struct bt_hci_cmd_send_sync_res *)handler_data;

	res->result = ser_decode_int(ctx);

	if (ser_decode_is_null(ctx)) {
		*res->rsp = NULL;
	} else {
		SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);
		*res->rsp = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
		decode_net_buf(&scratchpad, *res->rsp);
	}
}

int bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_hci_cmd_send_sync_res result;
	size_t buffer_size_max = 5;
	size_t scratchpad_size;

	buffer_size_max += buf == NULL ? 1 : 13 + buf->len;
	buffer_size_max += rsp == NULL ? 1 : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, opcode);

	if (buf == NULL) {
		ser_encode_null(&ctx);
	} else {
		scratchpad_size = SCRATCHPAD_ALIGN(buf->len);
		ser_encode_uint(&ctx, scratchpad_size);
		ser_encode_uint(&ctx, buf->len);
		ser_encode_buffer(&ctx, buf->data, buf->len);
	}

	if (rsp == NULL) {
		/* The caller is not interested in the response. */
		ser_encode_null(&ctx);
	}

	result.buf = buf;
	result.rsp = rsp;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_HCI_CMD_SEND_SYNC_RPC_CMD,
				&ctx, bt_hci_cmd_send_sync_rsp, &result);

	return result.result;
}
