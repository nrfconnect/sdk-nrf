/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/kernel.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"
#include <zephyr/settings/settings.h>

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static void bt_rpc_get_check_list_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	size_t size;
	uint8_t *data;
	size_t buffer_size_max = 5;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	size = ser_decode_uint(ctx);
	data = ser_scratchpad_add(&scratchpad, sizeof(uint8_t) * size);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_rpc_get_check_list(data, size);

	buffer_size_max += sizeof(uint8_t) * size;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_buffer(&ectx, data, sizeof(uint8_t) * size);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_RPC_GET_CHECK_LIST_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_get_check_list, BT_RPC_GET_CHECK_LIST_RPC_CMD,
			 bt_rpc_get_check_list_rpc_handler, NULL);


static inline void bt_ready_cb_t_callback(int err,
					  uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_int(&ctx, err);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_evt_no_err(&bt_rpc_grp,
				BT_READY_CB_T_CALLBACK_RPC_EVT, &ctx);
}

CBKPROXY_HANDLER(bt_ready_cb_t_encoder, bt_ready_cb_t_callback, (int err), (err));

static void bt_enable_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	bt_ready_cb_t cb;
	int result;

	cb = (bt_ready_cb_t)ser_decode_callback(ctx, bt_ready_cb_t_encoder);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_enable(cb);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_ENABLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_enable, BT_ENABLE_RPC_CMD,
			 bt_enable_rpc_handler, NULL);

static void bt_disable_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	int result;

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_disable();

	ser_rsp_send_int(group, result);

	return;

decoding_error:
	report_decoding_error(BT_DISABLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_disable, BT_DISABLE_RPC_CMD,
			 bt_disable_rpc_handler, NULL);

static void bt_is_ready_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	bool result;

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_is_ready();

	ser_rsp_send_bool(group, result);

	return;

decoding_error:
	report_decoding_error(BT_IS_READY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_is_ready, BT_IS_READY_RPC_CMD,
			 bt_is_ready_rpc_handler, NULL);

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)

static void bt_set_name_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	const char *name;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	name = ser_decode_str_into_scratchpad(&scratchpad, NULL);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_set_name(name);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_SET_NAME_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_name, BT_SET_NAME_RPC_CMD,
			 bt_set_name_rpc_handler, NULL);

static bool bt_get_name_out(char *name, size_t size)
{
	const char *src;

	src = bt_get_name();

	if (!src) {
		strcpy(name, "");
		return false;
	}

	strncpy(name, src, size);
	return true;
}

static void bt_get_name_out_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool result;
	size_t size;
	size_t name_strlen;
	char *name;
	size_t buffer_size_max = 6;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	size = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	name = ser_scratchpad_add(&scratchpad, size);

	result = bt_get_name_out(name, size);

	name_strlen = strlen(name);
	buffer_size_max += name_strlen;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_bool(&ectx, result);
		ser_encode_str(&ectx, name, name_strlen);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_GET_NAME_OUT_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_get_name_out, BT_GET_NAME_OUT_RPC_CMD,
			 bt_get_name_out_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) */

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
void bt_get_appearance_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	uint16_t appearance;

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	appearance = bt_get_appearance();

	ser_rsp_send_uint(group, appearance);

	return;

decoding_error:
	report_decoding_error(BT_GET_APPEARANCE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_get_appearance, BT_GET_APPEARANCE_RPC_CMD,
			 bt_get_appearance_rpc_handler, NULL);

void bt_set_appearance_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	int result;
	uint16_t appearance;

	appearance = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_set_appearance(appearance);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_SET_APPEARANCE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_appearance, BT_SET_APPEARANCE_RPC_CMD,
			 bt_set_appearance_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC) */

static void bt_id_get_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	size_t count_data;
	size_t *count = &count_data;
	bt_addr_le_t *addrs;
	size_t buffer_size_max = 10;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	*count = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	addrs = ser_scratchpad_add(&scratchpad, *count * sizeof(bt_addr_le_t));

	bt_id_get(addrs, count);

	buffer_size_max += *count * sizeof(bt_addr_le_t);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_uint(&ectx, *count);
		ser_encode_buffer(&ectx, addrs, *count * sizeof(bt_addr_le_t));

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_ID_GET_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_get, BT_ID_GET_RPC_CMD,
			 bt_id_get_rpc_handler, NULL);

static void bt_id_create_rpc_handler(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	bt_addr_le_t addr_data;
	bt_addr_le_t *addr;
	uint8_t *irk;
	size_t buffer_size_max = 13;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	irk = ser_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_id_create(addr, irk);

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	buffer_size_max += !irk ? 0 : sizeof(uint8_t) * 16;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		ser_encode_buffer(&ectx, addr, sizeof(bt_addr_le_t));
		ser_encode_buffer(&ectx, irk, sizeof(uint8_t) * 16);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_ID_CREATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_create, BT_ID_CREATE_RPC_CMD,
			 bt_id_create_rpc_handler, NULL);

