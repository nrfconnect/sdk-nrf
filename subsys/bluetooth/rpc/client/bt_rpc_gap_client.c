/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Client side of bluetooth API over nRF RPC.
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/settings/settings.h>


#include "bt_rpc_gatt_client.h"
#include "bt_rpc_conn_client.h"
#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"
#include <nrf_rpc_cbor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

struct bt_rpc_get_check_list_rpc_res {
	size_t size;
	uint8_t *data;
};

static void bt_rpc_get_check_list_rpc_rsp(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx,
					  void *handler_data)
{
	struct bt_rpc_get_check_list_rpc_res *res =
		(struct bt_rpc_get_check_list_rpc_res *)handler_data;

	ser_decode_buffer(ctx, res->data, sizeof(uint8_t) * (res->size));
}

static void bt_rpc_get_check_list(uint8_t *data, size_t size)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_rpc_get_check_list_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, size);

	result.size = size;
	result.data = data;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GET_CHECK_LIST_RPC_CMD,
				&ctx, bt_rpc_get_check_list_rpc_rsp, &result);
}

static void validate_config(void)
{
	size_t size = bt_rpc_calc_check_list_size();
	uint8_t data[size];
	static bool validated;

	if (!validated) {
		validated = true;
		bt_rpc_get_check_list(data, size);
		if (!bt_rpc_validate_check_list(data, size)) {
			nrf_rpc_err(-EINVAL, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp,
				    BT_ENABLE_RPC_CMD, NRF_RPC_PACKET_TYPE_CMD);
		}
	}
}

static void bt_ready_cb_t_callback_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx,
					       void *handler_data)
{
	int err;
	bt_ready_cb_t callback_slot;

	err = ser_decode_int(ctx);
	callback_slot = (bt_ready_cb_t)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(err);

	return;
decoding_error:
	report_decoding_error(BT_READY_CB_T_CALLBACK_RPC_EVT, handler_data);
}

NRF_RPC_CBOR_EVT_DECODER(bt_rpc_grp, bt_ready_cb_t_callback, BT_READY_CB_T_CALLBACK_RPC_EVT,
			 bt_ready_cb_t_callback_rpc_handler, NULL);

int bt_enable(bt_ready_cb_t cb)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result = 0;
	size_t buffer_size_max = 5;
	static atomic_t init;

	validate_config();

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		bt_rpc_conn_init();
		result = bt_rpc_gatt_init();
	}

	if (result) {
		return result;
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_callback(&ctx, cb);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ENABLE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	/* In case if the Bluetooth was disabled, we don't need to init again
	 * dependencies.
	 */
	if ((!atomic_cas(&init, 0, 1)) || result) {
		return result;
	}


	if (IS_ENABLED(CONFIG_SETTINGS)) {
		int network_load = 1;

		result = settings_subsys_init();
		if (result) {
			return result;
		}

		result = settings_save_one("bt_rpc/network",
					   &network_load, sizeof(network_load));
	}

	return result;
}

int bt_disable(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_DISABLE_RPC_CMD, &ctx, ser_rsp_decode_i32, &result);

	return result;
}

bool bt_is_ready(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;
	bool result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_IS_READY_RPC_CMD, &ctx, ser_rsp_decode_bool,
				&result);

	return result;
}

int bt_set_name(const char *name)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	struct nrf_rpc_cbor_ctx ctx;
	size_t name_strlen;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	name_strlen = strlen(name);
	buffer_size_max += name_strlen;

	scratchpad_size += SCRATCHPAD_ALIGN(name_strlen + 1);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_str(&ctx, name, name_strlen);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_NAME_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
#else
	return -ENOMEM;

#endif /* defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) */
}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
struct bt_get_name_out_rpc_res {
	bool result;
	size_t size;
	char *name;
};

static void bt_get_name_out_rpc_rsp(const struct nrf_rpc_group *group,
				    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_get_name_out_rpc_res *res =
		(struct bt_get_name_out_rpc_res *)handler_data;

	res->result = ser_decode_bool(ctx);
	(void)ser_decode_str(ctx, res->name, (res->size));
}

static bool bt_get_name_out(char *name, size_t size)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_get_name_out_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	scratchpad_size += SCRATCHPAD_ALIGN(size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, size);

	result.size = size;
	result.name = name;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GET_NAME_OUT_RPC_CMD,
				&ctx, bt_get_name_out_rpc_rsp, &result);

	return result.result;
}
#endif /* defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) */

