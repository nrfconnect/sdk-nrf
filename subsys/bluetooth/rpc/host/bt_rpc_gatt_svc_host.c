/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_gatt_common.h"
#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

struct remote_svc {
	struct bt_gatt_service *service;
	size_t attr_max;
	uint32_t index;
};

static struct bt_uuid const * const uuid_primary = BT_UUID_GATT_PRIMARY;
static struct bt_uuid const * const uuid_secondary = BT_UUID_GATT_SECONDARY;
static struct bt_uuid const * const uuid_chrc = BT_UUID_GATT_CHRC;
static struct bt_uuid const * const uuid_ccc = BT_UUID_GATT_CCC;
static struct bt_uuid const * const uuid_cep = BT_UUID_GATT_CEP;
static struct bt_uuid const * const uuid_cud = BT_UUID_GATT_CUD;
static struct bt_uuid const * const uuid_cpf = BT_UUID_GATT_CPF;

static uint32_t gatt_buffer_data[ceiling_fraction(CONFIG_BT_RPC_GATT_BUFFER_SIZE,
						  sizeof(uint32_t))];
static struct net_buf_simple gatt_buffer = {
	.data = (uint8_t *)gatt_buffer_data,
	.len  = 0,
	.size   = sizeof(gatt_buffer_data),
	.__buf  = (uint8_t *)gatt_buffer_data
};

static struct remote_svc current_service;

static inline void *bt_rpc_gatt_add(struct net_buf_simple *buf, size_t size)
{
	return net_buf_simple_add(buf, WB_UP(size));
}

void bt_rpc_encode_gatt_attr(CborEncoder *encoder, const struct bt_gatt_attr *attr)
{
	uint32_t attr_index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &attr_index);
	__ASSERT(err = 0, "Service attribute not found. Service database might be out of sync");

	ser_encode_uint(encoder, attr_index);
}
const struct bt_gatt_attr *bt_rpc_decode_gatt_attr(CborValue *value)
{
	uint32_t attr_index;

	attr_index = ser_decode_uint(value);

	return bt_rpc_gatt_index_to_attr(attr_index);
}
static struct bt_uuid *bt_uuid_gatt_dec(CborValue *value)
{
	struct bt_uuid *uuid;
	size_t buffer_size = ser_decode_buffer_size(value);

	if (buffer_size == sizeof(struct bt_uuid_16)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_16));
	} else if (buffer_size == sizeof(struct bt_uuid_32)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_32));
	} else if (buffer_size == sizeof(struct bt_uuid_128)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_128));
	} else {
		ser_decoder_invalid(value, CborErrorIllegalType);
		return NULL;
	}

	if (!uuid) {
		ser_decoder_invalid(value, CborErrorOutOfMemory);
		return NULL;
	}

	ser_decode_buffer(value, uuid, buffer_size);

	return uuid;
}

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static int bt_rpc_gatt_start_service(uint8_t remote_service_index, size_t attr_count)
{
	int err;
	uint32_t index;
	struct bt_gatt_service *service;
	struct bt_gatt_attr *attrs;

	service = (struct bt_gatt_service *)bt_rpc_gatt_add(&gatt_buffer,
							    sizeof(struct bt_gatt_service));
	attrs = (struct bt_gatt_attr *)bt_rpc_gatt_add(&gatt_buffer,
						       sizeof(struct bt_gatt_attr) * attr_count);

	if (!service || !attrs) {
		return -ENOMEM;
	}

	memset(service, 0, sizeof(struct bt_gatt_service));
	memset(attrs, 0, sizeof(struct bt_gatt_attr) * attr_count);

	service->attr_count = 0;
	service->attrs = attrs;

	err = bt_rpc_gatt_add_service(service, &index);
	if (err) {
		return err;
	}

	if (index != (uint32_t)remote_service_index) {
		return -EINVAL;
	}

	current_service.service = service;
	current_service.attr_max = attr_count;
	current_service.index = index;

	return 0;
}

