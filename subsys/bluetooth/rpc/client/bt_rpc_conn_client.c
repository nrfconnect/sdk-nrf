/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"
#include <nrf_rpc_cbor.h>

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

#define LOCK_CONN_INFO() k_mutex_lock(&bt_rpc_conn_mutex, K_FOREVER)
#define UNLOCK_CONN_INFO() k_mutex_unlock(&bt_rpc_conn_mutex)

static K_MUTEX_DEFINE(bt_rpc_conn_mutex);

struct bt_conn {
	atomic_t ref;
	uint8_t features[8];
	bt_addr_le_t src;
	bt_addr_le_t dst;
	bt_addr_le_t local;
	bt_addr_le_t remote;
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	struct bt_conn_le_phy_info phy;
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_info data_len;
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */
#if defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	struct bt_le_oob_sc_data oobd_local;
	struct bt_le_oob_sc_data oobd_remote;
#endif /* defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */
};

static struct bt_conn connections[CONFIG_BT_MAX_CONN];

static inline uint8_t get_conn_index(const struct bt_conn *conn)
{
	return (uint8_t)(conn - connections);
}

void bt_rpc_encode_bt_conn(struct nrf_rpc_cbor_ctx *encoder, const struct bt_conn *conn)
{
	uint8_t index;

	if (CONFIG_BT_MAX_CONN > 1) {
		/* In case when conn is NULL encode index which is out of range
		 * to decode it as NULL on the host.
		 */
		index = conn ? get_conn_index(conn) : CONFIG_BT_MAX_CONN;

		ser_encode_uint(encoder, index);
	}
}

struct bt_conn *bt_rpc_decode_bt_conn(struct nrf_rpc_cbor_ctx *ctx)
{
	uint8_t index = 0;

	if (CONFIG_BT_MAX_CONN > 1) {
		index = ser_decode_uint(ctx);
		if (index >= CONFIG_BT_MAX_CONN) {
			ser_decoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
			return NULL;
		}
	}
	return &connections[index];
}

static void bt_conn_remote_update_ref(struct bt_conn *conn, int8_t value)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_int(&ctx, value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static void bt_conn_ref_local(struct bt_conn *conn)
{
	if (conn) {
		atomic_inc(&conn->ref);
	}
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_inc(&conn->ref);

	if (old == 0) {
		bt_conn_remote_update_ref(conn, + 1);
	}

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_dec(&conn->ref);

	if (old == 1) {
		bt_conn_remote_update_ref(conn, -1);
	}
}

static void bt_conn_foreach_cb_callback_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	struct bt_conn *conn;
	void *data;
	bt_conn_foreach_cb callback_slot;

	conn = bt_rpc_decode_bt_conn(ctx);
	data = (void *)ser_decode_uint(ctx);
	callback_slot = (bt_conn_foreach_cb)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(conn, data);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_foreach_cb_callback,
			 BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD,
			 bt_conn_foreach_cb_callback_rpc_handler, NULL);

void bt_conn_foreach(enum bt_conn_type type, void (*func)(struct bt_conn *conn, void *data),
		     void *data)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 15;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_int(&ctx, type);
	ser_encode_callback(&ctx, func);
	ser_encode_uint(&ctx, (uintptr_t)data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_FOREACH_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

struct bt_conn_lookup_addr_le_rpc_res {
	struct bt_conn *result;
};

static void bt_conn_lookup_addr_le_rpc_rsp(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx,
					   void *handler_data)
{
	struct bt_conn_lookup_addr_le_rpc_res *res =
		(struct bt_conn_lookup_addr_le_rpc_res *)handler_data;

	res->result = bt_rpc_decode_bt_conn(ctx);

	bt_conn_ref_local(res->result);
}

struct bt_conn *bt_conn_lookup_addr_le(uint8_t id, const bt_addr_le_t *peer)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_lookup_addr_le_rpc_res result;
	size_t buffer_size_max = 5;

	buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, peer, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LOOKUP_ADDR_LE_RPC_CMD,
				&ctx, bt_conn_lookup_addr_le_rpc_rsp, &result);

	return result.result;
}

struct bt_conn_get_dst_out_rpc_res {
	bool result;
	bt_addr_le_t *dst;

};

