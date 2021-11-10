/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Client side of bluetooth API over nRF RPC.
 */

#include <sys/types.h>

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/att.h"
#include "bluetooth/gatt.h"

#include "bt_rpc_common.h"
#include "bt_rpc_gatt_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

static struct bt_uuid const * const uuid_primary = BT_UUID_GATT_PRIMARY;
static struct bt_uuid const * const uuid_secondary = BT_UUID_GATT_SECONDARY;
static struct bt_uuid const * const uuid_chrc = BT_UUID_GATT_CHRC;
static struct bt_uuid const * const uuid_ccc = BT_UUID_GATT_CCC;
static struct bt_uuid const * const uuid_cep = BT_UUID_GATT_CEP;
static struct bt_uuid const * const uuid_cud = BT_UUID_GATT_CUD;
static struct bt_uuid const * const uuid_cpf = BT_UUID_GATT_CPF;

#if !defined(__GNUC__)
#error Attribute read and write default function for services, characteristics and descriptors \
	are implemented only for GCC
#endif

#define GENERIC_ATTR_READ_FUNCTION_CREATE(_name) \
	ssize_t _CONCAT(bt_gatt_attr_read_, _name) (struct bt_conn *conn, \
						    const struct bt_gatt_attr *attr, \
						    void *buf, uint16_t len, uint16_t offset) \
	{ \
		__builtin_unreachable(); \
	}

GENERIC_ATTR_READ_FUNCTION_CREATE(service);
GENERIC_ATTR_READ_FUNCTION_CREATE(chrc);
GENERIC_ATTR_READ_FUNCTION_CREATE(included);
GENERIC_ATTR_READ_FUNCTION_CREATE(ccc);
GENERIC_ATTR_READ_FUNCTION_CREATE(cep);
GENERIC_ATTR_READ_FUNCTION_CREATE(cud);
GENERIC_ATTR_READ_FUNCTION_CREATE(cpf);

ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
	__builtin_unreachable();
}