static void bt_id_reset_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	int result;
	uint8_t id;
	bt_addr_le_t addr_data;
	bt_addr_le_t *addr;
	uint8_t *irk;
	size_t buffer_size_max = 13;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	id = ser_decode_uint(ctx);
	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	irk = ser_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_id_reset(id, addr, irk);

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	buffer_size_max += !irk ? 0 : sizeof(uint8_t) * 16;

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		ser_encode_buffer(&ectx, addr, sizeof(bt_addr_le_t));
		ser_encode_buffer(&ectx, irk, sizeof(uint8_t) * 16);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_ID_RESET_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_reset, BT_ID_RESET_RPC_CMD,
			 bt_id_reset_rpc_handler, NULL);

static void bt_id_delete_rpc_handler(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t id;
	int result;

	id = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_id_delete(id);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_ID_DELETE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_delete, BT_ID_DELETE_RPC_CMD,
			 bt_id_delete_rpc_handler, NULL);

void bt_data_dec(struct ser_scratchpad *scratchpad, struct bt_data *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->type = ser_decode_uint(ctx);
	data->data_len = ser_decode_uint(ctx);
	data->data = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
}

void bt_le_scan_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_scan_param *data)
{
	data->type = ser_decode_uint(ctx);
	data->options = ser_decode_uint(ctx);
	data->interval = ser_decode_uint(ctx);
	data->window = ser_decode_uint(ctx);
	data->timeout = ser_decode_uint(ctx);
	data->interval_coded = ser_decode_uint(ctx);
	data->window_coded = ser_decode_uint(ctx);
}


size_t net_buf_simple_sp_size(struct net_buf_simple *data)
{
	return SCRATCHPAD_ALIGN(data->len);
}

size_t net_buf_simple_buf_size(struct net_buf_simple *data)
{
	return 3 + data->len;
}

void net_buf_simple_enc(struct nrf_rpc_cbor_ctx *encoder, struct net_buf_simple *data)
{
	ser_encode_buffer(encoder, data->data, data->len);
}


static inline void bt_le_scan_cb_t_callback(const bt_addr_le_t *addr,
					    int8_t rssi, uint8_t adv_type,
					    struct net_buf_simple *buf,
					    uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 15;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;
	buffer_size_max += net_buf_simple_buf_size(buf);

	scratchpad_size += net_buf_simple_sp_size(buf);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	ser_encode_int(&ctx, rssi);
	ser_encode_uint(&ctx, adv_type);
	net_buf_simple_enc(&ctx, buf);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}


CBKPROXY_HANDLER(bt_le_scan_cb_t_encoder, bt_le_scan_cb_t_callback,
		 (const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		  struct net_buf_simple *buf), (addr, rssi, adv_type, buf));

#if defined(CONFIG_BT_BROADCASTER)

void bt_le_adv_param_dec(struct ser_scratchpad *scratchpad, struct bt_le_adv_param *data)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->id = ser_decode_uint(ctx);
	data->sid = ser_decode_uint(ctx);
	data->secondary_max_skip = ser_decode_uint(ctx);
	data->options = ser_decode_uint(ctx);
	data->interval_min = ser_decode_uint(ctx);
	data->interval_max = ser_decode_uint(ctx);
	data->peer = ser_decode_buffer_into_scratchpad(scratchpad, NULL);
}

static void bt_le_adv_start_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_adv_param param;
	size_t ad_len;
	struct bt_data *ad;
	size_t sd_len;
	struct bt_data *sd;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	bt_le_adv_param_dec(&scratchpad, &param);
	ad_len = ser_decode_uint(ctx);
	ad = ser_scratchpad_add(&scratchpad, ad_len * sizeof(struct bt_data));
	if (ad == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_dec(&scratchpad, &ad[i]);
	}

	sd_len = ser_decode_uint(ctx);
	sd = ser_scratchpad_add(&scratchpad, sd_len * sizeof(struct bt_data));
	if (sd == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_dec(&scratchpad, &sd[i]);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_adv_start(&param, ad, ad_len, sd, sd_len);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_ADV_START_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_start, BT_LE_ADV_START_RPC_CMD,
			 bt_le_adv_start_rpc_handler, NULL);

static void bt_le_adv_update_data_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	size_t ad_len;
	struct bt_data *ad;
	size_t sd_len;
	struct bt_data *sd;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	ad_len = ser_decode_uint(ctx);
	ad = ser_scratchpad_add(&scratchpad, ad_len * sizeof(struct bt_data));
	if (ad == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_dec(&scratchpad, &ad[i]);
	}
	sd_len = ser_decode_uint(ctx);
	sd = ser_scratchpad_add(&scratchpad, sd_len * sizeof(struct bt_data));
	if (sd == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_dec(&scratchpad, &sd[i]);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_adv_update_data(ad, ad_len, sd, sd_len);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_ADV_UPDATE_DATA_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_update_data, BT_LE_ADV_UPDATE_DATA_RPC_CMD,
			 bt_le_adv_update_data_rpc_handler, NULL);

