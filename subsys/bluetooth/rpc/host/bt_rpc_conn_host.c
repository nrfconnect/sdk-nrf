/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/kernel.h>

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

void bt_rpc_encode_bt_conn(struct nrf_rpc_cbor_ctx *encoder,
			   const struct bt_conn *conn)
{
	if (CONFIG_BT_MAX_CONN > 1) {
		ser_encode_uint(encoder, (uint8_t)bt_conn_index((struct bt_conn *)conn));
	}
}

struct bt_conn *bt_rpc_decode_bt_conn(struct nrf_rpc_cbor_ctx *ctx)
{
	/* Making bt_conn_lookup_index() public will be better approach. */
	extern struct bt_conn *bt_conn_lookup_index(uint8_t index);

	struct bt_conn *conn;
	uint8_t index;

	if (CONFIG_BT_MAX_CONN > 1) {
		index = ser_decode_uint(ctx);
	} else {
		index = 0;
	}

	conn = bt_conn_lookup_index(index);
	if (conn) {
		bt_conn_unref(conn);
	}

	return conn;
}

static void bt_conn_remote_update_ref(struct bt_conn *conn, int8_t value)
{
	if (value < 0) {
		bt_conn_unref(conn);
	} else {
		bt_conn_ref(conn);
	}
}

static void bt_conn_remote_update_ref_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	int8_t count;

	conn = bt_rpc_decode_bt_conn(ctx);
	count = ser_decode_int(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_remote_update_ref(conn, count);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_REMOTE_UPDATE_REF_RPC_CMD, handler_data);
}


NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_remote_update_ref, BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,
			 bt_conn_remote_update_ref_rpc_handler, NULL);

static inline void bt_conn_foreach_cb_callback(struct bt_conn *conn, void *data,
					       uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 11;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, (uintptr_t)data);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_conn_foreach_cb_encoder, bt_conn_foreach_cb_callback,
		 (struct bt_conn *conn, void *data), (conn, data));

static void bt_conn_foreach_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int type;
	bt_conn_foreach_cb func;
	void *data;

	type = ser_decode_int(ctx);
	func = (bt_conn_foreach_cb)ser_decode_callback(ctx, bt_conn_foreach_cb_encoder);
	data = (void *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_foreach(type, func, data);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_FOREACH_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_foreach, BT_CONN_FOREACH_RPC_CMD,
			 bt_conn_foreach_rpc_handler, NULL);


static void bt_conn_lookup_addr_le_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *result;
	uint8_t id;
	bt_addr_le_t peer_data;
	const bt_addr_le_t *peer;
	size_t buffer_size_max = 3;

	id = ser_decode_uint(ctx);
	peer = ser_decode_buffer(ctx, &peer_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_lookup_addr_le(id, peer);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		bt_rpc_encode_bt_conn(&ectx, result);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_LOOKUP_ADDR_LE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_lookup_addr_le, BT_CONN_LOOKUP_ADDR_LE_RPC_CMD,
			 bt_conn_lookup_addr_le_rpc_handler, NULL);

static void bt_conn_get_dst_out_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool result;
	const struct bt_conn *conn;
	bt_addr_le_t dst_data;
	bt_addr_le_t *dst = &dst_data;
	size_t buffer_size_max = 4;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	dst = (bt_addr_le_t *)bt_conn_get_dst(conn);
	result = (dst != NULL);

	buffer_size_max += sizeof(bt_addr_le_t);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_bool(&ectx, result);
		ser_encode_buffer(&ectx, dst, sizeof(bt_addr_le_t));

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_GET_DST_OUT_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_dst_out, BT_CONN_GET_DST_OUT_RPC_CMD,
			 bt_conn_get_dst_out_rpc_handler, NULL);

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static const size_t bt_conn_le_phy_info_buf_size =
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_phy_info, tx_phy) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_phy_info, rx_phy);

void bt_conn_le_phy_info_enc(struct nrf_rpc_cbor_ctx *encoder,
			     const struct bt_conn_le_phy_info *data)
{
	ser_encode_uint(encoder, data->tx_phy);
	ser_encode_uint(encoder, data->rx_phy);
}
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static const size_t bt_conn_le_data_len_info_buf_size =
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_data_len_info, tx_max_len) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_data_len_info, tx_max_time) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_data_len_info, rx_max_len) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_le_data_len_info, rx_max_time);