static void bt_rpc_gatt_start_service_rpc_handler(CborValue *value, void *handler_data)
{

	uint8_t service_index;
	size_t attr_count;
	int result;

	service_index = ser_decode_uint(value);
	attr_count = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_start_service(service_index, attr_count);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_START_SERVICE_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_start_service, BT_RPC_GATT_START_SERVICE_RPC_CMD,
	bt_rpc_gatt_start_service_rpc_handler, NULL);

struct bt_normal_attr_read_res {
	uint8_t *buf;
	int read_len;
};

static void bt_normal_attr_read_rsp(CborValue *value, void *handler_data)
{
	struct bt_normal_attr_read_res *res =
		(struct bt_normal_attr_read_res *)handler_data;

	res->read_len = ser_decode_int(value);
	ser_decode_buffer(value, res->buf, (res->read_len > 0) ? res->read_len : 0);
}

ssize_t bt_rpc_normal_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_normal_attr_read_res result;
	size_t buffer_size_max = 19;
	size_t scratchpad_size = 0;
	uint8_t read_buf[len];

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	scratchpad_size += SCRATCHPAD_ALIGN(len);

	ser_encode_uint(&ctx.encoder, scratchpad_size);
	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	bt_rpc_encode_gatt_attr(&ctx.encoder, attr);
	ser_encode_uint(&ctx.encoder, len);
	ser_encode_uint(&ctx.encoder, offset);

	result.buf = read_buf;
	result.read_len = 0;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_ATTR_READ_RPC_CMD,
				&ctx, bt_normal_attr_read_rsp, &result);

	if (result.read_len < 0) {
		return result.read_len;
	} else {
		return bt_gatt_attr_read(conn, attr, buf, len, 0,
					 result.buf, result.read_len);
	}
}

ssize_t bt_rpc_normal_attr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 26;
	size_t scratchpad_size = 0;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	buffer_size_max += len;

	scratchpad_size += SCRATCHPAD_ALIGN(len);

	ser_encode_uint(&ctx.encoder, scratchpad_size);
	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	bt_rpc_encode_gatt_attr(&ctx.encoder, attr);
	ser_encode_uint(&ctx.encoder, len);
	ser_encode_uint(&ctx.encoder, offset);
	ser_encode_uint(&ctx.encoder, flags);
	ser_encode_buffer(&ctx.encoder, buf, len);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_ATTR_WRITE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static void add_user_attr(struct bt_gatt_attr *attr, const struct bt_uuid *uuid, uint16_t data)
{
	attr->uuid = uuid;
	attr->read = (data & BT_RPC_GATT_ATTR_READ_PRESENT_FLAG) ?
		     bt_rpc_normal_attr_read : NULL;
	attr->write = (data & BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG) ?
		      bt_rpc_normal_attr_write : NULL;
	attr->user_data = (void *)((current_service.index << 16) |
				   current_service.service->attr_count);
	attr->handle = 0;
	attr->perm = (uint8_t)data;
}

static void add_srv_attr(struct bt_gatt_attr *attr, const struct bt_uuid *service_uuid,
			 const struct bt_uuid *attr_uuid)
{
	attr->uuid = attr_uuid;
	attr->read = bt_gatt_attr_read_service;
	attr->write = NULL;
	attr->user_data = (void *) service_uuid;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;
}