static void bt_le_adv_stop_rpc_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_le_adv_stop();

	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_stop, BT_LE_ADV_STOP_RPC_CMD,
			 bt_le_adv_stop_rpc_handler, NULL);

#endif /* defined(CONFIG_BT_BROADCASTER) */

size_t bt_le_oob_buf_size(const struct bt_le_oob *data)
{
	size_t buffer_size_max = 13;

	buffer_size_max += sizeof(bt_addr_le_t);
	buffer_size_max += 16 * sizeof(uint8_t);
	buffer_size_max += 16 * sizeof(uint8_t);
	return buffer_size_max;

}

void bt_le_oob_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_le_oob *data)
{
	ser_encode_buffer(encoder, &data->addr, sizeof(bt_addr_le_t));
	ser_encode_buffer(encoder, data->le_sc_data.r, 16 * sizeof(uint8_t));
	ser_encode_buffer(encoder, data->le_sc_data.c, 16 * sizeof(uint8_t));
}

#if defined(CONFIG_BT_EXT_ADV)

K_MEM_SLAB_DEFINE(bt_rpc_ext_adv_cb_cache,
		  sizeof(struct bt_le_ext_adv_cb),
		  CONFIG_BT_EXT_ADV_MAX_ADV_SET,
		  sizeof(void *));
static struct bt_le_ext_adv_cb *ext_adv_cb_cache_map[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

void bt_le_ext_adv_sent_info_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_le_ext_adv_sent_info *data)
{
	ser_encode_uint(encoder, data->num_sent);
}

void bt_le_ext_adv_connected_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				      const struct bt_le_ext_adv_connected_info *data)
{
	bt_rpc_encode_bt_conn(encoder, data->conn);
}

size_t bt_le_ext_adv_scanned_info_sp_size(const struct bt_le_ext_adv_scanned_info *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_ext_adv_scanned_info_buf_size(const struct bt_le_ext_adv_scanned_info *data)
{
	size_t buffer_size_max = 3;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_ext_adv_scanned_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				    const struct bt_le_ext_adv_scanned_info *data)
{
	ser_encode_buffer(encoder, data->addr, sizeof(bt_addr_le_t));
}

static inline
void bt_le_ext_adv_cb_sent_callback(struct bt_le_ext_adv *adv,
				    struct bt_le_ext_adv_sent_info *info,
				    uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 10;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_ext_adv_sent_info_enc(&ctx, info);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_SENT_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_le_ext_adv_cb_sent_encoder, bt_le_ext_adv_cb_sent_callback,
		 (struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info),
		 (adv, info));

static inline
void bt_le_ext_adv_cb_connected_callback(struct bt_le_ext_adv *adv,
					 struct bt_le_ext_adv_connected_info *info,
					 uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 11;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_ext_adv_connected_info_enc(&ctx, info);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_CONNECTED_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_le_ext_adv_cb_connected_encoder,
		 bt_le_ext_adv_cb_connected_callback,
		 (struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info),
		 (adv, info));

static inline
void bt_le_ext_adv_cb_scanned_callback(struct bt_le_ext_adv *adv,
				       struct bt_le_ext_adv_scanned_info *info,
				       uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 13;

	buffer_size_max += bt_le_ext_adv_scanned_info_buf_size(info);

	scratchpad_size += bt_le_ext_adv_scanned_info_sp_size(info);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)adv);
	bt_le_ext_adv_scanned_info_enc(&ctx, info);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_SCANNED_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_le_ext_adv_cb_scanned_encoder,
		 bt_le_ext_adv_cb_scanned_callback,
		 (struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info),
		 (adv, info));

void bt_le_ext_adv_cb_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_ext_adv_cb *data)
{
	data->sent = (bt_le_ext_adv_cb_sent)ser_decode_callback(ctx,
								 bt_le_ext_adv_cb_sent_encoder);
	data->connected = (bt_le_ext_adv_cb_connected)ser_decode_callback(ctx,
								bt_le_ext_adv_cb_connected_encoder);
	data->scanned = (bt_le_ext_adv_cb_scanned)ser_decode_callback(ctx,
								bt_le_ext_adv_cb_scanned_encoder);
}

static void bt_le_ext_adv_create_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	struct bt_le_adv_param param;
	struct bt_le_ext_adv *adv_data;
	struct bt_le_ext_adv **adv = &adv_data;
	size_t buffer_size_max = 10;
	struct ser_scratchpad scratchpad;

	size_t adv_index;
	struct bt_le_ext_adv_cb *cb = NULL;

	result = 0;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	bt_le_adv_param_dec(&scratchpad, &param);

	if (!ser_decode_is_undefined(ctx)) {
		result = k_mem_slab_alloc(&bt_rpc_ext_adv_cb_cache, (void **)&cb, K_NO_WAIT);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (result == 0) {
		result = bt_le_ext_adv_create(&param, cb, adv);
	}

	if (result == 0) {
		adv_index = bt_le_ext_adv_get_index(adv_data);
		ext_adv_cb_cache_map[adv_index] = cb;
	} else {
		k_mem_slab_free(&bt_rpc_ext_adv_cb_cache, (void *)cb);
	}

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		ser_encode_uint(&ectx, (uintptr_t)(*adv));

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_CREATE_RPC_CMD, handler_data);

	if (result == 0) {
		k_mem_slab_free(&bt_rpc_ext_adv_cb_cache, (void *)cb);
	}
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_create, BT_LE_EXT_ADV_CREATE_RPC_CMD,
			 bt_le_ext_adv_create_rpc_handler, NULL);