static void bt_conn_get_dst_out_rpc_rsp(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx,
					void *handler_data)
{
	struct bt_conn_get_dst_out_rpc_res *res =
		(struct bt_conn_get_dst_out_rpc_res *)handler_data;

	res->result = ser_decode_bool(ctx);
	ser_decode_buffer(ctx, res->dst, sizeof(bt_addr_le_t));
}

static bool bt_conn_get_dst_out(const struct bt_conn *conn, bt_addr_le_t *dst)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_get_dst_out_rpc_res result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	result.dst = dst;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_DST_OUT_RPC_CMD,
				&ctx, bt_conn_get_dst_out_rpc_rsp, &result);

	return result.result;
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	bool not_null;

	not_null = bt_conn_get_dst_out(conn, (bt_addr_le_t *)&conn->dst);

	if (not_null) {
		return &conn->dst;
	} else {
		return NULL;
	}
}

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	return get_conn_index(conn);
}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
void bt_conn_le_phy_info_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn_le_phy_info *data)
{
	data->tx_phy = ser_decode_uint(ctx);
	data->rx_phy = ser_decode_uint(ctx);
}
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void bt_conn_le_data_len_info_dec(struct nrf_rpc_cbor_ctx *ctx,
				  struct bt_conn_le_data_len_info *data)
{
	data->tx_max_len = ser_decode_uint(ctx);
	data->tx_max_time = ser_decode_uint(ctx);
	data->rx_max_len = ser_decode_uint(ctx);
	data->rx_max_time = ser_decode_uint(ctx);
}
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

void bt_conn_info_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn *conn,
		      struct bt_conn_info *info)
{
	info->type = ser_decode_uint(ctx);
	info->role = ser_decode_uint(ctx);
	info->id = ser_decode_uint(ctx);

	if (info->type == BT_CONN_TYPE_LE) {
		info->le.interval = ser_decode_uint(ctx);
		info->le.latency = ser_decode_uint(ctx);
		info->le.timeout = ser_decode_uint(ctx);
		LOCK_CONN_INFO();
		info->le.src = ser_decode_buffer(ctx, &conn->src, sizeof(bt_addr_le_t));
		info->le.dst = ser_decode_buffer(ctx, &conn->dst, sizeof(bt_addr_le_t));
		info->le.local = ser_decode_buffer(ctx, &conn->local, sizeof(bt_addr_le_t));
		info->le.remote = ser_decode_buffer(ctx, &conn->remote, sizeof(bt_addr_le_t));
#if defined(CONFIG_BT_USER_PHY_UPDATE)
		if (ser_decode_is_null(ctx)) {
			info->le.phy = NULL;
		} else {
			info->le.phy = &conn->phy;
			bt_conn_le_phy_info_dec(ctx, &conn->phy);
		}
#else
		ser_decode_skip(ctx);
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		if (ser_decode_is_null(ctx)) {
			info->le.data_len = NULL;
		} else {
			info->le.data_len = &conn->data_len;
			bt_conn_le_data_len_info_dec(ctx, &conn->data_len);
		}
#else
		ser_decode_skip(ctx);
#endif

		info->state = ser_decode_uint(ctx);

		UNLOCK_CONN_INFO();
	} else {
		/* non-LE connection types are not supported. */
		ser_decoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
	}
}

struct bt_conn_get_info_rpc_res {
	int result;
	struct bt_conn *conn;
	struct bt_conn_info *info;

};

static void bt_conn_get_info_rpc_rsp(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx,
				     void *handler_data)
{
	struct bt_conn_get_info_rpc_res *res =
		(struct bt_conn_get_info_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);

	bt_conn_info_dec(ctx, res->conn, res->info);
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_get_info_rpc_res result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	result.conn = (struct bt_conn *)conn;
	result.info = info;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_INFO_RPC_CMD,
				&ctx, bt_conn_get_info_rpc_rsp, &result);

	return result.result;
}

void bt_conn_remote_info_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn *conn,
			     struct bt_conn_remote_info *remote_info)
{
	remote_info->type = ser_decode_uint(ctx);
	remote_info->version = ser_decode_uint(ctx);
	remote_info->manufacturer = ser_decode_uint(ctx);
	remote_info->subversion = ser_decode_uint(ctx);