void bt_conn_le_data_len_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				  const struct bt_conn_le_data_len_info *data)
{
	ser_encode_uint(encoder, data->tx_max_len);
	ser_encode_uint(encoder, data->tx_max_time);
	ser_encode_uint(encoder, data->rx_max_len);
	ser_encode_uint(encoder, data->rx_max_time);
}
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

static const size_t bt_conn_info_buf_size =
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, type) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, role) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, id) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, le.interval) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, le.latency) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, le.timeout) +
	4 * (1 + sizeof(bt_addr_le_t)) +
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	bt_conn_le_phy_info_buf_size +
#else
	1 +
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	bt_conn_le_data_len_info_buf_size +
#else
	1 +
#endif
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_info, state);

void bt_conn_info_enc(struct nrf_rpc_cbor_ctx *encoder, struct bt_conn_info *info)
{
	ser_encode_uint(encoder, info->type);
	ser_encode_uint(encoder, info->role);
	ser_encode_uint(encoder, info->id);

	if (info->type == BT_CONN_TYPE_LE) {
		ser_encode_uint(encoder, info->le.interval);
		ser_encode_uint(encoder, info->le.latency);
		ser_encode_uint(encoder, info->le.timeout);
		ser_encode_buffer(encoder, info->le.src, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.dst, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.local, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.remote, sizeof(bt_addr_le_t));
#if defined(CONFIG_BT_USER_PHY_UPDATE)
		bt_conn_le_phy_info_enc(encoder, info->le.phy);
#else
		ser_encode_null(encoder);
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		bt_conn_le_data_len_info_enc(encoder, info->le.data_len);
#else
		ser_encode_null(encoder);
#endif
		ser_encode_uint(encoder, info->state);
	} else {
		/* non-LE connection types are not supported. */
		ser_encoder_invalid(encoder);
	}
}

static void bt_conn_get_info_rpc_handler(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	const struct bt_conn *conn;
	size_t buffer_size_max = 5;

	struct bt_conn_info info;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_get_info(conn, &info);

	buffer_size_max += bt_conn_info_buf_size;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);

		bt_conn_info_enc(&ectx, &info);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_GET_INFO_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_info, BT_CONN_GET_INFO_RPC_CMD,
			 bt_conn_get_info_rpc_handler, NULL);


static const size_t bt_conn_remote_info_buf_size =
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_remote_info, type) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_remote_info, version) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_remote_info, manufacturer) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_conn_remote_info, subversion) +
	1 + 8 * sizeof(uint8_t);

void bt_conn_remote_info_enc(struct nrf_rpc_cbor_ctx *encoder,
	struct bt_conn_remote_info *remote_info)
{
	ser_encode_uint(encoder, remote_info->type);
	ser_encode_uint(encoder, remote_info->version);
	ser_encode_uint(encoder, remote_info->manufacturer);
	ser_encode_uint(encoder, remote_info->subversion);

	if (remote_info->type == BT_CONN_TYPE_LE) {
		ser_encode_buffer(encoder, remote_info->le.features, 8 * sizeof(uint8_t));
	} else {
		/* non-LE connection types are not supported. */
		ser_encoder_invalid(encoder);
	}
}

static void bt_conn_get_remote_info_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	struct bt_conn *conn;
	size_t buffer_size_max = 5;

	struct bt_conn_remote_info remote_info;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_get_remote_info(conn, &remote_info);

	buffer_size_max += bt_conn_remote_info_buf_size;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);

		bt_conn_remote_info_enc(&ectx, &remote_info);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_GET_REMOTE_INFO_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_remote_info, BT_CONN_GET_REMOTE_INFO_RPC_CMD,
			 bt_conn_get_remote_info_rpc_handler, NULL);

void bt_le_conn_param_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_le_conn_param *data)
{
	ser_encode_uint(encoder, data->interval_min);
	ser_encode_uint(encoder, data->interval_max);
	ser_encode_uint(encoder, data->latency);
	ser_encode_uint(encoder, data->timeout);
}