void bt_rpc_encode_gatt_attr(CborEncoder *encoder, const struct bt_gatt_attr *attr)
{
	uint32_t attr_index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &attr_index);
	__ASSERT(err == 0, "Service attribute not found. Service database might be out of sync");

	ser_encode_uint(encoder, attr_index);
}
const struct bt_gatt_attr *bt_rpc_decode_gatt_attr(CborValue *value)
{
	uint32_t attr_index;

	attr_index = ser_decode_uint(value);

	return bt_rpc_gatt_index_to_attr(attr_index);
}

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t buf_len, uint16_t offset,
			  const void *value, uint16_t value_len)
{
	uint16_t len;

	if (offset > value_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	len = MIN(buf_len, value_len - offset);

	LOG_DBG("handle 0x%04x offset %u length %u", attr->handle, offset, len);

	memcpy(buf, (uint8_t *)value + offset, len);

	return len;
}

static void bt_gatt_complete_func_t_callback_rpc_handler(CborValue *value, void *handler_data)
{
	struct bt_conn *conn;
	bt_gatt_complete_func_t callback_slot;
	void *user_data;

	conn = bt_rpc_decode_bt_conn(value);
	user_data = (void *)ser_decode_uint(value);
	callback_slot = (bt_gatt_complete_func_t)ser_decode_callback_call(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	callback_slot(conn, user_data);

	ser_rsp_send_void();

	return;
decoding_error:
	report_decoding_error(BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_complete_func_t_callback,
			 BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD,
			 bt_gatt_complete_func_t_callback_rpc_handler, NULL);

static void bt_rpc_gatt_ccc_cfg_changed_cb_rpc_handler(CborValue *value, void *handler_data)
{
	uint32_t attr_index;
	uint16_t ccc_value;
	const struct bt_gatt_attr *attr;
	struct _bt_gatt_ccc *ccc;

	attr_index = ser_decode_uint(value);
	ccc_value = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(attr_index);
	if (!attr) {
		return;
	}

	ccc = (struct _bt_gatt_ccc *) attr->user_data;

	if (ccc->cfg_changed) {
		ccc->cfg_changed(attr, ccc_value);
	}

	ser_rsp_send_void();

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_ATTR_READ_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_ccc_cfg_changed_cb,
			 BT_RPC_GATT_CB_CCC_CFG_CHANGED_RPC_CMD,
			 bt_rpc_gatt_ccc_cfg_changed_cb_rpc_handler, NULL);

static void bt_rpc_gatt_attr_read_cb_rpc_handler(CborValue *value, void *handler_data)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_conn *conn;
	struct ser_scratchpad scratchpad;
	size_t buffer_size_max = 9;
	const struct bt_gatt_attr *attr;
	uint32_t service_index;
	ssize_t read_len = 0;
	uint16_t offset;
	uint16_t len;
	uint8_t *buf = NULL;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	conn = bt_rpc_decode_bt_conn(value);
	service_index = ser_decode_uint(value);
	len = ser_decode_uint(value);
	offset = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(service_index);
	if (!attr) {
		LOG_WRN("Service database may not be synchronized with client");
		read_len = BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	} else {
		buf = ser_scratchpad_add(&scratchpad, len);

		if (attr->read) {
			read_len = attr->read(conn, attr, buf, len, offset);
		}

		buffer_size_max += (read_len > 0) ? read_len : 0;
	}

	{
		NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

		ser_encode_int(&ctx.encoder, read_len);
		ser_encode_buffer(&ctx.encoder, buf, read_len);

		nrf_rpc_cbor_rsp_no_err(&ctx);
	}

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_ATTR_READ_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_attr_read_cb, BT_RPC_GATT_CB_ATTR_READ_RPC_CMD,
	bt_rpc_gatt_attr_read_cb_rpc_handler, NULL);

static void bt_rpc_gatt_attr_write_cb_rpc_handler(CborValue *value, void *handler_data)
{
	struct ser_scratchpad scratchpad;
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	int service_index;
	int write_len = 0;
	uint16_t len;
	uint16_t offset;
	uint8_t flags;
	uint8_t *buf;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	conn = bt_rpc_decode_bt_conn(value);
	service_index = ser_decode_int(value);
	len = ser_decode_uint(value);
	offset = ser_decode_uint(value);
	flags = ser_decode_uint(value);
	buf = ser_decode_buffer_into_scratchpad(&scratchpad);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(service_index);
	if (!attr) {
		LOG_WRN("Service database may not be synchronized with client");
		write_len = BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	} else {
		if (attr->write) {
			write_len = attr->write(conn, attr, buf, len, offset, flags);
		}
	}

	ser_rsp_send_int(write_len);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_ATTR_WRITE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_attr_write_cb, BT_RPC_GATT_CB_ATTR_WRITE_RPC_CMD,
	bt_rpc_gatt_attr_write_cb_rpc_handler, NULL);

static int bt_rpc_gatt_start_service(uint8_t service_index, size_t attr_count)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 7;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	ser_encode_uint(&ctx.encoder, service_index);
	ser_encode_uint(&ctx.encoder, attr_count);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_START_SERVICE_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static size_t bt_uuid_buf_size(const struct bt_uuid *uuid)
{
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		return sizeof(struct bt_uuid_16);

	case BT_UUID_TYPE_32:
		return sizeof(struct bt_uuid_32);

	case BT_UUID_TYPE_128:
		return sizeof(struct bt_uuid_128);

	default:
		return 0;
	}
}

static void bt_uuid_enc(CborEncoder *encoder, const struct bt_uuid *data)
{
	switch (data->type) {
	case BT_UUID_TYPE_16:
		ser_encode_buffer(encoder,
			(const struct bt_uuid_16 *)data, sizeof(struct bt_uuid_16));
		break;

	case BT_UUID_TYPE_32:
		ser_encode_buffer(encoder,
			(const struct bt_uuid_32 *)data, sizeof(struct bt_uuid_32));
		break;

	case BT_UUID_TYPE_128:
		ser_encode_buffer(encoder,
			(const struct bt_uuid_128 *)data, sizeof(const struct bt_uuid_128));
		break;

	default:
		ser_encoder_invalid(encoder);
		break;
	}
}

static int bt_rpc_gatt_send_simple_attr(uint8_t special_attr, const struct bt_uuid *uuid,
					uint16_t data)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	buffer_size_max += bt_uuid_buf_size(uuid);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_uuid_enc(&ctx.encoder, uuid);

	ser_encode_uint(&ctx.encoder, special_attr);
	ser_encode_uint(&ctx.encoder, data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static int send_normal_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	uint16_t data = attr->perm;

	if (attr->read) {
		data |= BT_RPC_GATT_ATTR_READ_PRESENT_FLAG;
	}

	if (attr->write) {
		data |= BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG;
	}

	return bt_rpc_gatt_send_simple_attr(special_attr, attr->uuid, data);
}

static int send_service_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_uuid *service_uuid = (struct bt_uuid *)attr->user_data;

	return bt_rpc_gatt_send_simple_attr(special_attr, service_uuid, 0);
}