const char *bt_get_name(void)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)

	static char bt_name_cache[CONFIG_BT_DEVICE_NAME_MAX + 1];
	bool not_null;

	not_null = bt_get_name_out(bt_name_cache, sizeof(bt_name_cache));
	return not_null ? bt_name_cache : NULL;

#else
	return CONFIG_BT_DEVICE_NAME;

#endif /* defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) */
}

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
int bt_set_appearance(uint16_t new_appearance)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 3;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, new_appearance);

	nrf_rpc_cbor_cmd(&bt_rpc_grp, BT_SET_APPEARANCE_RPC_CMD, &ctx, ser_rsp_decode_u16,
			 &result);

	return result;
}

static uint16_t bt_get_appearance_from_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;
	uint16_t appearance;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd(&bt_rpc_grp, BT_GET_APPEARANCE_RPC_CMD, &ctx, ser_rsp_decode_u16,
			 &appearance);

	return appearance;
}
#endif /* defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC) */

uint16_t bt_get_appearance(void)
{
#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	return bt_get_appearance_from_remote();
#else
	return CONFIG_BT_DEVICE_APPEARANCE;
#endif /* CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC */
}

struct bt_id_get_rpc_res {
	size_t *count;
	bt_addr_le_t *addrs;
};

static void bt_id_get_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	struct bt_id_get_rpc_res *res =
		(struct bt_id_get_rpc_res *)handler_data;

	*(res->count) = ser_decode_uint(ctx);
	ser_decode_buffer(ctx, res->addrs, *(res->count) * sizeof(bt_addr_le_t));
}

void bt_id_get(bt_addr_le_t *addrs, size_t *count)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_id_get_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	scratchpad_size += SCRATCHPAD_ALIGN(*count * sizeof(bt_addr_le_t));

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, *count);

	result.count = count;
	result.addrs = addrs;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_GET_RPC_CMD,
				&ctx, bt_id_get_rpc_rsp, &result);
}

struct bt_id_create_rpc_res {
	int result;
	bt_addr_le_t *addr;
	uint8_t *irk;
};

static void bt_id_create_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	struct bt_id_create_rpc_res *res =
		(struct bt_id_create_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	ser_decode_buffer(ctx, res->addr, sizeof(bt_addr_le_t));
	ser_decode_buffer(ctx, res->irk, sizeof(uint8_t) * 16);
}

int bt_id_create(bt_addr_le_t *addr, uint8_t *irk)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t irk_size;
	struct bt_id_create_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 13;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	irk_size = !irk ? 0 : sizeof(uint8_t) * 16;
	buffer_size_max += irk_size;

	scratchpad_size += SCRATCHPAD_ALIGN(irk_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	ser_encode_buffer(&ctx, irk, irk_size);

	result.addr = addr;
	result.irk = irk;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_CREATE_RPC_CMD,
				&ctx, bt_id_create_rpc_rsp, &result);

	return result.result;
}

struct bt_id_reset_rpc_res {
	int result;
	bt_addr_le_t *addr;
	uint8_t *irk;
};

static void bt_id_reset_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	struct bt_id_reset_rpc_res *res =
		(struct bt_id_reset_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	ser_decode_buffer(ctx, res->addr, sizeof(bt_addr_le_t));
	ser_decode_buffer(ctx, res->irk, sizeof(uint8_t) * 16);
}

int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t irk_size;
	struct bt_id_reset_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 15;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	irk_size = !irk ? 0 : sizeof(uint8_t) * 16;
	buffer_size_max += irk_size;

	scratchpad_size += SCRATCHPAD_ALIGN(irk_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	ser_encode_buffer(&ctx, irk, irk_size);

	result.addr = addr;
	result.irk = irk;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_RESET_RPC_CMD,
				&ctx, bt_id_reset_rpc_rsp, &result);

	return result.result;
}

int bt_id_delete(uint8_t id)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 2;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_DELETE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


size_t bt_data_buf_size(const struct bt_data *data)
{
	size_t buffer_size_max = 9;

	buffer_size_max += sizeof(uint8_t) * data->data_len;

	return buffer_size_max;
}

size_t bt_data_sp_size(const struct bt_data *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * data->data_len);

	return scratchpad_size;
}

void bt_data_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_data *data)
{
	ser_encode_uint(encoder, data->type);
	ser_encode_uint(encoder, data->data_len);
	ser_encode_buffer(encoder, data->data, sizeof(uint8_t) * data->data_len);
}