void bt_le_ext_adv_start_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_ext_adv_start_param *data)
{
	data->timeout = ser_decode_uint(ctx);
	data->num_events = ser_decode_uint(ctx);
}

static void bt_le_ext_adv_start_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_start_param param;
	int result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_ext_adv_start_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_start(adv, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_START_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_start, BT_LE_EXT_ADV_START_RPC_CMD,
			 bt_le_ext_adv_start_rpc_handler, NULL);

static void bt_le_ext_adv_stop_rpc_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	int result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_stop(adv);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_STOP_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_stop, BT_LE_EXT_ADV_STOP_RPC_CMD,
			 bt_le_ext_adv_stop_rpc_handler, NULL);

static void bt_le_ext_adv_set_data_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	size_t ad_len;
	struct bt_data *ad;
	size_t sd_len;
	struct bt_data *sd;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	ad_len = ser_decode_uint(ctx);
	ad = ser_scratchpad_add(&scratchpad, ad_len * sizeof(struct bt_data));
	if (ad == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_dec(&scratchpad, &ad[i]);
	}

	sd_len = ser_decode_uint(ctx);
	sd = ser_scratchpad_add(&scratchpad, sd_len * sizeof(struct bt_data));
	if (sd == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < sd_len; i++) {
		bt_data_dec(&scratchpad, &sd[i]);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_set_data(adv, ad, ad_len, sd, sd_len);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_SET_DATA_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_set_data, BT_LE_EXT_ADV_SET_DATA_RPC_CMD,
			 bt_le_ext_adv_set_data_rpc_handler, NULL);

static void bt_le_ext_adv_update_param_rpc_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_adv_param param;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_adv_param_dec(&scratchpad, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_update_param(adv, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_update_param, BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD,
			 bt_le_ext_adv_update_param_rpc_handler, NULL);

static void bt_le_ext_adv_delete_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	int result;

	size_t adv_index;
	struct bt_le_ext_adv_cb *cb;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	adv_index = bt_le_ext_adv_get_index(adv);

	result = bt_le_ext_adv_delete(adv);

	if (adv_index <= CONFIG_BT_EXT_ADV_MAX_ADV_SET) {
		cb = ext_adv_cb_cache_map[adv_index];
		k_mem_slab_free(&bt_rpc_ext_adv_cb_cache, (void *)cb);
	}

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_DELETE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_delete, BT_LE_EXT_ADV_DELETE_RPC_CMD,
			 bt_le_ext_adv_delete_rpc_handler, NULL);

static void bt_le_ext_adv_get_index_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	uint8_t result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_get_index(adv);

	ser_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_GET_INDEX_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_get_index, BT_LE_EXT_ADV_GET_INDEX_RPC_CMD,
			 bt_le_ext_adv_get_index_rpc_handler, NULL);

void bt_le_ext_adv_info_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_ext_adv_info *data)
{
	data->id = ser_decode_uint(ctx);
	data->tx_power = ser_decode_int(ctx);
}

static void bt_le_ext_adv_get_info_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_info info;
	int result;

	adv = (const struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_ext_adv_info_dec(ctx, &info);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_get_info(adv, &info);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_GET_INFO_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_get_info, BT_LE_EXT_ADV_GET_INFO_RPC_CMD,
			 bt_le_ext_adv_get_info_rpc_handler, NULL);

static void bt_le_ext_adv_oob_get_local_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	int result;
	struct bt_le_ext_adv *adv;
	struct bt_le_oob oob_data;
	struct bt_le_oob *oob = &oob_data;
	size_t buffer_size_max = 5;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_ext_adv_oob_get_local(adv, oob);

	buffer_size_max += bt_le_oob_buf_size(oob);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		bt_le_oob_enc(&ectx, oob);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_oob_get_local,
			 BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD,
			 bt_le_ext_adv_oob_get_local_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_EXT_ADV) */

#if defined(CONFIG_BT_OBSERVER)
static void bt_le_scan_start_rpc_handler(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_scan_param param;
	bt_le_scan_cb_t *cb;
	int result;

	bt_le_scan_param_dec(ctx, &param);
	cb = (bt_le_scan_cb_t *)ser_decode_callback(ctx, bt_le_scan_cb_t_encoder);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_scan_start(&param, cb);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_SCAN_START_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_start, BT_LE_SCAN_START_RPC_CMD,
			 bt_le_scan_start_rpc_handler, NULL);