void bt_le_conn_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_conn_param *data)
{
	data->interval_min = ser_decode_uint(ctx);
	data->interval_max = ser_decode_uint(ctx);
	data->latency = ser_decode_uint(ctx);
	data->timeout = ser_decode_uint(ctx);
}

static void bt_conn_le_param_update_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct bt_le_conn_param param;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_le_conn_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_le_param_update(conn, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_LE_PARAM_UPDATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_param_update, BT_CONN_LE_PARAM_UPDATE_RPC_CMD,
			 bt_conn_le_param_update_rpc_handler, NULL);

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void bt_conn_le_data_len_param_dec(struct nrf_rpc_cbor_ctx *ctx,
				   struct bt_conn_le_data_len_param *data)
{
	data->tx_max_len = ser_decode_uint(ctx);
	data->tx_max_time = ser_decode_uint(ctx);
}

static void bt_conn_le_data_len_update_rpc_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_le_data_len_param param;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_conn_le_data_len_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_le_data_len_update(conn, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_data_len_update, BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD,
			 bt_conn_le_data_len_update_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
void bt_conn_le_phy_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn_le_phy_param *data)
{
	data->options = ser_decode_uint(ctx);
	data->pref_tx_phy = ser_decode_uint(ctx);
	data->pref_rx_phy = ser_decode_uint(ctx);
}

static void bt_conn_le_phy_update_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_le_phy_param param;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_conn_le_phy_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_le_phy_update(conn, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_LE_PHY_UPDATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_phy_update, BT_CONN_LE_PHY_UPDATE_RPC_CMD,
			 bt_conn_le_phy_update_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

static void bt_conn_disconnect_rpc_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	uint8_t reason;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	reason = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_disconnect(conn, reason);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_DISCONNECT_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_disconnect, BT_CONN_DISCONNECT_RPC_CMD,
			 bt_conn_disconnect_rpc_handler, NULL);

#if defined(CONFIG_BT_CENTRAL)
void bt_conn_le_create_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn_le_create_param *data)
{
	data->options = ser_decode_uint(ctx);
	data->interval = ser_decode_uint(ctx);
	data->window = ser_decode_uint(ctx);
	data->interval_coded = ser_decode_uint(ctx);
	data->window_coded = ser_decode_uint(ctx);
	data->timeout = ser_decode_uint(ctx);
}

static void bt_conn_le_create_rpc_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	bt_addr_le_t peer_data;
	const bt_addr_le_t *peer;
	struct bt_conn_le_create_param create_param;
	struct bt_le_conn_param conn_param;
	struct bt_conn *conn_data;
	struct bt_conn **conn = &conn_data;
	size_t buffer_size_max = 8;

	peer = ser_decode_buffer(ctx, &peer_data, sizeof(bt_addr_le_t));
	bt_conn_le_create_param_dec(ctx, &create_param);
	bt_le_conn_param_dec(ctx, &conn_param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_le_create(peer, &create_param, &conn_param, conn);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		bt_rpc_encode_bt_conn(&ectx, *conn);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_LE_CREATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_create, BT_CONN_LE_CREATE_RPC_CMD,
			 bt_conn_le_create_rpc_handler, NULL);

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static void bt_conn_le_create_auto_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn_le_create_param create_param;
	struct bt_le_conn_param conn_param;
	int result;

	bt_conn_le_create_param_dec(ctx, &create_param);
	bt_le_conn_param_dec(ctx, &conn_param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_le_create_auto(&create_param, &conn_param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_LE_CREATE_AUTO_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_create_auto, BT_CONN_LE_CREATE_AUTO_RPC_CMD,
			 bt_conn_le_create_auto_rpc_handler, NULL);

static void bt_conn_create_auto_stop_rpc_handler(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_conn_create_auto_stop();

	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_create_auto_stop, BT_CONN_CREATE_AUTO_STOP_RPC_CMD,
			 bt_conn_create_auto_stop_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

#if !defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static void bt_le_set_auto_conn_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	struct bt_le_conn_param param_data;
	struct bt_le_conn_param *param;
	int result;

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	if (ser_decode_is_null(ctx)) {
		param = NULL;
	} else {
		param = &param_data;
		bt_le_conn_param_dec(ctx, param);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_set_auto_conn(addr, param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_SET_AUTO_CONN_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_set_auto_conn, BT_LE_SET_AUTO_CONN_RPC_CMD,
			 bt_le_set_auto_conn_rpc_handler, NULL);
#endif  /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */
#endif  /* defined(CONFIG_BT_CENTRAL) */

#if defined(CONFIG_BT_SMP)
static void bt_conn_set_security_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	bt_security_t sec;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	sec = (bt_security_t)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_set_security(conn, sec);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_SET_SECURITY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_set_security, BT_CONN_SET_SECURITY_RPC_CMD,
			 bt_conn_set_security_rpc_handler, NULL);

static void bt_conn_get_security_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_security_t result;
	struct bt_conn *conn;
	size_t buffer_size_max = 5;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_get_security(conn);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_uint(&ectx, (uint32_t)result);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CONN_GET_SECURITY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_security, BT_CONN_GET_SECURITY_RPC_CMD,
			 bt_conn_get_security_rpc_handler, NULL);

static void bt_conn_enc_key_size_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	uint8_t result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_enc_key_size(conn);

	ser_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_ENC_KEY_SIZE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_enc_key_size, BT_CONN_ENC_KEY_SIZE_RPC_CMD,
			 bt_conn_enc_key_size_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_SMP) */