void bt_le_scan_param_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_scan_param *data)
{
	ser_encode_uint(encoder, data->type);
	ser_encode_uint(encoder, data->options);
	ser_encode_uint(encoder, data->interval);
	ser_encode_uint(encoder, data->window);
	ser_encode_uint(encoder, data->timeout);
	ser_encode_uint(encoder, data->interval_coded);
	ser_encode_uint(encoder, data->window_coded);
}

void net_buf_simple_dec(struct ser_scratchpad *scratchpad, struct net_buf_simple *data)
{
	size_t len;

	data->data = ser_decode_buffer_into_scratchpad(scratchpad, &len);
	data->len = len;
	data->size = data->len;
	data->__buf = data->data;
}


static void bt_le_scan_cb_t_callback_rpc_handler(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	int8_t rssi;
	uint8_t adv_type;
	struct net_buf_simple buf;
	bt_le_scan_cb_t *callback_slot;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	rssi = ser_decode_int(ctx);
	adv_type = ser_decode_uint(ctx);
	net_buf_simple_dec(&scratchpad, &buf);
	callback_slot = (bt_le_scan_cb_t *)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(addr, rssi, adv_type, &buf);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_t_callback, BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,
			 bt_le_scan_cb_t_callback_rpc_handler, NULL);

size_t bt_le_adv_param_sp_size(const struct bt_le_adv_param *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += !data->peer ? 0 : SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_adv_param_buf_size(const struct bt_le_adv_param *data)
{
	size_t buffer_size_max = 22;

	buffer_size_max += !data->peer ? 0 : 2 + sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_adv_param_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_adv_param *data)
{
	ser_encode_uint(encoder, data->id);
	ser_encode_uint(encoder, data->sid);
	ser_encode_uint(encoder, data->secondary_max_skip);
	ser_encode_uint(encoder, data->options);
	ser_encode_uint(encoder, data->interval_min);
	ser_encode_uint(encoder, data->interval_max);
	ser_encode_buffer(encoder, data->peer, sizeof(bt_addr_le_t));
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 15;

	buffer_size_max += bt_le_adv_param_buf_size(param);

	for (size_t i = 0; i < ad_len; i++) {
		buffer_size_max += bt_data_buf_size(&ad[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&ad[i]);
	}

	for (size_t i = 0; i < sd_len; i++) {
		buffer_size_max += bt_data_buf_size(&sd[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&sd[i]);
	}

	scratchpad_size += bt_le_adv_param_sp_size(param);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	bt_le_adv_param_enc(&ctx, param);
	ser_encode_uint(&ctx, ad_len);

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_enc(&ctx, &ad[i]);
	}

	ser_encode_uint(&ctx, sd_len);

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_enc(&ctx, &sd[i]);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_START_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 15;

	for (size_t i = 0; i < ad_len; i++) {
		buffer_size_max += bt_data_buf_size(&ad[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&ad[i]);
	}
	for (size_t i = 0; i < sd_len; i++) {
		buffer_size_max += bt_data_buf_size(&sd[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&sd[i]);
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_uint(&ctx, ad_len);

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_enc(&ctx, &ad[i]);
	}

	ser_encode_uint(&ctx, sd_len);

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_enc(&ctx, &sd[i]);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_UPDATE_DATA_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_adv_stop(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_STOP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

void bt_le_oob_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_oob *data)
{
	ser_decode_buffer(ctx, &data->addr, sizeof(bt_addr_le_t));
	ser_decode_buffer(ctx, data->le_sc_data.r, 16 * sizeof(uint8_t));
	ser_decode_buffer(ctx, data->le_sc_data.c, 16 * sizeof(uint8_t));
}

#if defined(CONFIG_BT_EXT_ADV)
void bt_le_ext_adv_sent_info_dec(struct nrf_rpc_cbor_ctx *ctx,
	struct bt_le_ext_adv_sent_info *data)
{
	data->num_sent = ser_decode_uint(ctx);
}

void bt_le_ext_adv_scanned_info_dec(struct ser_scratchpad *scratchpad,
				    struct bt_le_ext_adv_scanned_info *data)
{
	data->addr = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
}


static void bt_le_ext_adv_cb_sent_callback_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_sent_info info;
	bt_le_ext_adv_cb_sent callback_slot;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_ext_adv_sent_info_dec(ctx, &info);
	callback_slot = (bt_le_ext_adv_cb_sent)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(adv, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_CB_SENT_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_cb_sent_callback,
			 BT_LE_EXT_ADV_CB_SENT_CALLBACK_RPC_CMD,
			 bt_le_ext_adv_cb_sent_callback_rpc_handler, NULL);

#if defined(CONFIG_BT_CONN)
void bt_le_ext_adv_connected_info_dec(struct nrf_rpc_cbor_ctx *ctx,
	struct bt_le_ext_adv_connected_info *data)
{
	data->conn = bt_rpc_decode_bt_conn(ctx);
}

static void bt_le_ext_adv_cb_connected_callback_rpc_handler(const struct nrf_rpc_group *group,
							    struct nrf_rpc_cbor_ctx *ctx,
							    void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_connected_info info;
	bt_le_ext_adv_cb_connected callback_slot;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_ext_adv_connected_info_dec(ctx, &info);
	callback_slot = (bt_le_ext_adv_cb_connected)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(adv, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_CB_CONNECTED_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_cb_connected_callback,
			 BT_LE_EXT_ADV_CB_CONNECTED_CALLBACK_RPC_CMD,
			 bt_le_ext_adv_cb_connected_callback_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_CONN) */

static void bt_le_ext_adv_cb_scanned_callback_rpc_handler(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_scanned_info info;
	bt_le_ext_adv_cb_scanned callback_slot;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_ext_adv_scanned_info_dec(&scratchpad, &info);
	callback_slot = (bt_le_ext_adv_cb_scanned)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(adv, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_CB_SCANNED_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_cb_scanned_callback,
			 BT_LE_EXT_ADV_CB_SCANNED_CALLBACK_RPC_CMD,
			 bt_le_ext_adv_cb_scanned_callback_rpc_handler, NULL);

static const size_t bt_le_ext_adv_cb_buf_size = 15;

void bt_le_ext_adv_cb_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_ext_adv_cb *data)
{
	ser_encode_callback(encoder, data->sent);
	ser_encode_callback(encoder, data->connected);
	ser_encode_callback(encoder, data->scanned);
}

struct bt_le_ext_adv_create_rpc_res {
	int result;
	struct bt_le_ext_adv **adv;
};

static void bt_le_ext_adv_create_rpc_rsp(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv_create_rpc_res *res =
		(struct bt_le_ext_adv_create_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	*(res->adv) = (struct bt_le_ext_adv *)(uintptr_t)ser_decode_uint(ctx);
}

int bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_le_ext_adv_create_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 5;

	buffer_size_max += bt_le_adv_param_buf_size(param);

	buffer_size_max += bt_le_ext_adv_cb_buf_size;

	scratchpad_size += bt_le_adv_param_sp_size(param);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	bt_le_adv_param_enc(&ctx, param);

	if (cb == NULL) {
		ser_encode_undefined(&ctx);
	} else {
		bt_le_ext_adv_cb_enc(&ctx, cb);
	}

	result.adv = adv;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CREATE_RPC_CMD,
				&ctx, bt_le_ext_adv_create_rpc_rsp, &result);

	return result.result;
}

void bt_le_ext_adv_start_param_enc(struct nrf_rpc_cbor_ctx *encoder,
				   const struct bt_le_ext_adv_start_param *data)
{
	ser_encode_uint(encoder, data->timeout);
	ser_encode_uint(encoder, data->num_events);
}

int bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_start_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 10;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_ext_adv_start_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_START_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


int bt_le_ext_adv_stop(struct bt_le_ext_adv *adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_STOP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


int bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 20;

	for (size_t i = 0; i < ad_len; i++) {
		buffer_size_max += bt_data_buf_size(&ad[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&ad[i]);
	}
	for (size_t i = 0; i < sd_len; i++) {
		buffer_size_max += bt_data_buf_size(&sd[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&sd[i]);
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	ser_encode_uint(&ctx, ad_len);

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_enc(&ctx, &ad[i]);
	}

	ser_encode_uint(&ctx, sd_len);

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_enc(&ctx, &sd[i]);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_SET_DATA_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_ext_adv_update_param(struct bt_le_ext_adv *adv,
			       const struct bt_le_adv_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	buffer_size_max += bt_le_adv_param_buf_size(param);

	scratchpad_size += bt_le_adv_param_sp_size(param);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_adv_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_ext_adv_delete(struct bt_le_ext_adv *adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_DELETE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

uint8_t bt_le_ext_adv_get_index(struct bt_le_ext_adv *adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_GET_INDEX_RPC_CMD,
				&ctx, ser_rsp_decode_u8, &result);

	return result;
}

void bt_le_ext_adv_info_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_ext_adv_info *data)
{
	ser_encode_uint(encoder, data->id);
	ser_encode_int(encoder, data->tx_power);
}


int bt_le_ext_adv_get_info(const struct bt_le_ext_adv *adv,
			   struct bt_le_ext_adv_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 9;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_ext_adv_info_enc(&ctx, info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_GET_INFO_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

struct bt_le_ext_adv_oob_get_local_rpc_res {
	int result;
	struct bt_le_oob *oob;
};

static void bt_le_ext_adv_oob_get_local_rpc_rsp(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv_oob_get_local_rpc_res *res =
		(struct bt_le_ext_adv_oob_get_local_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	bt_le_oob_dec(ctx, res->oob);
}

int bt_le_ext_adv_oob_get_local(struct bt_le_ext_adv *adv,
				struct bt_le_oob *oob)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_le_ext_adv_oob_get_local_rpc_res result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	result.oob = oob;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD,
				&ctx, bt_le_ext_adv_oob_get_local_rpc_rsp, &result);

	return result.result;
}
#endif /* defined(CONFIG_BT_EXT_ADV) */

#if defined(CONFIG_BT_PER_ADV)
void bt_le_per_adv_param_enc(struct nrf_rpc_cbor_ctx *encoder,
			     const struct bt_le_per_adv_param *data)
{
	ser_encode_uint(encoder, data->interval_min);
	ser_encode_uint(encoder, data->interval_max);
	ser_encode_uint(encoder, data->options);
}


int bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
			    const struct bt_le_per_adv_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 16;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_per_adv_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SET_PARAM_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 15;

	for (size_t i = 0; i < ad_len; i++) {
		buffer_size_max += bt_data_buf_size(&ad[i]);
		scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));
		scratchpad_size += bt_data_sp_size(&ad[i]);
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	ser_encode_uint(&ctx, ad_len);
	for (size_t i = 0; i < ad_len; i++) {
		bt_data_enc(&ctx, &ad[i]);
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SET_DATA_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_start(struct bt_le_ext_adv *adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_START_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_stop(struct bt_le_ext_adv *adv)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_STOP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_CONN)
int bt_le_per_adv_set_info_transfer(const struct bt_le_ext_adv *adv,
				    const struct bt_conn *conn,
				    uint16_t service_data)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 11;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, service_data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SET_INFO_TRANSFER_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif  /* defined(CONFIG_BT_CONN) || */
#endif  /* defined(CONFIG_BT_PER_ADV) */

#if defined(CONFIG_BT_PER_ADV_SYNC)
uint8_t bt_le_per_adv_sync_get_index(struct bt_le_per_adv_sync *per_adv_sync)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)per_adv_sync);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_GET_INDEX_RPC_CMD,
				&ctx, ser_rsp_decode_u8, &result);

	return result;
}

size_t bt_le_per_adv_sync_param_buf_size(const struct bt_le_per_adv_sync_param *data)
{
	size_t buffer_size_max = 16;

	buffer_size_max += sizeof(bt_addr_le_t);
	return buffer_size_max;

}

void bt_le_per_adv_sync_param_enc(struct nrf_rpc_cbor_ctx *encoder,
				  const struct bt_le_per_adv_sync_param *data)
{
	ser_encode_buffer(encoder, &data->addr, sizeof(bt_addr_le_t));
	ser_encode_uint(encoder, data->sid);
	ser_encode_uint(encoder, data->options);
	ser_encode_uint(encoder, data->skip);
	ser_encode_uint(encoder, data->timeout);
}

struct bt_le_per_adv_sync_create_rpc_res {
	int result;
	struct bt_le_per_adv_sync **out_sync;
};

static void bt_le_per_adv_sync_create_rpc_rsp(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_per_adv_sync_create_rpc_res *res =
		(struct bt_le_per_adv_sync_create_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	*(res->out_sync) = (struct bt_le_per_adv_sync *)(uintptr_t)ser_decode_uint(ctx);
}

int bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
			      struct bt_le_per_adv_sync **out_sync)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_le_per_adv_sync_create_rpc_res result;
	size_t buffer_size_max = 0;

	buffer_size_max += bt_le_per_adv_sync_param_buf_size(param);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_le_per_adv_sync_param_enc(&ctx, param);

	result.out_sync = out_sync;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_CREATE_RPC_CMD,
				&ctx, bt_le_per_adv_sync_create_rpc_rsp, &result);

	return result.result;
}

int bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)per_adv_sync);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_DELETE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static sys_slist_t pa_sync_cbs = SYS_SLIST_STATIC_INIT(&pa_sync_cbs);

void bt_le_per_adv_sync_cb_register_on_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb)
{
	bool register_on_remote;

	register_on_remote = sys_slist_is_empty(&pa_sync_cbs);

	sys_slist_append(&pa_sync_cbs, &cb->node);

	if (register_on_remote) {
		bt_le_per_adv_sync_cb_register_on_remote();
	}
}