	if (remote_info->type == BT_CONN_TYPE_LE && conn != NULL) {
		LOCK_CONN_INFO();
		remote_info->le.features = ser_decode_buffer(ctx, &conn->features,
							     sizeof(conn->features));
		UNLOCK_CONN_INFO();
	} else {
		/* non-LE connection types are not supported. */
		ser_decoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
	}
}

struct bt_conn_get_remote_info_rpc_res {
	int result;
	struct bt_conn *conn;
	struct bt_conn_remote_info *remote_info;
};

static void bt_conn_get_remote_info_rpc_rsp(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx,
					    void *handler_data)
{
	struct bt_conn_get_remote_info_rpc_res *res =
		(struct bt_conn_get_remote_info_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);

	bt_conn_remote_info_dec(ctx, res->conn, res->remote_info);
}

int bt_conn_get_remote_info(struct bt_conn *conn,
			    struct bt_conn_remote_info *remote_info)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_get_remote_info_rpc_res result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	result.conn = conn;
	result.remote_info = remote_info;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_REMOTE_INFO_RPC_CMD,
				&ctx, bt_conn_get_remote_info_rpc_rsp, &result);

	return result.result;
}

void bt_le_conn_param_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_conn_param *data)
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

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 15;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_le_conn_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_PARAM_UPDATE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void bt_conn_le_data_len_param_enc(struct nrf_rpc_cbor_ctx *encoder,
				   const struct bt_conn_le_data_len_param *data)
{
	ser_encode_uint(encoder, data->tx_max_len);
	ser_encode_uint(encoder, data->tx_max_time);
}

int bt_conn_le_data_len_update(struct bt_conn *conn,
			       const struct bt_conn_le_data_len_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 9;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_conn_le_data_len_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
void bt_conn_le_phy_param_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_conn_le_phy_param *data)
{
	ser_encode_uint(encoder, data->options);
	ser_encode_uint(encoder, data->pref_tx_phy);
	ser_encode_uint(encoder, data->pref_rx_phy);
}

int bt_conn_le_phy_update(struct bt_conn *conn,
			  const struct bt_conn_le_phy_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 10;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_conn_le_phy_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_PHY_UPDATE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, reason);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_DISCONNECT_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_CENTRAL)
void bt_conn_le_create_param_enc(struct nrf_rpc_cbor_ctx *encoder,
				 const struct bt_conn_le_create_param *data)
{
	ser_encode_uint(encoder, data->options);
	ser_encode_uint(encoder, data->interval);
	ser_encode_uint(encoder, data->window);
	ser_encode_uint(encoder, data->interval_coded);
	ser_encode_uint(encoder, data->window_coded);
	ser_encode_uint(encoder, data->timeout);
}

struct bt_conn_le_create_rpc_res {
	int result;
	struct bt_conn **conn;

};