static void bt_conn_cb_connected_call(struct bt_conn *conn, uint8_t err)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, err);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_CONNECTED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static void bt_conn_cb_disconnected_call(struct bt_conn *conn, uint8_t reason)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, reason);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static bool bt_conn_cb_le_param_req_call(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;
	size_t buffer_size_max = 15;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_le_conn_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_bool, &result);

	return result;
}

static void bt_conn_cb_le_param_updated_call(struct bt_conn *conn, uint16_t interval, uint16_t
					     latency, uint16_t timeout)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 12;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, interval);
	ser_encode_uint(&ctx, latency);
	ser_encode_uint(&ctx, timeout);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

#if defined(CONFIG_BT_SMP)
static void bt_conn_cb_identity_resolved_call(struct bt_conn *conn, const bt_addr_le_t *rpa, const
					      bt_addr_le_t *identity)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 9;

	buffer_size_max += rpa ? sizeof(bt_addr_le_t) : 0;
	buffer_size_max += identity ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_buffer(&ctx, rpa, sizeof(bt_addr_le_t));
	ser_encode_buffer(&ctx, identity, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static void bt_conn_cb_security_changed_call(struct bt_conn *conn, bt_security_t level,
					     enum bt_security_err err)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 13;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, (uint32_t)level);
	ser_encode_uint(&ctx, (uint32_t)err);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}
#endif /* defined(CONFIG_BT_SMP) */