static void bt_le_scan_stop_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_le_scan_stop();

	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_stop, BT_LE_SCAN_STOP_RPC_CMD,
			 bt_le_scan_stop_rpc_handler, NULL);

size_t bt_le_scan_recv_info_sp_size(const struct bt_le_scan_recv_info *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_scan_recv_info_buf_size(const struct bt_le_scan_recv_info *data)
{
	size_t buffer_size_max = 21;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_scan_recv_info_enc(struct nrf_rpc_cbor_ctx *encoder,
			      const struct bt_le_scan_recv_info *data)
{
	ser_encode_buffer(encoder, data->addr, sizeof(bt_addr_le_t));
	ser_encode_uint(encoder, data->sid);
	ser_encode_int(encoder, data->rssi);
	ser_encode_int(encoder, data->tx_power);
	ser_encode_uint(encoder, data->adv_type);
	ser_encode_uint(encoder, data->adv_props);
	ser_encode_uint(encoder, data->interval);
	ser_encode_uint(encoder, data->primary_phy);
	ser_encode_uint(encoder, data->secondary_phy);
}

void bt_le_scan_cb_recv(const struct bt_le_scan_recv_info *info,
			struct net_buf_simple *buf)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 5;

	buffer_size_max += bt_le_scan_recv_info_buf_size(info);
	buffer_size_max += net_buf_simple_buf_size(buf);

	scratchpad_size += bt_le_scan_recv_info_sp_size(info);
	scratchpad_size += net_buf_simple_sp_size(buf);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	bt_le_scan_recv_info_enc(&ctx, info);
	net_buf_simple_enc(&ctx, buf);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_RECV_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_scan_cb_timeout(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_TIMEOUT_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = bt_le_scan_cb_recv,
	.timeout = bt_le_scan_cb_timeout,
};

static void bt_le_scan_cb_register_on_remote_rpc_handler(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	bt_le_scan_cb_register(&scan_cb);

	ser_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_register_on_remote,
			 BT_LE_SCAN_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_le_scan_cb_register_on_remote_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_OBSERVER) */

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static void bt_le_filter_accept_list_add_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	int result;

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_filter_accept_list_add(addr);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_FILTER_ACCEPT_LIST_ADD_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_filter_accept_list_add,
			 BT_LE_FILTER_ACCEPT_LIST_ADD_RPC_CMD,
			 bt_le_filter_accept_list_add_rpc_handler, NULL);

static void bt_le_filter_accept_list_remove_rpc_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	int result;

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_filter_accept_list_remove(addr);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_FILTER_ACCEPT_LIST_REMOVE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_filter_accept_list_remove,
			 BT_LE_FILTER_ACCEPT_LIST_REMOVE_RPC_CMD,
			 bt_le_filter_accept_list_remove_rpc_handler, NULL);

static void bt_le_filter_accept_list_clear_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_le_filter_accept_list_clear();

	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_filter_accept_list_clear,
			 BT_LE_ACCEPT_LIST_CLEAR_RPC_CMD,
			 bt_le_filter_accept_list_clear_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

static void bt_le_set_chan_map_rpc_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t *chan_map;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	chan_map = ser_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_set_chan_map(chan_map);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_SET_CHAN_MAP_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_set_chan_map, BT_LE_SET_CHAN_MAP_RPC_CMD,
			 bt_le_set_chan_map_rpc_handler, NULL);

static void bt_le_oob_get_local_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	uint8_t id;
	struct bt_le_oob oob_data;
	struct bt_le_oob *oob = &oob_data;
	size_t buffer_size_max = 5;

	id = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_oob_get_local(id, oob);

	buffer_size_max += bt_le_oob_buf_size(oob);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		bt_le_oob_enc(&ectx, oob);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_LE_OOB_GET_LOCAL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_get_local, BT_LE_OOB_GET_LOCAL_RPC_CMD,
			 bt_le_oob_get_local_rpc_handler, NULL);

#if defined(CONFIG_BT_CONN)
static void bt_unpair_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	uint8_t id;
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	int result;

	id = ser_decode_uint(ctx);
	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_unpair(id, addr);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_UNPAIR_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_unpair, BT_UNPAIR_RPC_CMD,
			 bt_unpair_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_CONN) */

#if (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP))
size_t bt_bond_info_buf_size(const struct bt_bond_info *data)
{
	size_t buffer_size_max = 3;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_bond_info_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_bond_info *data)
{
	ser_encode_buffer(encoder, &data->addr, sizeof(bt_addr_le_t));
}

static inline void bt_foreach_bond_cb_callback(const struct bt_bond_info *info,
					       void *user_data,
					       uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;

	buffer_size_max += bt_bond_info_buf_size(info);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_bond_info_enc(&ctx, info);
	ser_encode_uint(&ctx, (uintptr_t)user_data);
	ser_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_FOREACH_BOND_CB_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_foreach_bond_cb_encoder, bt_foreach_bond_cb_callback,
		 (const struct bt_bond_info *info, void *user_data), (info, user_data));