static void bt_conn_le_create_rpc_rsp(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx,
				      void *handler_data)
{
	struct bt_conn_le_create_rpc_res *res =
		(struct bt_conn_le_create_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	*(res->conn) = bt_rpc_decode_bt_conn(ctx);

	bt_conn_ref_local(*(res->conn));
}

int bt_conn_le_create(const bt_addr_le_t *peer,
		      const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param,
		      struct bt_conn **conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_le_create_rpc_res result;
	size_t buffer_size_max = 35;

	buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, peer, sizeof(bt_addr_le_t));
	bt_conn_le_create_param_enc(&ctx, create_param);
	bt_le_conn_param_enc(&ctx, conn_param);

	result.conn = conn;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_CREATE_RPC_CMD,
				&ctx, bt_conn_le_create_rpc_rsp, &result);

	return result.result;
}

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
int bt_conn_le_create_auto(const struct bt_conn_le_create_param *create_param,
			   const struct bt_le_conn_param *conn_param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 32;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_conn_le_create_param_enc(&ctx, create_param);
	bt_le_conn_param_enc(&ctx, conn_param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_CREATE_AUTO_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_create_auto_stop(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CREATE_AUTO_STOP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

#if !defined(CONFIG_BT_FILTER_ACCEPT_LIST)
int bt_le_set_auto_conn(const bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	buffer_size_max += (param == NULL) ? 1 : 12;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	if (param == NULL) {
		ser_encode_null(&ctx);
	} else {
		bt_le_conn_param_enc(&ctx, param);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SET_AUTO_CONN_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif  /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */
#endif  /* defined(CONFIG_BT_CENTRAL) */

#if defined(CONFIG_BT_SMP)
int bt_conn_set_security(struct bt_conn *conn, bt_security_t sec)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, (uint32_t)sec);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_SET_SECURITY_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


struct bt_conn_get_security_rpc_res {
	bt_security_t result;
};

static void bt_conn_get_security_rpc_rsp(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx,
					 void *handler_data)
{
	struct bt_conn_get_security_rpc_res *res =
		(struct bt_conn_get_security_rpc_res *)handler_data;

	res->result = (bt_security_t)ser_decode_uint(ctx);
}

bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn_get_security_rpc_res result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_SECURITY_RPC_CMD,
				&ctx, bt_conn_get_security_rpc_rsp, &result);

	return result.result;
}

uint8_t bt_conn_enc_key_size(const struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_ENC_KEY_SIZE_RPC_CMD,
				&ctx, ser_rsp_decode_u8, &result);

	return result;
}
#endif /* defined(CONFIG_BT_SMP) */

static struct bt_conn_cb *first_bt_conn_cb;

static void bt_conn_cb_connected_call(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->connected) {
			cb->connected(conn, err);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected) {
			cb->connected(conn, err);
		}
	}
}

static void bt_conn_cb_connected_call_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx,
						  void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;

	conn = bt_rpc_decode_bt_conn(ctx);
	err = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_connected_call(conn, err);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_CONNECTED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_connected_call, BT_CONN_CB_CONNECTED_CALL_RPC_CMD,
			 bt_conn_cb_connected_call_rpc_handler, NULL);

static void bt_conn_cb_disconnected_call(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->disconnected) {
			cb->disconnected(conn, reason);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected) {
			cb->disconnected(conn, reason);
		}
	}
}

static void bt_conn_cb_disconnected_call_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_conn *conn;
	uint8_t reason;

	conn = bt_rpc_decode_bt_conn(ctx);
	reason = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_disconnected_call(conn, reason);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_disconnected_call,
			 BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD,
			 bt_conn_cb_disconnected_call_rpc_handler, NULL);

static bool bt_conn_cb_le_param_req_call(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_param_req) {
			if (!cb->le_param_req(conn, param)) {
				return false;
			}
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_param_req) {
			if (!cb->le_param_req(conn, param)) {
				return false;
			}
		}
	}

	return true;
}

static void bt_conn_cb_le_param_req_call_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_conn *conn;
	struct bt_le_conn_param param;
	bool result;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_le_conn_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_conn_cb_le_param_req_call(conn, &param);

	ser_rsp_send_bool(group, result);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_param_req_call,
			 BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD,
			 bt_conn_cb_le_param_req_call_rpc_handler, NULL);

static void bt_conn_cb_le_param_updated_call(struct bt_conn *conn, uint16_t interval, uint16_t
					     latency, uint16_t timeout)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_param_updated) {
			cb->le_param_updated(conn, interval, latency, timeout);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_param_updated) {
			cb->le_param_updated(conn, interval, latency, timeout);
		}
	}
}

static void bt_conn_cb_le_param_updated_call_rpc_handler(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct bt_conn *conn;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;

	conn = bt_rpc_decode_bt_conn(ctx);
	interval = ser_decode_uint(ctx);
	latency = ser_decode_uint(ctx);
	timeout = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_le_param_updated_call(conn, interval, latency, timeout);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_param_updated_call,
			 BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD,
			 bt_conn_cb_le_param_updated_call_rpc_handler, NULL);

#if defined(CONFIG_BT_SMP)
static void bt_conn_cb_identity_resolved_call(struct bt_conn *conn, const bt_addr_le_t *rpa, const
					      bt_addr_le_t *identity)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, identity);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, identity);
		}
	}
}