static int send_chrc_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *) attr->user_data;

	__ASSERT(chrc->value_handle == 0, "Only default value of value_handle is implemented!");

	return bt_rpc_gatt_send_simple_attr(special_attr, chrc->uuid, chrc->properties);
}

static int bt_rpc_gatt_send_desc_attr(uint8_t special_attr, uint16_t param, uint8_t *buffer,
				      size_t size)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 20;

	buffer_size = sizeof(uint8_t) * size;
	buffer_size_max += buffer_size;

	scratchpad_size += SCRATCHPAD_ALIGN(buffer_size);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);
	ser_encode_uint(&ctx.encoder, scratchpad_size);

	ser_encode_uint(&ctx.encoder, special_attr);
	ser_encode_uint(&ctx.encoder, param);
	ser_encode_uint(&ctx.encoder, size);
	ser_encode_buffer(&ctx.encoder, buffer, buffer_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static int send_ccc_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct _bt_gatt_ccc *ccc = (struct _bt_gatt_ccc *)attr->user_data;
	uint16_t data = attr->perm;

	if (ccc->cfg_changed) {
		data |= BT_RPC_GATT_CCC_CFG_CHANGE_PRESENT_FLAG;
	}

	if (ccc->cfg_write) {
		data |= BT_RPC_GATT_CCC_CFG_WRITE_PRESENT_FLAG;
	}

	if (ccc->cfg_match) {
		data |= BT_RPC_GATT_CCC_CFG_MATCH_PRESET_FLAG;
	}

	return bt_rpc_gatt_send_desc_attr(special_attr, data, NULL, 0);
}

static int send_cep_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_gatt_cep *cep = (struct bt_gatt_cep *)attr->user_data;

	return bt_rpc_gatt_send_desc_attr(special_attr, cep->properties, NULL, 0);
}

static int send_cud_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	char *cud = (char *)attr->user_data;

	return bt_rpc_gatt_send_desc_attr(special_attr, attr->perm, (uint8_t *)cud,
					  strlen(cud) + 1);
}

static int send_cpf_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_gatt_cpf *cpf = (struct bt_gatt_cpf *)attr->user_data;

	return bt_rpc_gatt_send_desc_attr(special_attr, 0, (uint8_t *)cpf,
					  sizeof(struct bt_gatt_cpf));
}