static void bt_foreach_bond_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t id;
	bt_foreach_bond_cb func;
	void *user_data;

	id = ser_decode_uint(ctx);
	func = (bt_foreach_bond_cb)ser_decode_callback(ctx, bt_foreach_bond_cb_encoder);
	user_data = (void *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	bt_foreach_bond(id, func, user_data);

	ser_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_FOREACH_BOND_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_foreach_bond, BT_FOREACH_BOND_RPC_CMD,
			 bt_foreach_bond_rpc_handler, NULL);
#endif /* (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP)) */

#if defined(CONFIG_BT_PER_ADV)
static void bt_le_per_adv_list_clear_rpc_handler(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_le_per_adv_list_clear();

	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_list_clear, BT_LE_PER_ADV_LIST_CLEAR_RPC_CMD,
			 bt_le_per_adv_list_clear_rpc_handler, NULL);

static void bt_le_per_adv_list_add_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	uint8_t sid;
	int result;

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	sid = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_list_add(addr, sid);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_LIST_ADD_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_list_add, BT_LE_PER_ADV_LIST_ADD_RPC_CMD,
			 bt_le_per_adv_list_add_rpc_handler, NULL);

static void bt_le_per_adv_list_remove_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bt_addr_le_t addr_data;
	const bt_addr_le_t *addr;
	uint8_t sid;
	int result;

	addr = ser_decode_buffer(ctx, &addr_data, sizeof(bt_addr_le_t));
	sid = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_list_remove(addr, sid);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_LIST_REMOVE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_list_remove, BT_LE_PER_ADV_LIST_REMOVE_RPC_CMD,
			 bt_le_per_adv_list_remove_rpc_handler, NULL);

void bt_le_per_adv_param_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_le_per_adv_param *data)
{
	data->interval_min = ser_decode_uint(ctx);
	data->interval_max = ser_decode_uint(ctx);
	data->options = ser_decode_uint(ctx);
}

static void bt_le_per_adv_set_param_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_per_adv_param param;
	int result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);
	bt_le_per_adv_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_set_param(adv, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SET_PARAM_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_set_param, BT_LE_PER_ADV_SET_PARAM_RPC_CMD,
			 bt_le_per_adv_set_param_rpc_handler, NULL);

static void bt_le_per_adv_set_data_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct bt_le_ext_adv *adv;
	size_t ad_len;
	struct bt_data *ad;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	adv = (const struct bt_le_ext_adv *)ser_decode_uint(ctx);
	ad_len = ser_decode_uint(ctx);
	ad = ser_scratchpad_add(&scratchpad, ad_len * sizeof(struct bt_data));
	if (ad == NULL) {
		goto decoding_error;
	}

	for (size_t i = 0; i < ad_len; i++) {
		bt_data_dec(&scratchpad, &ad[i]);
	}

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_set_data(adv, ad, ad_len);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SET_DATA_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_set_data, BT_LE_PER_ADV_SET_DATA_RPC_CMD,
			 bt_le_per_adv_set_data_rpc_handler, NULL);

static void bt_le_per_adv_start_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	int result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_start(adv);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_START_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_start, BT_LE_PER_ADV_START_RPC_CMD,
			 bt_le_per_adv_start_rpc_handler, NULL);

static void bt_le_per_adv_stop_rpc_handler(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_ext_adv *adv;
	int result;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_stop(adv);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_STOP_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_stop, BT_LE_PER_ADV_STOP_RPC_CMD,
			 bt_le_per_adv_stop_rpc_handler, NULL);

#if defined(CONFIG_BT_CONN)
static void bt_le_per_adv_set_info_transfer_rpc_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	const struct bt_le_ext_adv *adv;
	const struct bt_conn *conn;
	uint16_t service_data;
	int result;

	adv = (const struct bt_le_ext_adv *)ser_decode_uint(ctx);
	conn = bt_rpc_decode_bt_conn(ctx);
	service_data = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_set_info_transfer(adv, conn, service_data);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SET_INFO_TRANSFER_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_set_info_transfer,
			 BT_LE_PER_ADV_SET_INFO_TRANSFER_RPC_CMD,
			 bt_le_per_adv_set_info_transfer_rpc_handler, NULL);
#endif  /* defined(CONFIG_BT_CONN) */
#endif  /* defined(CONFIG_BT_PER_ADV) */

#if defined(CONFIG_BT_PER_ADV_SYNC)
static void bt_le_per_adv_sync_get_index_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	struct bt_le_per_adv_sync *per_adv_sync;
	uint8_t result;

	per_adv_sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_get_index(per_adv_sync);

	ser_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_GET_INDEX_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_get_index,
			 BT_LE_PER_ADV_SYNC_GET_INDEX_RPC_CMD,
			 bt_le_per_adv_sync_get_index_rpc_handler, NULL);