static void bt_conn_cb_identity_resolved_call_rpc_handler(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{
	struct bt_conn *conn;
	bt_addr_le_t rpa_data;
	const bt_addr_le_t *rpa;
	bt_addr_le_t identity_data;
	const bt_addr_le_t *identity;

	conn = bt_rpc_decode_bt_conn(ctx);
	rpa = ser_decode_buffer(ctx, &rpa_data, sizeof(bt_addr_le_t));
	identity = ser_decode_buffer(ctx, &identity_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_identity_resolved_call(conn, rpa, identity);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_identity_resolved_call,
			 BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD,
			 bt_conn_cb_identity_resolved_call_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_SMP) */

#if defined(CONFIG_BT_SMP)
static void bt_conn_cb_security_changed_call(struct bt_conn *conn, bt_security_t level,
					     enum bt_security_err err)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->security_changed(conn, level, err);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->security_changed) {
			cb->security_changed(conn, level, err);
		}
	}
}

static void bt_conn_cb_security_changed_call_rpc_handler(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct bt_conn *conn;
	bt_security_t level;
	enum bt_security_err err;

	conn = bt_rpc_decode_bt_conn(ctx);
	level = (bt_security_t)ser_decode_uint(ctx);
	err = (enum bt_security_err)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_security_changed_call(conn, level, err);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_security_changed_call,
			 BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD,
			 bt_conn_cb_security_changed_call_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_SMP) */

#if defined(CONFIG_BT_REMOTE_INFO)
static void bt_conn_cb_remote_info_available_call(struct bt_conn *conn,
						  struct bt_conn_remote_info *remote_info)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->remote_info_available) {
			cb->remote_info_available(conn, remote_info);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->remote_info_available) {
			cb->remote_info_available(conn, remote_info);
		}
	}
}

static void bt_conn_cb_remote_info_available_call_rpc_handler(const struct nrf_rpc_group *group,
							      struct nrf_rpc_cbor_ctx *ctx,
							      void *handler_data)
{
	struct bt_conn *conn;

	struct bt_conn_remote_info remote_info;

	conn = bt_rpc_decode_bt_conn(ctx);

	bt_conn_remote_info_dec(ctx, conn, &remote_info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_remote_info_available_call(conn, &remote_info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_remote_info_available_call,
			 BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD,
			 bt_conn_cb_remote_info_available_call_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void bt_conn_cb_le_phy_updated_call(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_phy_updated) {
			cb->le_phy_updated(conn, param);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_phy_updated) {
			cb->le_phy_updated(conn, param);
		}
	}
}

static void bt_conn_cb_le_phy_updated_call_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_le_phy_info param;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_conn_le_phy_info_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_le_phy_updated_call(conn, &param);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_phy_updated_call,
			 BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD,
			 bt_conn_cb_le_phy_updated_call_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void bt_conn_cb_le_data_len_updated_call(struct bt_conn *conn,
						struct bt_conn_le_data_len_info *info)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_data_len_updated) {
			cb->le_data_len_updated(conn, info);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_data_len_updated) {
			cb->le_data_len_updated(conn, info);
		}
	}
}

static void bt_conn_cb_le_data_len_updated_call_rpc_handler(const struct nrf_rpc_group *group,
							    struct nrf_rpc_cbor_ctx *ctx,
							    void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_le_data_len_info info;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_conn_le_data_len_info_dec(ctx, &info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_conn_cb_le_data_len_updated_call(conn, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_data_len_updated_call,
			 BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD,
			 bt_conn_cb_le_data_len_updated_call_rpc_handler, NULL);

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

static void bt_conn_cb_register_on_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;
	static bool registered;

	if (!registered) {
		registered = true;

		NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

		nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_REGISTER_ON_REMOTE_RPC_CMD,
					&ctx, ser_rsp_decode_void, NULL);
	}
}

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
	LOCK_CONN_INFO();
	cb->_next = first_bt_conn_cb;
	first_bt_conn_cb = cb;
	UNLOCK_CONN_INFO();

	bt_conn_cb_register_on_remote();
}