int bt_le_per_adv_sync_recv_enable(struct bt_le_per_adv_sync *per_adv_sync)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)per_adv_sync);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_RECV_ENABLE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_sync_recv_disable(struct bt_le_per_adv_sync *per_adv_sync)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)per_adv_sync);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_RECV_DISABLE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_CONN)
int bt_le_per_adv_sync_transfer(const struct bt_le_per_adv_sync *per_adv_sync,
				const struct bt_conn *conn,
				uint16_t service_data)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 11;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)per_adv_sync);
	bt_rpc_encode_bt_conn(&ctx, conn);
	ser_encode_uint(&ctx, service_data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_TRANSFER_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

void bt_le_per_adv_sync_transfer_param_enc(struct nrf_rpc_cbor_ctx *encoder,
					   const struct bt_le_per_adv_sync_transfer_param *data)
{
	ser_encode_uint(encoder, data->skip);
	ser_encode_uint(encoder, data->timeout);
	ser_encode_uint(encoder, data->options);
}

int bt_le_per_adv_sync_transfer_subscribe(
	const struct bt_conn *conn,
	const struct bt_le_per_adv_sync_transfer_param *param)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 14;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_le_per_adv_sync_transfer_param_enc(&ctx, param);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_TRANSFER_SUBSCRIBE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_sync_transfer_unsubscribe(const struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_SYNC_TRANSFER_UNSUBSCRIBE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_CONN) */

int bt_le_per_adv_list_add(const bt_addr_le_t *addr, uint8_t sid)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	ser_encode_uint(&ctx, sid);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_LIST_ADD_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_list_remove(const bt_addr_le_t *addr, uint8_t sid)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	ser_encode_uint(&ctx, sid);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_LIST_REMOVE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_per_adv_list_clear(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_PER_ADV_LIST_CLEAR_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

void bt_le_per_adv_sync_synced_info_dec(struct ser_scratchpad *scratchpad,
					struct bt_le_per_adv_sync_synced_info *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->addr = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->sid = ser_decode_uint(ctx);
	data->interval = ser_decode_uint(ctx);
	data->phy = ser_decode_uint(ctx);
	data->recv_enabled = ser_decode_bool(ctx);
	data->service_data = ser_decode_uint(ctx);
#if defined(CONFIG_BT_CONN)
	data->conn = bt_rpc_decode_bt_conn(ctx);
#else
	data->conn = 0;
#endif /* defined(CONFIG_BT_CONN) */
}

static void per_adv_sync_cb_synced(struct bt_le_per_adv_sync *sync,
				   struct bt_le_per_adv_sync_synced_info *info)
{
	struct bt_le_per_adv_sync_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->synced) {
			listener->synced(sync, info);
		}
	}
}

