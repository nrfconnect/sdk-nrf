/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <hci_core.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static void bt_addr_le_is_bonded_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	uint8_t id;
	bool result;

	id = ser_decode_uint(ctx);
	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_addr_le_is_bonded(id, addr);
	ser_rsp_send_bool(group, result);

	return;

decoding_error:
	report_decoding_error(BT_ADDR_LE_IS_BONDED_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_addr_le_is_bonded, BT_ADDR_LE_IS_BONDED_CMD,
			 bt_addr_le_is_bonded_rpc_handler, NULL);

static void decode_net_buf(struct ser_scratchpad *scratchpad, struct net_buf *data)
{
	size_t len;
	void *buf;

	buf = ser_decode_buffer_into_scratchpad(scratchpad, &len);
	net_buf_add_mem(data, buf, len);
}

static void bt_hci_cmd_send_sync_rsp(const struct nrf_rpc_group *group, int result,
				     const struct net_buf *rsp)
{
	size_t buffer_size_max = 20;
	size_t scratchpad_size = 0;
	struct nrf_rpc_cbor_ctx ctx;

	buffer_size_max += (rsp == NULL) ? 1 : (3 + rsp->len);
	NRF_RPC_CBOR_ALLOC(group, ctx, buffer_size_max);

	ser_encode_int(&ctx, result);

	if (rsp == NULL) {
		ser_encode_null(&ctx);
	} else {
		scratchpad_size = SCRATCHPAD_ALIGN(rsp->len);
		ser_encode_uint(&ctx, scratchpad_size);
		ser_encode_buffer(&ctx, rsp->data, rsp->len);
	}

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

static void bt_hci_cmd_send_sync_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *hanler_data)
{
	int ret = 0;
	uint16_t opcode;
	size_t len;
	struct net_buf *buf = NULL;
	struct net_buf *rsp = NULL;
	struct ser_scratchpad scratchpad;
	bool response;

	opcode = ser_decode_uint(ctx);

	if (ser_decode_is_null(ctx)) {
		ser_decode_skip(ctx);
		buf = NULL;
	} else {
		SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);
		len = ser_decode_uint(ctx);
		buf = bt_hci_cmd_create(opcode, len);
		if (!buf) {
			ret = -ENOBUFS;
		} else {
			decode_net_buf(&scratchpad, buf);
		}
	}

	if (ser_decode_is_null(ctx)) {
		/* The caller is not interested in the response. */
		response = false;
	} else {
		response = true;
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (!ret) {
		ret = bt_hci_cmd_send_sync(opcode, buf, response ? &rsp : NULL);
	}

	bt_hci_cmd_send_sync_rsp(group, ret, rsp);

	if (response && (ret == 0)) {
		net_buf_unref(rsp);
	}

	return;

decoding_error:
	report_decoding_error(BT_HCI_CMD_SEND_SYNC_RPC_CMD, hanler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_hci_cmd_send_sync, BT_HCI_CMD_SEND_SYNC_RPC_CMD,
			 bt_hci_cmd_send_sync_rpc_handler, NULL);