static int add_chrc_attr(struct bt_gatt_attr *attr, const struct bt_uuid *uuid, uint8_t properties)
{
	struct bt_gatt_chrc *chrc;

	chrc = (struct bt_gatt_chrc *)bt_rpc_gatt_add(&gatt_buffer,
						      sizeof(struct bt_gatt_chrc));
	if (!chrc) {
		return -ENOMEM;
	}

	memset(chrc, 0, sizeof(struct bt_gatt_chrc));

	chrc->uuid = uuid;
	chrc->properties = properties;
	chrc->value_handle = 0;

	attr->uuid = uuid_chrc;
	attr->read = bt_gatt_attr_read_chrc;
	attr->write = NULL;
	attr->user_data = (void *)chrc;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

static int bt_rpc_gatt_send_simple_attr(uint8_t special_attr, const struct bt_uuid *uuid,
					uint16_t data)
{
	int err = 0;
	struct bt_gatt_attr *attr;

	if (!current_service.service ||
	    (current_service.service->attr_count >= current_service.attr_max)) {
		return -ENOMEM;
	}

	attr = &current_service.service->attrs[current_service.service->attr_count];

	switch (special_attr) {
	case BT_RPC_GATT_ATTR_USER_DEFINED:
		add_user_attr(attr, uuid, data);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_SERVICE:
		add_srv_attr(attr, uuid, uuid_primary);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_SECONDARY:
		add_srv_attr(attr, uuid, uuid_secondary);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_CHRC:
		err = add_chrc_attr(attr, uuid, data);
		break;
	default:
		return -EINVAL;
	}

	if (!err) {
		current_service.service->attr_count++;
	}

	return err;
}

static void bt_rpc_gatt_send_simple_attr_rpc_handler(CborValue *value, void *handler_data)
{

	uint8_t special_attr;
	uint16_t data;
	int result;

	struct bt_uuid *uuid;

	uuid = bt_uuid_gatt_dec(value);

	special_attr = ser_decode_uint(value);
	data = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_send_simple_attr(special_attr, uuid, data);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD, handler_data);

}

static int add_cep_attr(struct bt_gatt_attr *attr, uint16_t properties)
{
	struct bt_gatt_cep *cep;

	cep = (struct bt_gatt_cep *)bt_rpc_gatt_add(&gatt_buffer,
						    sizeof(struct bt_gatt_cep));
	if (!cep) {
		return -ENOMEM;
	}

	memset(cep, 0, sizeof(*cep));

	cep->properties = properties;

	attr->uuid = uuid_cep;
	attr->read = bt_gatt_attr_read_cep;
	attr->write = NULL;
	attr->user_data = (void *)cep;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

static int add_cud_attr(struct bt_gatt_attr *attr, uint8_t perm, char *text, size_t size)
{
	attr->user_data = bt_rpc_gatt_add(&gatt_buffer, size);
	if (!attr->user_data) {
		return -ENOMEM;
	}

	memset(attr->user_data, 0, size);

	attr->uuid = uuid_cud;
	attr->read =  bt_gatt_attr_read_cud;
	attr->write = NULL;
	attr->handle = 0;
	attr->perm = perm;

	memcpy(attr->user_data, text, size);

	return 0;
}

static int add_cpf_attr(struct bt_gatt_attr *attr, uint8_t *buffer, size_t size)
{
	struct bt_gatt_cpf *cpf;

	cpf = (struct bt_gatt_cpf *)bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_gatt_cpf));
	if (!cpf || (size != sizeof(struct bt_gatt_cpf))) {
		return -ENOMEM;
	}

	memset(cpf, 0, sizeof(*cpf));
	memcpy(cpf, buffer, size);

	attr->uuid = uuid_cpf;
	attr->read = bt_gatt_attr_read_cpf;
	attr->write = NULL;
	attr->user_data = (void *)cpf;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