static void per_adv_sync_cb_synced_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_per_adv_sync *sync;
	struct bt_le_per_adv_sync_synced_info info;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);
	bt_le_per_adv_sync_synced_info_dec(&scratchpad, &info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	per_adv_sync_cb_synced(sync, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(PER_ADV_SYNC_CB_SYNCED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, per_adv_sync_cb_synced, PER_ADV_SYNC_CB_SYNCED_RPC_CMD,
			 per_adv_sync_cb_synced_rpc_handler, NULL);

void bt_le_per_adv_sync_term_info_dec(struct ser_scratchpad *scratchpad,
				      struct bt_le_per_adv_sync_term_info *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->addr = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->sid = ser_decode_uint(ctx);
}

void bt_le_per_adv_sync_recv_info_dec(struct ser_scratchpad *scratchpad,
				      struct bt_le_per_adv_sync_recv_info *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->addr = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->sid = ser_decode_uint(ctx);
	data->tx_power = ser_decode_int(ctx);
	data->rssi = ser_decode_int(ctx);
	data->cte_type = ser_decode_uint(ctx);
}

void bt_le_per_adv_sync_state_info_dec(struct nrf_rpc_cbor_ctx *ctx,
				       struct bt_le_per_adv_sync_state_info *data)
{
	data->recv_enabled = ser_decode_bool(ctx);
}

void per_adv_sync_cb_term(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_term_info *info)
{
	struct bt_le_per_adv_sync_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->term) {
			listener->term(sync, info);
		}
	}
}