#if defined(CONFIG_BT_SMP)
void bt_set_bondable(bool enable)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 1;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_bool(&ctx, enable);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_BONDABLE_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_oob_set_legacy_flag(bool enable)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 1;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_bool(&ctx, enable);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_LEGACY_FLAG_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_oob_set_sc_flag(bool enable)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 1;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_bool(&ctx, enable);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_SC_FLAG_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
int bt_le_oob_set_legacy_tk(struct bt_conn *conn, const uint8_t *tk)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t tk_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 13;

	tk_size = sizeof(uint8_t) * 16;
	buffer_size_max += tk_size;

	scratchpad_size += SCRATCHPAD_ALIGN(tk_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_buffer(&ctx, tk, tk_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_LEGACY_TK_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
size_t bt_le_oob_sc_data_buf_size(const struct bt_le_oob_sc_data *data)
{
	size_t buffer_size_max = 10;

	buffer_size_max += 16 * sizeof(uint8_t);
	buffer_size_max += 16 * sizeof(uint8_t);

	return buffer_size_max;
}

void bt_le_oob_sc_data_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_le_oob_sc_data *data)
{
	ser_encode_buffer(encoder, data->r, 16 * sizeof(uint8_t));
	ser_encode_buffer(encoder, data->c, 16 * sizeof(uint8_t));
}

void bt_le_oob_sc_data_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_oob_sc_data *data)
{
	ser_decode_buffer(ctx, data->r, 16 * sizeof(uint8_t));
	ser_decode_buffer(ctx, data->c, 16 * sizeof(uint8_t));
}

int bt_le_oob_set_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data *oobd_local,
			  const struct bt_le_oob_sc_data *oobd_remote)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	buffer_size_max += (oobd_local == NULL) ? 1 : 0 + bt_le_oob_sc_data_buf_size(oobd_local);
	buffer_size_max += (oobd_remote == NULL) ? 1 : 0 + bt_le_oob_sc_data_buf_size(oobd_remote);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	if (oobd_local == NULL) {
		ser_encode_null(&ctx);
	} else {
		bt_le_oob_sc_data_enc(&ctx, oobd_local);
	}
	if (oobd_remote == NULL) {
		ser_encode_null(&ctx);
	} else {
		bt_le_oob_sc_data_enc(&ctx, oobd_remote);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_SC_DATA_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

struct bt_le_oob_get_sc_data_rpc_res {
	int result;
	struct bt_conn *conn;
	const struct bt_le_oob_sc_data **oobd_local;
	const struct bt_le_oob_sc_data **oobd_remote;
};

static void bt_le_oob_get_sc_data_rpc_rsp(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_oob_get_sc_data_rpc_res *res =
		(struct bt_le_oob_get_sc_data_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);

	if (ser_decode_is_null(ctx)) {
		*res->oobd_local = NULL;
	} else {
		*res->oobd_local = &res->conn->oobd_local;
		bt_le_oob_sc_data_dec(ctx, &res->conn->oobd_local);
	}

	if (ser_decode_is_null(ctx)) {
		*res->oobd_remote = NULL;
	} else {
		*res->oobd_remote = &res->conn->oobd_remote;
		bt_le_oob_sc_data_dec(ctx, &res->conn->oobd_remote);
	}
}

int bt_le_oob_get_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data **oobd_local,
			  const struct bt_le_oob_sc_data **oobd_remote)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_le_oob_get_sc_data_rpc_res result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	result.conn = conn;
	result.oobd_local = oobd_local;
	result.oobd_remote = oobd_remote;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_GET_SC_DATA_RPC_CMD,
				&ctx, bt_le_oob_get_sc_data_rpc_rsp, &result);

	return result.result;
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

#if defined(CONFIG_BT_FIXED_PASSKEY)
int bt_passkey_set(unsigned int passkey)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, passkey);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_PASSKEY_SET_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_FIXED_PASSKEY) */

static const struct bt_conn_auth_cb *auth_cb;

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
void bt_conn_pairing_feat_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_conn_pairing_feat *data)
{
	data->io_capability = ser_decode_uint(ctx);
	data->oob_data_flag = ser_decode_uint(ctx);
	data->auth_req = ser_decode_uint(ctx);
	data->max_enc_key_size = ser_decode_uint(ctx);
	data->init_key_dist = ser_decode_uint(ctx);
	data->resp_key_dist = ser_decode_uint(ctx);
}