void bt_le_per_adv_sync_param_dec(struct nrf_rpc_cbor_ctx *ctx,
				  struct bt_le_per_adv_sync_param *data)
{
	ser_decode_buffer(ctx, &data->addr, sizeof(bt_addr_le_t));
	data->sid = ser_decode_uint(ctx);
	data->options = ser_decode_uint(ctx);
	data->skip = ser_decode_uint(ctx);
	data->timeout = ser_decode_uint(ctx);
}

static void bt_le_per_adv_sync_create_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	struct bt_le_per_adv_sync_param param;
	struct bt_le_per_adv_sync *out_sync_data;
	struct bt_le_per_adv_sync **out_sync = &out_sync_data;
	size_t buffer_size_max = 10;

	bt_le_per_adv_sync_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_create(&param, out_sync);

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		ser_encode_int(&ectx, result);
		ser_encode_uint(&ectx, (uintptr_t)(*out_sync));

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_CREATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_create, BT_LE_PER_ADV_SYNC_CREATE_RPC_CMD,
			 bt_le_per_adv_sync_create_rpc_handler, NULL);

static void bt_le_per_adv_sync_delete_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_le_per_adv_sync *per_adv_sync;
	int result;

	per_adv_sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_delete(per_adv_sync);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_DELETE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_delete, BT_LE_PER_ADV_SYNC_DELETE_RPC_CMD,
			 bt_le_per_adv_sync_delete_rpc_handler, NULL);

static void bt_le_per_adv_sync_recv_enable_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_le_per_adv_sync *per_adv_sync;
	int result;

	per_adv_sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_recv_enable(per_adv_sync);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_RECV_ENABLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_recv_enable,
			 BT_LE_PER_ADV_SYNC_RECV_ENABLE_RPC_CMD,
			 bt_le_per_adv_sync_recv_enable_rpc_handler, NULL);

static void bt_le_per_adv_sync_recv_disable_rpc_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	struct bt_le_per_adv_sync *per_adv_sync;
	int result;

	per_adv_sync = (struct bt_le_per_adv_sync *)ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_recv_disable(per_adv_sync);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_RECV_DISABLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_recv_disable,
			 BT_LE_PER_ADV_SYNC_RECV_DISABLE_RPC_CMD,
			 bt_le_per_adv_sync_recv_disable_rpc_handler, NULL);

#if defined(CONFIG_BT_CONN)
static void bt_le_per_adv_sync_transfer_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	const struct bt_le_per_adv_sync *per_adv_sync;
	const struct bt_conn *conn;
	uint16_t service_data;
	int result;

	per_adv_sync = (const struct bt_le_per_adv_sync *)ser_decode_uint(ctx);
	conn = bt_rpc_decode_bt_conn(ctx);
	service_data = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_transfer(per_adv_sync, conn, service_data);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_TRANSFER_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_transfer,
			 BT_LE_PER_ADV_SYNC_TRANSFER_RPC_CMD,
			 bt_le_per_adv_sync_transfer_rpc_handler, NULL);

static void bt_le_per_adv_sync_transfer_unsubscribe_rpc_handler(const struct nrf_rpc_group *group,
								struct nrf_rpc_cbor_ctx *ctx,
								void *handler_data)
{
	const struct bt_conn *conn;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_transfer_unsubscribe(conn);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_TRANSFER_UNSUBSCRIBE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_transfer_unsubscribe,
			 BT_LE_PER_ADV_SYNC_TRANSFER_UNSUBSCRIBE_RPC_CMD,
			 bt_le_per_adv_sync_transfer_unsubscribe_rpc_handler, NULL);

void bt_le_per_adv_sync_transfer_param_dec(struct nrf_rpc_cbor_ctx *ctx,
					   struct bt_le_per_adv_sync_transfer_param *data)
{
	data->skip = ser_decode_uint(ctx);
	data->timeout = ser_decode_uint(ctx);
	data->options = ser_decode_uint(ctx);
}