#if defined(CONFIG_BT_REMOTE_INFO)
static void bt_conn_cb_remote_info_available_call(struct bt_conn *conn,
						  struct bt_conn_remote_info *remote_info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;

	buffer_size_max += bt_conn_remote_info_buf_size;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	bt_conn_remote_info_enc(&ctx, remote_info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void bt_conn_cb_le_phy_updated_call(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 7;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_conn_le_phy_info_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void bt_conn_cb_le_data_len_updated_call(struct bt_conn *conn,
						struct bt_conn_le_data_len_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 15;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_conn_le_data_len_info_enc(&ctx, info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

static struct bt_conn_cb bt_conn_cb_register_data = {

	.connected = bt_conn_cb_connected_call,
	.disconnected = bt_conn_cb_disconnected_call,
	.le_param_req = bt_conn_cb_le_param_req_call,
	.le_param_updated = bt_conn_cb_le_param_updated_call,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = bt_conn_cb_identity_resolved_call,
	.security_changed = bt_conn_cb_security_changed_call,
#endif /* defined(CONFIG_BT_SMP) */
#if defined(CONFIG_BT_REMOTE_INFO)
	.remote_info_available = bt_conn_cb_remote_info_available_call,
#endif /* defined(CONFIG_BT_REMOTE_INFO) */
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = bt_conn_cb_le_phy_updated_call,
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = bt_conn_cb_le_data_len_updated_call,
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
};

static void bt_conn_cb_register_on_remote_rpc_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	bt_conn_cb_register(&bt_conn_cb_register_data);

	ser_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_register_on_remote,
			 BT_CONN_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_conn_cb_register_on_remote_rpc_handler, NULL);

#if defined(CONFIG_BT_SMP)
static void bt_set_bondable_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enable;

	enable = ser_decode_bool(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_set_bondable(enable);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_SET_BONDABLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_bondable, BT_SET_BONDABLE_RPC_CMD,
			 bt_set_bondable_rpc_handler, NULL);


static void bt_le_oob_set_legacy_flag_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enable;

	enable = ser_decode_bool(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_le_oob_set_legacy_flag(enable);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_SET_LEGACY_FLAG_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_legacy_flag, BT_LE_OOB_SET_LEGACY_FLAG_RPC_CMD,
			 bt_le_oob_set_legacy_flag_rpc_handler, NULL);


static void bt_le_oob_set_sc_flag_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enable;

	enable = ser_decode_bool(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_le_oob_set_sc_flag(enable);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_SET_SC_FLAG_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_sc_flag, BT_LE_OOB_SET_SC_FLAG_RPC_CMD,
			 bt_le_oob_set_sc_flag_rpc_handler, NULL);


#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
static void bt_le_oob_set_legacy_tk_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	const uint8_t *tk;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	tk = ser_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_oob_set_legacy_tk(conn, tk);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_SET_LEGACY_TK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_legacy_tk, BT_LE_OOB_SET_LEGACY_TK_RPC_CMD,
			 bt_le_oob_set_legacy_tk_rpc_handler, NULL);
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
size_t bt_le_oob_sc_data_buf_size(const struct bt_le_oob_sc_data *data)
{
	size_t buffer_size_max = 10;

	buffer_size_max += 16 * sizeof(uint8_t);
	buffer_size_max += 16 * sizeof(uint8_t);

	return buffer_size_max;
}

void bt_le_oob_sc_data_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_oob_sc_data *data)
{
	ser_encode_buffer(encoder, data->r, 16 * sizeof(uint8_t));
	ser_encode_buffer(encoder, data->c, 16 * sizeof(uint8_t));
}

void bt_le_oob_sc_data_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_oob_sc_data *data)
{
	ser_decode_buffer(ctx, data->r, 16 * sizeof(uint8_t));
	ser_decode_buffer(ctx, data->c, 16 * sizeof(uint8_t));
}

static void bt_le_oob_set_sc_data_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct bt_le_oob_sc_data oobd_local_data;
	struct bt_le_oob_sc_data *oobd_local;
	struct bt_le_oob_sc_data oobd_remote_data;
	struct bt_le_oob_sc_data *oobd_remote;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	if (ser_decode_is_null(ctx)) {
		oobd_local = NULL;
	} else {
		oobd_local = &oobd_local_data;
		bt_le_oob_sc_data_dec(ctx, oobd_local);
	}
	if (ser_decode_is_null(ctx)) {
		oobd_remote = NULL;
	} else {
		oobd_remote = &oobd_remote_data;
		bt_le_oob_sc_data_dec(ctx, oobd_remote);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_SET_SC_DATA_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_sc_data, BT_LE_OOB_SET_SC_DATA_RPC_CMD,
			 bt_le_oob_set_sc_data_rpc_handler, NULL);

static void bt_le_oob_get_sc_data_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	struct bt_conn *conn;
	size_t buffer_size_max = 5;

	const struct bt_le_oob_sc_data *oobd_local;
	const struct bt_le_oob_sc_data *oobd_remote;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_oob_get_sc_data(conn, &oobd_local, &oobd_remote);

	buffer_size_max += bt_le_oob_sc_data_buf_size(oobd_local);
	buffer_size_max += bt_le_oob_sc_data_buf_size(oobd_remote);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);

		if (!oobd_local) {
			ser_encode_null(&ectx);
		} else {
			bt_le_oob_sc_data_enc(&ectx, oobd_local);
		}

		if (!oobd_remote) {
			ser_encode_null(&ectx);
		} else {
			bt_le_oob_sc_data_enc(&ectx, oobd_remote);
		}

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_GET_SC_DATA_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_get_sc_data, BT_LE_OOB_GET_SC_DATA_RPC_CMD,
			 bt_le_oob_get_sc_data_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