static int bt_rpc_gatt_end_service(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_END_SERVICE_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static bool attr_type_check(const struct bt_gatt_attr *attr, const struct bt_uuid *uuid,
			    void *read_func, void *write_func)
{
	return (!bt_uuid_cmp(attr->uuid, uuid) &&
		(attr->read == read_func) &&
		(attr->write == write_func));
}

static uint8_t special_attr_get(const struct bt_gatt_attr *attr)
{
	uint8_t special_attr;

	if (attr_type_check(attr, uuid_primary, bt_gatt_attr_read_service, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_SERVICE;
	} else if (attr_type_check(attr, uuid_secondary, bt_gatt_attr_read_service, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_SECONDARY;
	} else if (attr_type_check(attr, uuid_chrc, bt_gatt_attr_read_chrc, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_CHRC;
	} else if (attr_type_check(attr, uuid_ccc, bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_CCC;
	} else if (attr_type_check(attr, uuid_cep, bt_gatt_attr_read_cep, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_CEP;
	} else if (attr_type_check(attr, uuid_cud, bt_gatt_attr_read_cud, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_CUD;
	} else if (attr_type_check(attr, uuid_cpf, bt_gatt_attr_read_cpf, NULL)) {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_CPF;
	} else {
		special_attr = BT_RPC_GATT_ATTR_SPECIAL_USER;
	}

	return special_attr;
}

static int send_service(const struct bt_gatt_service *svc)
{
	int err;
	uint32_t service_index;
	uint8_t special_attr;
	const struct bt_gatt_attr *attr;

	err = bt_rpc_gatt_add_service(svc, &service_index);
	if (err) {
		return err;
	}

	LOG_DBG("Sending service %d", service_index);

	err = bt_rpc_gatt_start_service(service_index, svc->attr_count);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < svc->attr_count; i++) {

		attr = &svc->attrs[i];

		special_attr = special_attr_get(attr);

		switch (special_attr) {
		case BT_RPC_GATT_ATTR_SPECIAL_USER:
			err = send_normal_attr(0, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_SERVICE:
		case BT_RPC_GATT_ATTR_SPECIAL_SECONDARY:
			err = send_service_attr(special_attr, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_CHRC:
			err = send_chrc_attr(special_attr, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_CCC:
			err = send_ccc_attr(special_attr, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_CEP:
			err = send_cep_attr(special_attr, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_CUD:
			err = send_cud_attr(special_attr, attr);
			break;

		case BT_RPC_GATT_ATTR_SPECIAL_CPF:
			err = send_cpf_attr(special_attr, attr);
			break;

		default:
			err = -EINVAL;
			break;
		}
	}

	if (err) {
		return err;
	}

	return bt_rpc_gatt_end_service();
}

int bt_rpc_gatt_init(void)
{
	int err;

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		err = send_service((const struct bt_gatt_service *)svc);
		if (err) {
			LOG_ERR("Sending static service error: %d", err);
			return err;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
int bt_gatt_service_register(struct bt_gatt_service *svc)
{
	return send_service(svc);
}

int bt_gatt_service_unregister(struct bt_gatt_service *svc)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 3;
	uint16_t svc_index;
	int err;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	err = bt_rpc_gatt_service_to_index(svc, &svc_index);
	if (err) {
		return err;
	}

	ser_encode_uint(&ctx.encoder, svc_index);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD,
				&ctx, ser_rsp_decode_i32, &result);

	if (result) {
		return result;
	}

	return bt_rpc_gatt_remove_service(svc);
}
#endif /* defined(CONFIG_BT_GATT_DYNAMIC_DB) */

size_t bt_gatt_notify_params_buf_size(const struct bt_gatt_notify_params *data)
{

	size_t buffer_size_max = 23;

	buffer_size_max += sizeof(uint8_t) * data->len;

	buffer_size_max += data->len;

	return buffer_size_max;

}

size_t bt_gatt_notify_params_sp_size(const struct bt_gatt_notify_params *data)
{

	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * data->len);

	scratchpad_size += data->len;

	return scratchpad_size;

}

void bt_gatt_notify_params_enc(CborEncoder *encoder, const struct bt_gatt_notify_params *data)
{

	bt_rpc_encode_gatt_attr(encoder, data->attr);
	ser_encode_uint(encoder, data->len);
	ser_encode_buffer(encoder, data->data, sizeof(uint8_t) * data->len);
	ser_encode_callback(encoder, data->func);
	ser_encode_uint(encoder, (uintptr_t)data->user_data);

	if (data->uuid) {
		bt_uuid_enc(encoder, data->uuid);
	} else {
		ser_encode_null(encoder);
	}

}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 8;

	buffer_size_max += bt_gatt_notify_params_buf_size(params);

	scratchpad_size += bt_gatt_notify_params_sp_size(params);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);
	ser_encode_uint(&ctx.encoder, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	bt_gatt_notify_params_enc(&ctx.encoder, params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_NOTIFY_CB_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_GATT_NOTIFY_MULTIPLE)
int bt_gatt_notify_multiple(struct bt_conn *conn, uint16_t num_params,
			    struct bt_gatt_notify_params *params)
{
	int i, ret;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(num_params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	for (i = 0; i < num_params; i++) {
		ret = bt_gatt_notify_cb(conn, &params[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_GATT_NOTIFY_MULTIPLE */

size_t bt_gatt_indicate_params_sp_size(const struct bt_gatt_indicate_params *data)
{

	size_t scratchpad_size = 0;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * data->len);

	scratchpad_size += data->uuid ? bt_uuid_buf_size(data->uuid) : 0;

	return scratchpad_size;

}

size_t bt_gatt_indicate_params_buf_size(const struct bt_gatt_indicate_params *data)
{

	size_t buffer_size_max = 15;

	buffer_size_max += sizeof(uint8_t) * data->len;

	buffer_size_max += data->uuid ? bt_uuid_buf_size(data->uuid) : 1;

	return buffer_size_max;

}

void bt_gatt_indicate_params_enc(CborEncoder *encoder, const struct bt_gatt_indicate_params *data)
{

	bt_rpc_encode_gatt_attr(encoder, data->attr);
	ser_encode_uint(encoder, data->len);
	ser_encode_buffer(encoder, data->data, sizeof(uint8_t) * data->len);
	ser_encode_uint(encoder, data->_ref);

	if (data->uuid) {
		bt_uuid_enc(encoder, data->uuid);
	} else {
		ser_encode_null(encoder);
	}

}

int bt_gatt_indicate(struct bt_conn *conn, struct bt_gatt_indicate_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 13;
	uintptr_t params_addr = (uintptr_t)params;

	buffer_size_max += bt_gatt_indicate_params_buf_size(params);
	scratchpad_size += bt_gatt_indicate_params_sp_size(params);

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);
	ser_encode_uint(&ctx.encoder, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	bt_gatt_indicate_params_enc(&ctx.encoder, params);
	ser_encode_uint(&ctx.encoder, params_addr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

static void bt_gatt_indicate_func_t_callback_rpc_handler(CborValue *value, void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;
	struct bt_gatt_indicate_params *params;

	conn = bt_rpc_decode_bt_conn(value);
	err = ser_decode_uint(value);
	params = (struct bt_gatt_indicate_params *) ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	if (params->func) {
		params->func(conn, params, err);
	}

	ser_rsp_send_void();

	return;
decoding_error:
	report_decoding_error(BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_indicate_func_t_callback,
			 BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD,
			 bt_gatt_indicate_func_t_callback_rpc_handler, NULL);

static void bt_gatt_indicate_params_destroy_t_callback_rpc_handler(CborValue *value,
								   void *handler_data)
{
	struct bt_gatt_indicate_params *params;

	params = (struct bt_gatt_indicate_params *)ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	if (params->destroy) {
		params->destroy(params);
	}

	ser_rsp_send_void();

	return;
decoding_error:
	report_decoding_error(BT_GATT_INDICATE_PARAMS_DESTROY_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_indicate_params_destroy_t_callback,
			 BT_GATT_INDICATE_PARAMS_DESTROY_T_CALLBACK_RPC_CMD,
			 bt_gatt_indicate_params_destroy_t_callback_rpc_handler, NULL);

bool bt_gatt_is_subscribed(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint16_t ccc_value)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;
	size_t buffer_size_max = 11;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);
	bt_rpc_encode_gatt_attr(&ctx.encoder, attr);
	ser_encode_uint(&ctx.encoder, ccc_value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_IS_SUBSCRIBED_RPC_CMD,
		&ctx, ser_rsp_decode_bool, &result);

	return result;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx.encoder, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_GET_MTU_RPC_CMD,
		&ctx, ser_rsp_decode_u16, &result);

	return result;
}

static uint8_t find_next(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct bt_gatt_attr **next = user_data;

	*next = (struct bt_gatt_attr *)attr;

	return BT_GATT_ITER_STOP;
}
struct bt_gatt_attr *bt_gatt_attr_next(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *next = NULL;
	uint16_t handle = bt_gatt_attr_get_handle(attr);

	bt_gatt_foreach_attr(handle + 1, handle + 1, find_next, &next);

	return next;
}

void bt_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
			       const struct bt_uuid *uuid,
			       const void *attr_data, uint16_t num_matches,
			       bt_gatt_attr_func_t func,
			       void *user_data)
{
	bt_rpc_gatt_foreach_attr_type(start_handle, end_handle, uuid, attr_data,
				      num_matches, func, user_data);
}

uint16_t bt_gatt_attr_get_handle(const struct bt_gatt_attr *attr)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_gatt_attr(&ctx.encoder, attr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_ATTR_GET_HANDLE_RPC_CMD,
		&ctx, ser_rsp_decode_u16, &result);

	return result;
}

uint16_t bt_gatt_attr_value_handle(const struct bt_gatt_attr *attr)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t result;
	size_t buffer_size_max = 5;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	bt_rpc_encode_gatt_attr(&ctx.encoder, attr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_ATTR_VALUE_HANDLE_RPC_CMD,
		&ctx, ser_rsp_decode_u16, &result);

	return result;
}

struct bt_gatt_attr *bt_gatt_find_by_uuid(const struct bt_gatt_attr *attr,
					  uint16_t attr_count,
					  const struct bt_uuid *uuid)
{
	struct bt_gatt_attr *found = NULL;
	uint16_t start_handle = bt_gatt_attr_value_handle(attr);
	uint16_t end_handle = start_handle && attr_count ?
			      start_handle + attr_count : 0xffff;

	bt_gatt_foreach_attr_type(start_handle, end_handle, uuid, NULL, 1,
				  find_next, &found);

	return found;
}

sys_slist_t gatt_cbs = SYS_SLIST_STATIC_INIT(&gatt_cbs);

static void bt_le_gatt_cb_register_on_remote(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;

	NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_GATT_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, ser_rsp_decode_void, NULL);
}

void bt_gatt_cb_register(struct bt_gatt_cb *cb)
{
	bool register_on_remote;

	register_on_remote = sys_slist_is_empty(&gatt_cbs);

	sys_slist_append(&gatt_cbs, &cb->node);

	if (register_on_remote) {
		bt_le_gatt_cb_register_on_remote();
	}
}

static void bt_gatt_cb_att_mtu_update_call_rpc_handler(CborValue *value, void *handler_data)
{
	struct bt_gatt_cb *listener;
	struct bt_conn *conn;
	uint16_t tx;
	uint16_t rx;

	conn = bt_rpc_decode_bt_conn(value);
	tx = ser_decode_uint(value);
	rx = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&gatt_cbs, listener, node) {
		listener->att_mtu_updated(conn, tx, rx);
	}

	ser_rsp_send_void();

	return;
decoding_error:
	report_decoding_error(BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_cb_att_mtu_update_call,
			 BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD,
			 bt_gatt_cb_att_mtu_update_call_rpc_handler, NULL);