static void bt_rpc_auth_cb_pairing_accept_rpc_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	enum bt_security_err result;
	struct bt_conn *conn;
	struct bt_conn_pairing_feat feat;
	size_t buffer_size_max = 5;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_conn_pairing_feat_dec(ctx, &feat);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->pairing_accept) {
		result = auth_cb->pairing_accept(conn, &feat);
	} else {
		result = BT_SECURITY_ERR_INVALID_PARAM;
	}

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_uint(&ectx, (uint32_t)result);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_accept,
			 BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD,
			 bt_rpc_auth_cb_pairing_accept_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT) */

static void bt_rpc_auth_cb_passkey_display_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_conn *conn;
	unsigned int passkey;

	conn = bt_rpc_decode_bt_conn(ctx);
	passkey = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->passkey_display) {
		auth_cb->passkey_display(conn, passkey);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_display,
			 BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD,
			 bt_rpc_auth_cb_passkey_display_rpc_handler, NULL);

static void bt_rpc_auth_cb_passkey_entry_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_conn *conn;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->passkey_entry) {
		auth_cb->passkey_entry(conn);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_entry,
			 BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD,
			 bt_rpc_auth_cb_passkey_entry_rpc_handler, NULL);

static void bt_rpc_auth_cb_passkey_confirm_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_conn *conn;
	unsigned int passkey;

	conn = bt_rpc_decode_bt_conn(ctx);
	passkey = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->passkey_confirm) {
		auth_cb->passkey_confirm(conn, passkey);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_confirm,
			 BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD,
			 bt_rpc_auth_cb_passkey_confirm_rpc_handler, NULL);

static void bt_rpc_auth_cb_oob_data_request_rpc_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_oob_info info_data;
	struct bt_conn_oob_info *info;

	conn = bt_rpc_decode_bt_conn(ctx);
	info = ser_decode_buffer(ctx, &info_data, sizeof(struct bt_conn_oob_info));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->oob_data_request) {
		auth_cb->oob_data_request(conn, info);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_oob_data_request,
			 BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD,
			 bt_rpc_auth_cb_oob_data_request_rpc_handler, NULL);

static void bt_rpc_auth_cb_cancel_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->cancel) {
		auth_cb->cancel(conn);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_CANCEL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_cancel, BT_RPC_AUTH_CB_CANCEL_RPC_CMD,
			 bt_rpc_auth_cb_cancel_rpc_handler, NULL);

static void bt_rpc_auth_cb_pairing_confirm_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_conn *conn;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (auth_cb && auth_cb->pairing_confirm) {
		auth_cb->pairing_confirm(conn);
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_confirm,
			 BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD,
			 bt_rpc_auth_cb_pairing_confirm_rpc_handler, NULL);

int bt_conn_auth_cb_register_on_remote(uint16_t flags)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, flags);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	int res;
	uint16_t flags = 0;

	if (cb != NULL) {
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
		flags |= cb->pairing_accept ? FLAG_PAIRING_ACCEPT_PRESENT : 0;
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
		flags |= cb->passkey_display ? FLAG_PASSKEY_DISPLAY_PRESENT : 0;
		flags |= cb->passkey_entry ? FLAG_PASSKEY_ENTRY_PRESENT : 0;
		flags |= cb->passkey_confirm ? FLAG_PASSKEY_CONFIRM_PRESENT : 0;
		flags |= cb->oob_data_request ? FLAG_OOB_DATA_REQUEST_PRESENT : 0;
		flags |= cb->cancel ? FLAG_CANCEL_PRESENT : 0;
		flags |= cb->pairing_confirm ? FLAG_PAIRING_CONFIRM_PRESENT : 0;
	} else {
		flags = FLAG_AUTH_CB_IS_NULL;
	}

	res = bt_conn_auth_cb_register_on_remote(flags);

	if (res == 0) {
		auth_cb = cb;
	}

	return res;
}

sys_slist_t bt_auth_info_cbs = SYS_SLIST_STATIC_INIT(&bt_auth_info_cbs);

void bt_rpc_auth_info_cb_bond_deleted_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t id;
	bt_addr_le_t peer_data;
	const bt_addr_le_t *peer;
	struct bt_conn_auth_info_cb *info_cb;

	id = ser_decode_uint(ctx);
	peer = ser_decode_buffer(ctx, &peer_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_auth_info_cbs, info_cb, node) {
		if (info_cb->bond_deleted) {
			info_cb->bond_deleted(id, peer);
		}
	}

	ser_rsp_send_void(group);