static void per_adv_sync_cb_term_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_per_adv_sync *sync;
	struct bt_le_per_adv_sync_term_info info;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);
	bt_le_per_adv_sync_term_info_dec(&scratchpad, &info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	per_adv_sync_cb_term(sync, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(PER_ADV_SYNC_CB_TERM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, per_adv_sync_cb_term, PER_ADV_SYNC_CB_TERM_RPC_CMD,
			 per_adv_sync_cb_term_rpc_handler, NULL);

void per_adv_sync_cb_recv(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_recv_info *info,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple_state state;
	struct bt_le_per_adv_sync_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->recv) {
			net_buf_simple_save(buf, &state);
			listener->recv(sync, info, buf);
			net_buf_simple_restore(buf, &state);
		}
	}
}

static void per_adv_sync_cb_recv_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_per_adv_sync *sync;
	struct bt_le_per_adv_sync_recv_info info;
	struct net_buf_simple buf;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);
	bt_le_per_adv_sync_recv_info_dec(&scratchpad, &info);
	net_buf_simple_dec(&scratchpad, &buf);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	per_adv_sync_cb_recv(sync, &info, &buf);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(PER_ADV_SYNC_CB_RECV_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, per_adv_sync_cb_recv, PER_ADV_SYNC_CB_RECV_RPC_CMD,
			 per_adv_sync_cb_recv_rpc_handler, NULL);