static void bt_le_per_adv_sync_transfer_subscribe_rpc_handler(const struct nrf_rpc_group *group,
							      struct nrf_rpc_cbor_ctx *ctx,
							      void *handler_data)
{
	const struct bt_conn *conn;
	struct bt_le_per_adv_sync_transfer_param param;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_le_per_adv_sync_transfer_param_dec(ctx, &param);

	if (!ser_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_le_per_adv_sync_transfer_subscribe(conn, &param);

	ser_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_LE_PER_ADV_SYNC_TRANSFER_SUBSCRIBE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_transfer_subscribe,
			 BT_LE_PER_ADV_SYNC_TRANSFER_SUBSCRIBE_RPC_CMD,
			 bt_le_per_adv_sync_transfer_subscribe_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_CONN) */

size_t bt_le_per_adv_sync_synced_info_sp_size(const struct bt_le_per_adv_sync_synced_info *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_per_adv_sync_synced_info_buf_size(const struct bt_le_per_adv_sync_synced_info *data)
{
	size_t buffer_size_max = 17;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_per_adv_sync_synced_info_enc(struct nrf_rpc_cbor_ctx *encoder,
					const struct bt_le_per_adv_sync_synced_info *data)
{
	ser_encode_buffer(encoder, data->addr, sizeof(bt_addr_le_t));
	ser_encode_uint(encoder, data->sid);
	ser_encode_uint(encoder, data->interval);
	ser_encode_uint(encoder, data->phy);
	ser_encode_bool(encoder, data->recv_enabled);
	ser_encode_uint(encoder, data->service_data);
	bt_rpc_encode_bt_conn(encoder, data->conn);
}

void per_adv_sync_cb_synced(struct bt_le_per_adv_sync *sync,
			    struct bt_le_per_adv_sync_synced_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	buffer_size_max += bt_le_per_adv_sync_synced_info_buf_size(info);

	scratchpad_size += bt_le_per_adv_sync_synced_info_sp_size(info);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)sync);
	bt_le_per_adv_sync_synced_info_enc(&ctx, info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, PER_ADV_SYNC_CB_SYNCED_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

size_t bt_le_per_adv_sync_term_info_sp_size(const struct bt_le_per_adv_sync_term_info *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_per_adv_sync_term_info_buf_size(const struct bt_le_per_adv_sync_term_info *data)
{
	size_t buffer_size_max = 5;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_per_adv_sync_term_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				      const struct bt_le_per_adv_sync_term_info *data)
{
	ser_encode_buffer(encoder, data->addr, sizeof(bt_addr_le_t));
	ser_encode_uint(encoder, data->sid);
}

void per_adv_sync_cb_term(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_term_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	buffer_size_max += bt_le_per_adv_sync_term_info_buf_size(info);

	scratchpad_size += bt_le_per_adv_sync_term_info_sp_size(info);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)sync);
	bt_le_per_adv_sync_term_info_enc(&ctx, info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, PER_ADV_SYNC_CB_TERM_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

size_t bt_le_per_adv_sync_recv_info_sp_size(const struct bt_le_per_adv_sync_recv_info *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));

	return scratchpad_size;
}

size_t bt_le_per_adv_sync_recv_info_buf_size(const struct bt_le_per_adv_sync_recv_info *data)
{
	size_t buffer_size_max = 11;

	buffer_size_max += sizeof(bt_addr_le_t);

	return buffer_size_max;
}

void bt_le_per_adv_sync_recv_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				      const struct bt_le_per_adv_sync_recv_info *data)
{
	ser_encode_buffer(encoder, data->addr, sizeof(bt_addr_le_t));
	ser_encode_uint(encoder, data->sid);
	ser_encode_int(encoder, data->tx_power);
	ser_encode_int(encoder, data->rssi);
	ser_encode_uint(encoder, data->cte_type);
}

void per_adv_sync_cb_recv(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_recv_info *info,
			  struct net_buf_simple *buf)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	buffer_size_max += bt_le_per_adv_sync_recv_info_buf_size(info);
	buffer_size_max += net_buf_simple_buf_size(buf);

	scratchpad_size += bt_le_per_adv_sync_recv_info_sp_size(info);
	scratchpad_size += net_buf_simple_sp_size(buf);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);

	ser_encode_uint(&ctx, (uintptr_t)sync);
	bt_le_per_adv_sync_recv_info_enc(&ctx, info);
	net_buf_simple_enc(&ctx, buf);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, PER_ADV_SYNC_CB_RECV_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_le_per_adv_sync_state_info_enc(struct nrf_rpc_cbor_ctx *encoder,
				       const struct bt_le_per_adv_sync_state_info *data)
{
	ser_encode_bool(encoder, data->recv_enabled);
}

void per_adv_sync_cb_state_changed(struct bt_le_per_adv_sync *sync,
				   const struct bt_le_per_adv_sync_state_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 6;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, (uintptr_t)sync);
	bt_le_per_adv_sync_state_info_enc(&ctx, info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, PER_ADV_SYNC_CB_STATE_CHANGED_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static struct bt_le_per_adv_sync_cb per_adv_sync_cb = {
	.synced = per_adv_sync_cb_synced,
	.term = per_adv_sync_cb_term,
	.recv = per_adv_sync_cb_recv,
	.state_changed = per_adv_sync_cb_state_changed
};

static void bt_le_per_adv_sync_cb_register_on_remote_rpc_handler(const struct nrf_rpc_group *group,
								 struct nrf_rpc_cbor_ctx *ctx,
								 void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	bt_le_per_adv_sync_cb_register(&per_adv_sync_cb);
	ser_rsp_send_void(group);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_per_adv_sync_cb_register_on_remote,
			 BT_LE_PER_ADV_SYNC_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_le_per_adv_sync_cb_register_on_remote_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */

#if defined(CONFIG_SETTINGS)
static void bt_rpc_settings_load_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	settings_load();
	ser_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_settings_load,
			 BT_SETTINGS_LOAD_RPC_CMD,
			 bt_rpc_settings_load_rpc_handler, NULL);
#endif /* defined(CONFIG_SETTINGS) */