decoding_error:
	report_decoding_error(BT_RPC_AUTH_INFO_CB_BOND_DELETED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_info_cb_bond_deleted,
			 BT_RPC_AUTH_INFO_CB_BOND_DELETED_RPC_CMD,
			 bt_rpc_auth_info_cb_bond_deleted_rpc_handler, NULL);

static void bt_rpc_auth_info_cb_pairing_complete_rpc_handler(const struct nrf_rpc_group *group,
							     struct nrf_rpc_cbor_ctx *ctx,
							     void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_auth_info_cb *info_cb;
	bool bonded;

	conn = bt_rpc_decode_bt_conn(ctx);
	bonded = ser_decode_bool(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_auth_info_cbs, info_cb, node) {
		if (info_cb->pairing_complete) {
			info_cb->pairing_complete(conn, bonded);
		}
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_INFO_CB_PAIRING_COMPLETE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_info_cb_pairing_complete,
			 BT_RPC_AUTH_INFO_CB_PAIRING_COMPLETE_RPC_CMD,
			 bt_rpc_auth_info_cb_pairing_complete_rpc_handler, NULL);

static void bt_rpc_auth_info_cb_pairing_failed_rpc_handler(const struct nrf_rpc_group *group,
							   struct nrf_rpc_cbor_ctx *ctx,
							   void *handler_data)
{
	struct bt_conn *conn;
	struct bt_conn_auth_info_cb *info_cb;
	enum bt_security_err reason;

	conn = bt_rpc_decode_bt_conn(ctx);
	reason = (enum bt_security_err)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&bt_auth_info_cbs, info_cb, node) {
		if (info_cb->pairing_failed) {
			info_cb->pairing_failed(conn, reason);
		}
	}

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_AUTH_INFO_CB_PAIRING_FAILED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_info_cb_pairing_failed,
			 BT_RPC_AUTH_INFO_CB_PAIRING_FAILED_RPC_CMD,
			 bt_rpc_auth_info_cb_pairing_failed_rpc_handler, NULL);

int bt_conn_auth_info_cb_register_on_remote(uint16_t flags)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, flags);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_INFO_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_info_cb_register(struct bt_conn_auth_info_cb *cb)
{
	int res;
	uint16_t flags = 0;
	bool register_on_remote = sys_slist_is_empty(&bt_auth_info_cbs);

	/* We need to register callback on the remote side only once. */
	if (!register_on_remote) {
		if (cb == NULL) {
			return -EINVAL;
		}

		sys_slist_append(&bt_auth_info_cbs, &cb->node);

		return 0;
	}

	if (cb != NULL) {
		flags |= cb->pairing_complete ? FLAG_PAIRING_COMPLETE_PRESENT : 0;
		flags |= cb->pairing_failed ? FLAG_PAIRING_FAILED_PRESENT : 0;
		flags |= cb->bond_deleted ? FLAG_BOND_DELETED_PRESENT : 0;
	} else {
		flags = FLAG_AUTH_CB_IS_NULL;
	}

	res = bt_conn_auth_info_cb_register_on_remote(flags);
	if (res == 0) {
		sys_slist_append(&bt_auth_info_cbs, &cb->node);
	}

	return res;
}

int bt_conn_auth_info_cb_unregister_on_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_INFO_CB_UNREGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_info_cb_unregister(struct bt_conn_auth_info_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&bt_auth_info_cbs, &cb->node)) {
		return -EALREADY;
	}

	/* If all callbacks are unregistered,
	 * we need also unregister callback on the remote side.
	 */
	if (sys_slist_is_empty(&bt_auth_info_cbs)) {
		return bt_conn_auth_info_cb_unregister_on_remote();
	}

	return 0;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, passkey);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_cancel(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_CANCEL_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_conn_auth_pairing_confirm(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#else /* defined(CONFIG_BT_SMP) */

bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return BT_SECURITY_L1;
}
#endif /* defined(CONFIG_BT_SMP) */

void bt_rpc_conn_init(void)
{
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		bt_conn_cb_register_on_remote();
		return;
	}
}