void per_adv_sync_cb_state_changed(struct bt_le_per_adv_sync *sync,
				   const struct bt_le_per_adv_sync_state_info *info)
{
	struct bt_le_per_adv_sync_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->state_changed) {
			listener->state_changed(sync, info);
		}
	}
}

static void per_adv_sync_cb_state_changed_rpc_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	struct bt_le_per_adv_sync *sync;
	struct bt_le_per_adv_sync_state_info info;

	sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);
	bt_le_per_adv_sync_state_info_dec(ctx, &info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	per_adv_sync_cb_state_changed(sync, &info);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(PER_ADV_SYNC_CB_STATE_CHANGED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, per_adv_sync_cb_state_changed,
			 PER_ADV_SYNC_CB_STATE_CHANGED_RPC_CMD,
			 per_adv_sync_cb_state_changed_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */

#if defined(CONFIG_BT_OBSERVER)
int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 27;

	result = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_le_scan_param_enc(&ctx, param);
	ser_encode_callback(&ctx, cb);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_START_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_le_scan_stop(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_STOP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

void bt_le_scan_recv_info_dec(struct ser_scratchpad *scratchpad,
			      struct bt_le_scan_recv_info *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->addr = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->sid = ser_decode_uint(ctx);
	data->rssi = ser_decode_int(ctx);
	data->tx_power = ser_decode_int(ctx);
	data->adv_type = ser_decode_uint(ctx);
	data->adv_props = ser_decode_uint(ctx);
	data->interval = ser_decode_uint(ctx);
	data->primary_phy = ser_decode_uint(ctx);
	data->secondary_phy = ser_decode_uint(ctx);
}


static sys_slist_t scan_cbs = SYS_SLIST_STATIC_INIT(&scan_cbs);

static void bt_le_scan_cb_recv(const struct bt_le_scan_recv_info *info,
			       struct net_buf_simple *buf)
{
	struct bt_le_scan_cb *listener;
	struct net_buf_simple_state state;

	SYS_SLIST_FOR_EACH_CONTAINER(&scan_cbs, listener, node) {
		net_buf_simple_save(buf, &state);
		listener->recv(info, buf);
		net_buf_simple_restore(buf, &state);
	}
}

static void bt_le_scan_cb_recv_rpc_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_scan_recv_info info;
	struct net_buf_simple buf;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	bt_le_scan_recv_info_dec(&scratchpad, &info);
	net_buf_simple_dec(&scratchpad, &buf);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_le_scan_cb_recv(&info, &buf);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_LE_SCAN_CB_RECV_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_recv, BT_LE_SCAN_CB_RECV_RPC_CMD,
			 bt_le_scan_cb_recv_rpc_handler, NULL);

static void bt_le_scan_cb_timeout(void)
{
	struct bt_le_scan_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&scan_cbs, listener, node) {
		listener->timeout();
	}
}

static void bt_le_scan_cb_timeout_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	bt_le_scan_cb_timeout();

	ser_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_timeout, BT_LE_SCAN_CB_TIMEOUT_RPC_CMD,
			 bt_le_scan_cb_timeout_rpc_handler, NULL);