static void bt_ccc_cfg_changed_call(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 8;
	uint32_t index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &index);
	if (err) {
		LOG_WRN("Cannot find CCC descriptor");
		return;
	}

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	ser_encode_uint(&ctx.encoder, index);
	ser_encode_uint(&ctx.encoder, value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_CHANGED_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

static ssize_t bt_ccc_cfg_write_call(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, uint16_t value)
{
	struct nrf_rpc_cbor_ctx ctx;
	ssize_t result;
	size_t buffer_size_max = 11;
	uint32_t index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &index);
	if (err) {
		LOG_WRN("Cannot find CCC descriptor");
		return 0;
	}

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	ser_encode_uint(&ctx.encoder, index);
	ser_encode_uint(&ctx.encoder, value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_WRITE_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static bool bt_ccc_cfg_match_call(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;
	size_t buffer_size_max = 8;
	uint32_t index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &index);
	if (err) {
		LOG_WRN("Cannot find CCC descriptor");
		return false;
	}

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	ser_encode_uint(&ctx.encoder, index);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_MATCH_RPC_CMD,
				&ctx, ser_rsp_decode_bool, &result);

	return result;
}

static int add_ccc_attr(struct bt_gatt_attr *attr, uint16_t param)
{
	struct _bt_gatt_ccc *ccc;

	ccc = (struct _bt_gatt_ccc *)bt_rpc_gatt_add(&gatt_buffer, sizeof(struct _bt_gatt_ccc));
	if (!ccc) {
		return -ENOMEM;
	}

	memset(ccc, 0, sizeof(struct _bt_gatt_ccc));

	ccc->cfg_changed = (param & BT_RPC_GATT_CCC_CFG_CHANGE_PRESENT_FLAG) ?
								bt_ccc_cfg_changed_call : NULL;
	ccc->cfg_write = (param & BT_RPC_GATT_CCC_CFG_WRITE_PRESENT_FLAG) ?
								bt_ccc_cfg_write_call : NULL;
	ccc->cfg_match = (param & BT_RPC_GATT_CCC_CFG_MATCH_PRESET_FLAG) ?
								bt_ccc_cfg_match_call : NULL;

	attr->uuid = uuid_ccc;
	attr->read = bt_gatt_attr_read_ccc;
	attr->write = bt_gatt_attr_write_ccc;
	attr->user_data = (void *)ccc;
	attr->perm = (uint8_t)param;

	return 0;
}

static int bt_rpc_gatt_send_desc_attr(uint8_t special_attr, uint16_t param, uint8_t *buffer,
				      size_t size)
{

	int err = 0;
	struct bt_gatt_attr *attr;

	if (!current_service.service ||
	    (current_service.service->attr_count >= current_service.attr_max)) {
		return -ENOMEM;
	}

	attr = &current_service.service->attrs[current_service.service->attr_count];

	switch (special_attr) {
	case BT_RPC_GATT_ATTR_SPECIAL_CCC:
		err = add_ccc_attr(attr, param);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_CEP:
		err = add_cep_attr(attr, param);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_CUD:
		err = add_cud_attr(attr, param, buffer, size);
		break;
	case BT_RPC_GATT_ATTR_SPECIAL_CPF:
		err = add_cpf_attr(attr, buffer, size);
		break;
	default:
		return -EINVAL;
	}

	if (!err) {
		current_service.service->attr_count++;
	}

	return err;
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_send_simple_attr,
			 BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,
			 bt_rpc_gatt_send_simple_attr_rpc_handler, NULL);

static void bt_rpc_gatt_send_desc_attr_rpc_handler(CborValue *value, void *handler_data)
{

	uint8_t special_attr;
	uint16_t param;
	size_t size;
	uint8_t *buffer;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	special_attr = ser_decode_uint(value);
	param = ser_decode_uint(value);
	size = ser_decode_uint(value);
	buffer = ser_decode_buffer_into_scratchpad(&scratchpad);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_send_desc_attr(special_attr, param, buffer, size);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_send_desc_attr,
			 BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,
			 bt_rpc_gatt_send_desc_attr_rpc_handler, NULL);

static int bt_rpc_gatt_end_service(void)
{
	int err;

	err = bt_gatt_service_register(current_service.service);

	current_service.service = NULL;
	current_service.attr_max = 0;

	return err;
}