#if defined(CONFIG_BT_FIXED_PASSKEY)
static void bt_passkey_set_rpc_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	unsigned int passkey;
	int result;

	passkey = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_passkey_set(passkey);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_PASSKEY_SET_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_passkey_set, BT_PASSKEY_SET_RPC_CMD,
			 bt_passkey_set_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_FIXED_PASSKEY) */

static struct bt_conn_auth_cb remote_auth_cb;

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
static const size_t bt_conn_pairing_feat_buf_size = 12;

void bt_conn_pairing_feat_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_conn_pairing_feat *data)
{
	ser_encode_uint(encoder, data->io_capability);
	ser_encode_uint(encoder, data->oob_data_flag);
	ser_encode_uint(encoder, data->auth_req);
	ser_encode_uint(encoder, data->max_enc_key_size);
	ser_encode_uint(encoder, data->init_key_dist);
	ser_encode_uint(encoder, data->resp_key_dist);
}

struct bt_rpc_auth_cb_pairing_accept_rpc_res {

	enum bt_security_err result;

};

static void bt_rpc_auth_cb_pairing_accept_rpc_rsp(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_rpc_auth_cb_pairing_accept_rpc_res *res =
		(struct bt_rpc_auth_cb_pairing_accept_rpc_res *)handler_data;

	res->result = (enum bt_security_err)ser_decode_uint(ctx);
}

enum bt_security_err bt_rpc_auth_cb_pairing_accept(struct bt_conn *conn,
						   const struct bt_conn_pairing_feat *const feat)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_rpc_auth_cb_pairing_accept_rpc_res result;
	size_t buffer_size_max = 3;

	buffer_size_max += bt_conn_pairing_feat_buf_size;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_conn_pairing_feat_enc(&ctx, feat);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD,
				&ctx, bt_rpc_auth_cb_pairing_accept_rpc_rsp, &result);

	return result.result;
}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