static void bt_le_scan_cb_register_on_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_scan_cb_register(struct bt_le_scan_cb *cb)
{
	bool register_on_remote;

	register_on_remote = sys_slist_is_empty(&scan_cbs);

	sys_slist_append(&scan_cbs, &cb->node);

	if (register_on_remote) {
		bt_le_scan_cb_register_on_remote();
	}
}
#endif /* defined(CONFIG_BT_OBSERVER) */

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
int bt_le_filter_accept_list_add(const bt_addr_le_t *addr)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_FILTER_ACCEPT_LIST_ADD_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


int bt_le_filter_accept_list_remove(const bt_addr_le_t *addr)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_FILTER_ACCEPT_LIST_REMOVE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}


int bt_le_filter_accept_list_clear(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ACCEPT_LIST_CLEAR_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

int bt_le_set_chan_map(uint8_t chan_map[5])
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t chan_map_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	chan_map_size = sizeof(uint8_t) * 5;
	buffer_size_max += chan_map_size;

	scratchpad_size += SCRATCHPAD_ALIGN(chan_map_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_buffer(&ctx, chan_map, chan_map_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SET_CHAN_MAP_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data)
{
	while (ad->len > 1) {
		struct bt_data data;
		uint8_t len;

		len = net_buf_simple_pull_u8(ad);
		if (len == 0U) {
			/* Early termination */
			return;
		}

		if (len > ad->len) {
			LOG_WRN("Malformed data");
			return;
		}

		data.type = net_buf_simple_pull_u8(ad);
		data.data_len = len - 1;
		data.data = ad->data;

		if (!func(&data, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

struct bt_le_oob_get_local_rpc_res {
	int result;
	struct bt_le_oob *oob;
};

static void bt_le_oob_get_local_rpc_rsp(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_oob_get_local_rpc_res *res =
		(struct bt_le_oob_get_local_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	bt_le_oob_dec(ctx, res->oob);
}

int bt_le_oob_get_local(uint8_t id, struct bt_le_oob *oob)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_le_oob_get_local_rpc_res result;
	size_t buffer_size_max = 2;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);

	result.oob = oob;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_GET_LOCAL_RPC_CMD,
				&ctx, bt_le_oob_get_local_rpc_rsp, &result);

	return result.result;
}

int bt_addr_from_str(const char *str, bt_addr_t *addr)
{
	int i, j;
	uint8_t tmp;

	if (strlen(str) != 17U) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

int bt_addr_le_from_str(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = bt_addr_from_str(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else if (!strcmp(type, "public-id") || !strcmp(type, "(public-id)")) {
		addr->type = BT_ADDR_LE_PUBLIC_ID;
	} else if (!strcmp(type, "random-id") || !strcmp(type, "(random-id)")) {
		addr->type = BT_ADDR_LE_RANDOM_ID;
	} else {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_CONN)
int bt_unpair(uint8_t id, const bt_addr_le_t *addr)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_UNPAIR_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}
#endif /* defined(CONFIG_BT_CONN) */

#if (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP))
void bt_bond_info_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_bond_info *data)
{
	ser_decode_buffer(ctx, &data->addr, sizeof(bt_addr_le_t));
}

static void bt_foreach_bond_cb_callback_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	struct bt_bond_info info;
	void *user_data;
	bt_foreach_bond_cb callback_slot;

	bt_bond_info_dec(ctx, &info);
	user_data = (void *)ser_decode_uint(ctx);
	callback_slot = (bt_foreach_bond_cb)ser_decode_callback_call(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(&info, user_data);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_FOREACH_BOND_CB_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_foreach_bond_cb_callback,
			 BT_FOREACH_BOND_CB_CALLBACK_RPC_CMD,
			 bt_foreach_bond_cb_callback_rpc_handler, NULL);

void bt_foreach_bond(uint8_t id, void (*func)(const struct bt_bond_info *info,
					      void *user_data),
		     void *user_data)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 12;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_callback(&ctx, func);
	ser_encode_uint(&ctx, (uintptr_t)user_data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_FOREACH_BOND_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}
#endif /* (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP)) */

static int rpc_settings_set(const char *key, size_t len_rd, settings_read_cb read_cb,
				    void *cb_arg)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;
	ssize_t len;
	const char *next;

	if (!key) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	len = settings_name_next(key, &next);

	if (!strncmp(key, "network", len)) {
		NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

		nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SETTINGS_LOAD_RPC_CMD,
					&ctx, ser_rsp_decode_void, NULL);

	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_gatt_rpc, "bt_rpc", NULL, rpc_settings_set,
			       NULL, NULL);