static void bt_rpc_gatt_end_service_rpc_handler(CborValue *value, void *handler_data)
{

	int result;

	nrf_rpc_cbor_decoding_done(value);

	result = bt_rpc_gatt_end_service();

	ser_rsp_send_int(result);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_end_service, BT_RPC_GATT_END_SERVICE_RPC_CMD,
	bt_rpc_gatt_end_service_rpc_handler, NULL);

static void bt_rpc_gatt_service_unregister_rpc_handler(CborValue *value, void *handler_data)
{
	int result;
	uint16_t svc_index;
	const struct bt_gatt_service *svc;

	svc_index = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	svc = bt_rpc_gatt_get_service_by_index(svc_index);
	if (!svc) {
		result = -EINVAL;
	}

	if (!result) {
		result = bt_gatt_service_unregister((struct bt_gatt_service *)svc);
	}

	if (!result) {
		result = bt_rpc_gatt_remove_service(svc);
	}

	ser_rsp_send_int(result);
	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_service_unregister,
			 BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD,
			 bt_rpc_gatt_service_unregister_rpc_handler, NULL);

static inline void bt_gatt_complete_func_t_callback(struct bt_conn *conn, void *user_data,
						    uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 13;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	ser_encode_uint(&ctx.encoder, (uintptr_t)user_data);
	ser_encode_callback_call(&ctx.encoder, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

CBKPROXY_HANDLER(bt_gatt_complete_func_t_encoder, bt_gatt_complete_func_t_callback,
		 (struct bt_conn *conn, void *user_data), (conn, user_data));

void bt_gatt_notify_params_dec(struct ser_scratchpad *scratchpad,
			       struct bt_gatt_notify_params *data)
{

	CborValue *value = scratchpad->value;

	data->attr = bt_rpc_decode_gatt_attr(value);
	data->len = ser_decode_uint(value);
	data->data = ser_decode_buffer_into_scratchpad(scratchpad);
	data->func = (bt_gatt_complete_func_t)ser_decode_callback(value,
								   bt_gatt_complete_func_t_encoder);
	data->user_data = (void *)(uintptr_t)ser_decode_uint(value);

	data->uuid = (struct bt_uuid *)ser_decode_buffer_into_scratchpad(scratchpad);

}

static void bt_gatt_notify_cb_rpc_handler(CborValue *value, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_gatt_notify_params params;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	conn = bt_rpc_decode_bt_conn(value);
	bt_gatt_notify_params_dec(&scratchpad, &params);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_gatt_notify_cb(conn, &params);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_NOTIFY_CB_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_notify_cb, BT_GATT_NOTIFY_CB_RPC_CMD,
	bt_gatt_notify_cb_rpc_handler, NULL);

void bt_gatt_indicate_params_dec(struct ser_scratchpad *scratchpad,
				 struct bt_gatt_indicate_params *data)
{

	CborValue *value = scratchpad->value;

	data->attr = bt_rpc_decode_gatt_attr(value);
	data->len = ser_decode_uint(value);
	data->data = ser_decode_buffer_into_scratchpad(scratchpad);
	data->_ref = ser_decode_uint(value);

	data->uuid = (struct bt_uuid *)ser_decode_buffer_into_scratchpad(scratchpad);

}

static void bt_gatt_indicate_func_t_callback(struct bt_conn *conn,
					     struct bt_gatt_indicate_params *params,
					     uint8_t err)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 10;
	struct bt_rpc_gatt_indication_params *rpc_params;

	rpc_params = CONTAINER_OF(params, struct bt_rpc_gatt_indication_params, params);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	ser_encode_uint(&ctx.encoder, err);
	ser_encode_uint(&ctx.encoder, rpc_params->param_addr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD,
		&ctx, ser_rsp_decode_void, NULL);
}

static void bt_gatt_indicate_params_destroy_t_callback(struct bt_gatt_indicate_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 5;
	struct bt_rpc_gatt_indication_params *rpc_params;

	rpc_params = CONTAINER_OF(params, struct bt_rpc_gatt_indication_params, params);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	ser_encode_uint(&ctx.encoder, rpc_params->param_addr);

	k_free(rpc_params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_PARAMS_DESTROY_T_CALLBACK_RPC_CMD,
		&ctx, ser_rsp_decode_void, NULL);
}

static void bt_gatt_indicate_rpc_handler(CborValue *value, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_rpc_gatt_indication_params *params;
	int result;
	struct ser_scratchpad scratchpad;

	params = (struct bt_rpc_gatt_indication_params *)k_malloc(sizeof(*params));

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	conn = bt_rpc_decode_bt_conn(value);
	bt_gatt_indicate_params_dec(&scratchpad, &params->params);
	params->param_addr = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	params->params.func = bt_gatt_indicate_func_t_callback;
	params->params.destroy = bt_gatt_indicate_params_destroy_t_callback;

	result = bt_gatt_indicate(conn, &params->params);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_INDICATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_indicate, BT_GATT_INDICATE_RPC_CMD,
	bt_gatt_indicate_rpc_handler, NULL);

static void bt_gatt_is_subscribed_rpc_handler(CborValue *value, void *handler_data)
{

	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	uint16_t ccc_value;
	bool result;

	conn = bt_rpc_decode_bt_conn(value);
	attr = bt_rpc_decode_gatt_attr(value);
	ccc_value = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_gatt_is_subscribed(conn, attr, ccc_value);

	ser_rsp_send_bool(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_IS_SUBSCRIBED_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_is_subscribed, BT_GATT_IS_SUBSCRIBED_RPC_CMD,
			 bt_gatt_is_subscribed_rpc_handler, NULL);

static void bt_gatt_get_mtu_rpc_handler(CborValue *value, void *handler_data)
{

	struct bt_conn *conn;
	uint16_t result;

	conn = bt_rpc_decode_bt_conn(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_gatt_get_mtu(conn);

	ser_rsp_send_uint(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_GET_MTU_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_get_mtu, BT_GATT_GET_MTU_RPC_CMD,
			 bt_gatt_get_mtu_rpc_handler, NULL);

static void bt_gatt_attr_get_handle_rpc_handler(CborValue *value, void *handler_data)
{

	const struct bt_gatt_attr *attr;
	uint16_t result;

	attr = bt_rpc_decode_gatt_attr(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_gatt_attr_get_handle(attr);

	ser_rsp_send_uint(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_ATTR_GET_HANDLE_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_attr_get_handle, BT_GATT_ATTR_GET_HANDLE_RPC_CMD,
			 bt_gatt_attr_get_handle_rpc_handler, NULL);

static void bt_gatt_attr_value_handle_rpc_handler(CborValue *value, void *handler_data)
{

	const struct bt_gatt_attr *attr;
	uint16_t result;

	attr = bt_rpc_decode_gatt_attr(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_gatt_attr_value_handle(attr);

	ser_rsp_send_uint(result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_ATTR_VALUE_HANDLE_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_attr_value_handle, BT_GATT_ATTR_VALUE_HANDLE_RPC_CMD,
			 bt_gatt_attr_value_handle_rpc_handler, NULL);

void bt_gatt_cb_att_mtu_update_call(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 9;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	ser_encode_uint(&ctx.encoder, tx);
	ser_encode_uint(&ctx.encoder, rx);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD,
		&ctx, ser_rsp_decode_void, NULL);
}

static struct bt_gatt_cb bt_gatt_cb_data = {
	.att_mtu_updated = bt_gatt_cb_att_mtu_update_call
};

static void bt_gatt_cb_register_handler(CborValue *value, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(value);

	bt_gatt_cb_register(&bt_gatt_cb_data);

	ser_rsp_send_void();
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_cb_register, BT_LE_GATT_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_gatt_cb_register_handler, NULL);