void bt_rpc_auth_cb_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, passkey);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_cb_passkey_entry(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_cb_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, passkey);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_cb_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 6;

	buffer_size_max += info ? sizeof(struct bt_conn_oob_info) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_buffer(&ctx, info, sizeof(struct bt_conn_oob_info));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_cb_cancel(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_CANCEL_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_cb_pairing_confirm(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static int bt_conn_auth_cb_register_on_remote(uint16_t flags)
{
	if (flags & FLAG_AUTH_CB_IS_NULL) {
		return bt_conn_auth_cb_register(NULL);
	}

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	remote_auth_cb.pairing_accept = (flags & FLAG_PAIRING_ACCEPT_PRESENT) ?
					bt_rpc_auth_cb_pairing_accept : NULL;
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
	remote_auth_cb.passkey_display = (flags & FLAG_PASSKEY_DISPLAY_PRESENT) ?
					 bt_rpc_auth_cb_passkey_display : NULL;
	remote_auth_cb.passkey_entry = (flags & FLAG_PASSKEY_ENTRY_PRESENT) ?
				       bt_rpc_auth_cb_passkey_entry : NULL;
	remote_auth_cb.passkey_confirm = (flags & FLAG_PASSKEY_CONFIRM_PRESENT) ?
					 bt_rpc_auth_cb_passkey_confirm : NULL;
	remote_auth_cb.oob_data_request = (flags & FLAG_OOB_DATA_REQUEST_PRESENT) ?
					  bt_rpc_auth_cb_oob_data_request : NULL;
	remote_auth_cb.cancel = (flags & FLAG_CANCEL_PRESENT) ? bt_rpc_auth_cb_cancel : NULL;
	remote_auth_cb.pairing_confirm = (flags & FLAG_PAIRING_CONFIRM_PRESENT) ?
					 bt_rpc_auth_cb_pairing_confirm : NULL;

	return bt_conn_auth_cb_register(&remote_auth_cb);
}

static void bt_conn_auth_cb_register_on_remote_rpc_handler(const struct nrf_rpc_group *group,
							   struct nrf_rpc_cbor_ctx *ctx,
							   void *handler_data)
{
	uint16_t flags;
	int result;

	flags = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_cb_register_on_remote(flags);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_cb_register_on_remote,
			 BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_conn_auth_cb_register_on_remote_rpc_handler, NULL);

static struct bt_conn_auth_info_cb remote_auth_info_cb;

void bt_rpc_auth_info_cb_pairing_complete(struct bt_conn *conn, bool bonded)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 4;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_bool(&ctx, bonded);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_INFO_CB_PAIRING_COMPLETE_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_info_cb_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, (uint32_t)reason);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_INFO_CB_PAIRING_FAILED_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_rpc_auth_info_cb_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;

	buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, peer, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_INFO_CB_BOND_DELETED_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static int bt_conn_auth_info_cb_register_on_remote(uint16_t flags)
{
	if (flags & FLAG_AUTH_CB_IS_NULL) {
		return bt_conn_auth_info_cb_register(NULL);
	}

	remote_auth_info_cb.pairing_complete = (flags & FLAG_PAIRING_COMPLETE_PRESENT) ?
						bt_rpc_auth_info_cb_pairing_complete : NULL;
	remote_auth_info_cb.pairing_failed = (flags & FLAG_PAIRING_FAILED_PRESENT) ?
					      bt_rpc_auth_info_cb_pairing_failed : NULL;
	remote_auth_info_cb.bond_deleted = (flags & FLAG_BOND_DELETED_PRESENT) ?
					    bt_rpc_auth_info_cb_bond_deleted : NULL;

	return bt_conn_auth_info_cb_register(&remote_auth_info_cb);
}

static void bt_conn_auth_info_cb_register_on_remote_rpc_handler(const struct nrf_rpc_group *group,
								struct nrf_rpc_cbor_ctx *ctx,
								void *handler_data)
{
	uint16_t flags;
	int result;

	flags = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_info_cb_register_on_remote(flags);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_INFO_CB_REGISTER_ON_REMOTE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_info_cb_register_on_remote,
			 BT_CONN_AUTH_INFO_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_conn_auth_info_cb_register_on_remote_rpc_handler, NULL);

static void bt_conn_auth_info_cb_unregister_on_remote_rpc_handler(const struct nrf_rpc_group *group,
								  struct nrf_rpc_cbor_ctx *ctx,
								  void *handler_data)
{
	int result;

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_info_cb_unregister(&remote_auth_info_cb);

	ser_rsp_send_int(group, result);

	return;

decoding_error:
	report_decoding_error(BT_CONN_AUTH_INFO_CB_UNREGISTER_ON_REMOTE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_info_cb_unregister_on_remote,
			 BT_CONN_AUTH_INFO_CB_UNREGISTER_ON_REMOTE_RPC_CMD,
			 bt_conn_auth_info_cb_unregister_on_remote_rpc_handler, NULL);

static void bt_conn_auth_passkey_entry_rpc_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	unsigned int passkey;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	passkey = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_passkey_entry(conn, passkey);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_passkey_entry, BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD,
			 bt_conn_auth_passkey_entry_rpc_handler, NULL);

static void bt_conn_auth_cancel_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_cancel(conn);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_CANCEL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_cancel, BT_CONN_AUTH_CANCEL_RPC_CMD,
			 bt_conn_auth_cancel_rpc_handler, NULL);

static void bt_conn_auth_passkey_confirm_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_conn *conn;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_passkey_confirm(conn);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_passkey_confirm,
			 BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD,
			 bt_conn_auth_passkey_confirm_rpc_handler, NULL);

static void bt_conn_auth_pairing_confirm_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_conn *conn;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_auth_pairing_confirm(conn);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_pairing_confirm,
			 BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD,
			 bt_conn_auth_pairing_confirm_rpc_handler, NULL);

#endif /* defined(CONFIG_BT_SMP) */
