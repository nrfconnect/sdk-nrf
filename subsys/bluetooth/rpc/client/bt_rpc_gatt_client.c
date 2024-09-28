/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Client side of bluetooth API over nRF RPC.
 */

#include <sys/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>

#include "bt_rpc_common.h"
#include "bt_rpc_gatt_common.h"
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include "nrf_rpc_cbor.h"

#include <zephyr/logging/log.h>

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

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
#error "CONFIG_BT_GATT_AUTO_DISCOVER_CCC is not supported by the RPC GATT"
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

void bt_rpc_encode_gatt_attr(struct nrf_rpc_cbor_ctx *encoder, const struct bt_gatt_attr *attr)
{
	uint32_t attr_index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &attr_index);
	__ASSERT(err == 0, "Service attribute not found. Service database might be out of sync");

	nrf_rpc_encode_uint(encoder, attr_index);
}
const struct bt_gatt_attr *bt_rpc_decode_gatt_attr(struct nrf_rpc_cbor_ctx *ctx)
{
	uint32_t attr_index;

	attr_index = nrf_rpc_decode_uint(ctx);

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

static void bt_gatt_complete_func_t_callback_rpc_handler(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct bt_conn *conn;
	bt_gatt_complete_func_t callback_slot;
	void *user_data;

	conn = bt_rpc_decode_bt_conn(ctx);
	user_data = (void *)nrf_rpc_decode_uint(ctx);
	callback_slot = (bt_gatt_complete_func_t)nrf_rpc_decode_callback_call(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	callback_slot(conn, user_data);

	nrf_rpc_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_complete_func_t_callback,
			 BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD,
			 bt_gatt_complete_func_t_callback_rpc_handler, NULL);

static void bt_rpc_gatt_ccc_cfg_changed_cb_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	uint32_t attr_index;
	uint16_t ccc_value;
	const struct bt_gatt_attr *attr;
	struct _bt_gatt_ccc *ccc;

	attr_index = nrf_rpc_decode_uint(ctx);
	ccc_value = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
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

	nrf_rpc_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_CCC_CFG_CHANGED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_ccc_cfg_changed_cb,
			 BT_RPC_GATT_CB_CCC_CFG_CHANGED_RPC_CMD,
			 bt_rpc_gatt_ccc_cfg_changed_cb_rpc_handler, NULL);

static void bt_rpc_gatt_ccc_cfg_write_cb_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t attr_index;
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	struct _bt_gatt_ccc *ccc;
	uint16_t ccc_value;
	ssize_t write_len = 0;

	conn = bt_rpc_decode_bt_conn(ctx);
	attr_index = nrf_rpc_decode_uint(ctx);
	ccc_value = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(attr_index);
	if (!attr) {
		return;
	}

	ccc = (struct _bt_gatt_ccc *) attr->user_data;

	if (ccc->cfg_write) {
		write_len = ccc->cfg_write(conn, attr, ccc_value);
	}

	nrf_rpc_rsp_send_int(group, write_len);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_CCC_CFG_WRITE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_ccc_cfg_write_cb,
			 BT_RPC_GATT_CB_CCC_CFG_WRITE_RPC_CMD,
			 bt_rpc_gatt_ccc_cfg_write_cb_rpc_handler, NULL);

static void bt_rpc_gatt_ccc_cfg_match_cb_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t attr_index;
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	struct _bt_gatt_ccc *ccc;
	bool match = false;

	conn = bt_rpc_decode_bt_conn(ctx);
	attr_index = nrf_rpc_decode_int(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(attr_index);
	if (!attr) {
		return;
	}

	ccc = (struct _bt_gatt_ccc *) attr->user_data;

	if (ccc->cfg_match) {
		match = ccc->cfg_match(conn, attr);
	}

	nrf_rpc_rsp_send_bool(group, match);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_CCC_CFG_MATCH_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_ccc_cfg_match_cb,
			 BT_RPC_GATT_CB_CCC_CFG_MATCH_RPC_CMD,
			 bt_rpc_gatt_ccc_cfg_match_cb_rpc_handler, NULL);

static void bt_rpc_gatt_attr_read_cb_rpc_handler(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct nrf_rpc_scratchpad scratchpad;
	size_t buffer_size_max = 9;
	const struct bt_gatt_attr *attr;
	uint32_t service_index;
	ssize_t read_len = 0;
	uint16_t offset;
	uint16_t len;
	uint8_t *buf = NULL;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	service_index = nrf_rpc_decode_uint(ctx);
	len = nrf_rpc_decode_uint(ctx);
	offset = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	attr = bt_rpc_gatt_index_to_attr(service_index);
	if (!attr) {
		LOG_WRN("Service database may not be synchronized with client");
		read_len = BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	} else {
		buf = nrf_rpc_scratchpad_add(&scratchpad, len);

		if (attr->read) {
			read_len = attr->read(conn, attr, buf, len, offset);
		}

		buffer_size_max += (read_len > 0) ? read_len : 0;
	}

	{
		struct nrf_rpc_cbor_ctx ectx;

		NRF_RPC_CBOR_ALLOC(group, ectx, buffer_size_max);

		nrf_rpc_encode_int(&ectx, read_len);
		nrf_rpc_encode_buffer(&ectx, buf, read_len);

		nrf_rpc_cbor_rsp_no_err(group, &ectx);
	}

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_CB_ATTR_READ_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_attr_read_cb, BT_RPC_GATT_CB_ATTR_READ_RPC_CMD,
	bt_rpc_gatt_attr_read_cb_rpc_handler, NULL);

static void bt_rpc_gatt_attr_write_cb_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_scratchpad scratchpad;
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	int service_index;
	int write_len = 0;
	uint16_t len;
	uint16_t offset;
	uint8_t flags;
	uint8_t *buf;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	service_index = nrf_rpc_decode_int(ctx);
	len = nrf_rpc_decode_uint(ctx);
	offset = nrf_rpc_decode_uint(ctx);
	flags = nrf_rpc_decode_uint(ctx);
	buf = nrf_rpc_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
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

	nrf_rpc_rsp_send_int(group, write_len);

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_encode_uint(&ctx, service_index);
	nrf_rpc_encode_uint(&ctx, attr_count);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_START_SERVICE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

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

static size_t bt_uuid_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_uuid *uuid)
{
	size_t size = 0;

	if (uuid != NULL) {
		switch (uuid->type) {
		case BT_UUID_TYPE_16:
			size = sizeof(struct bt_uuid_16);
			break;
		case BT_UUID_TYPE_32:
			size = sizeof(struct bt_uuid_32);
			break;
		case BT_UUID_TYPE_128:
			size = sizeof(struct bt_uuid_128);
			break;
		default:
			if (encoder != NULL) {
				nrf_rpc_encoder_invalid(encoder);
			}
			return 1;
		}
	}
	if (encoder != NULL) {
		nrf_rpc_encode_buffer(encoder, uuid, size);
	}
	return 1 + size;
}

#if defined(CONFIG_BT_GATT_CLIENT)

static struct bt_uuid *bt_uuid_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_uuid *uuid)
{
	return (struct bt_uuid *)nrf_rpc_decode_buffer(ctx, uuid, sizeof(struct bt_uuid_128));
}

#endif /* CONFIG_BT_GATT_CLIENT */

static int bt_rpc_gatt_send_simple_attr(uint8_t special_attr, const struct bt_uuid *uuid,
					uint16_t data)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 5;

	buffer_size_max += bt_uuid_buf_size(uuid);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_uuid_enc(&ctx, uuid);

	nrf_rpc_encode_uint(&ctx, special_attr);
	nrf_rpc_encode_uint(&ctx, data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

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

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(buffer_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	nrf_rpc_encode_uint(&ctx, scratchpad_size);

	nrf_rpc_encode_uint(&ctx, special_attr);
	nrf_rpc_encode_uint(&ctx, param);
	nrf_rpc_encode_uint(&ctx, size);
	nrf_rpc_encode_buffer(&ctx, buffer, buffer_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_END_SERVICE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	err = bt_rpc_gatt_service_to_index(svc, &svc_index);
	if (err) {
		return err;
	}

	nrf_rpc_encode_uint(&ctx, svc_index);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD,
				&ctx, nrf_rpc_rsp_decode_i32, &result);

	if (result) {
		return result;
	}

	return bt_rpc_gatt_remove_service(svc);
}
#endif /* defined(CONFIG_BT_GATT_DYNAMIC_DB) */

static size_t bt_gatt_notify_params_buf_size(const struct bt_gatt_notify_params *data)
{
	size_t buffer_size_max = 23;

	buffer_size_max += sizeof(uint8_t) * data->len;

	buffer_size_max += data->len;

	return buffer_size_max;
}

static size_t bt_gatt_notify_params_sp_size(const struct bt_gatt_notify_params *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(sizeof(uint8_t) * data->len);

	scratchpad_size += data->len;

	return scratchpad_size;
}

static void bt_gatt_notify_params_enc(struct nrf_rpc_cbor_ctx *encoder,
				      const struct bt_gatt_notify_params *data)
{
	bt_rpc_encode_gatt_attr(encoder, data->attr);
	nrf_rpc_encode_uint(encoder, data->len);
	nrf_rpc_encode_buffer(encoder, data->data, sizeof(uint8_t) * data->len);
	nrf_rpc_encode_callback(encoder, data->func);
	nrf_rpc_encode_uint(encoder, (uintptr_t)data->user_data);

	if (data->uuid) {
		bt_uuid_enc(encoder, data->uuid);
	} else {
		nrf_rpc_encode_null(encoder);
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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	nrf_rpc_encode_uint(&ctx, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_gatt_notify_params_enc(&ctx, params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_NOTIFY_CB_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

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

static size_t bt_gatt_indicate_params_sp_size(const struct bt_gatt_indicate_params *data)
{
	size_t scratchpad_size = 0;

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(sizeof(uint8_t) * data->len);

	scratchpad_size += data->uuid ? bt_uuid_buf_size(data->uuid) : 0;

	return scratchpad_size;
}

static size_t bt_gatt_indicate_params_buf_size(const struct bt_gatt_indicate_params *data)
{
	size_t buffer_size_max = 15;

	buffer_size_max += sizeof(uint8_t) * data->len;
	buffer_size_max += data->uuid ? bt_uuid_buf_size(data->uuid) : 1;

	return buffer_size_max;
}

static void bt_gatt_indicate_params_enc(struct nrf_rpc_cbor_ctx *encoder,
					const struct bt_gatt_indicate_params *data)
{
	bt_rpc_encode_gatt_attr(encoder, data->attr);
	nrf_rpc_encode_uint(encoder, data->len);
	nrf_rpc_encode_buffer(encoder, data->data, sizeof(uint8_t) * data->len);
	nrf_rpc_encode_uint(encoder, data->_ref);

	if (data->uuid) {
		bt_uuid_enc(encoder, data->uuid);
	} else {
		nrf_rpc_encode_null(encoder);
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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	nrf_rpc_encode_uint(&ctx, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_gatt_indicate_params_enc(&ctx, params);
	nrf_rpc_encode_uint(&ctx, params_addr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static void bt_gatt_indicate_func_t_callback_rpc_handler(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;
	struct bt_gatt_indicate_params *params;

	conn = bt_rpc_decode_bt_conn(ctx);
	err = nrf_rpc_decode_uint(ctx);
	params = (struct bt_gatt_indicate_params *) nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (params->func) {
		params->func(conn, params, err);
	}

	nrf_rpc_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_indicate_func_t_callback,
			 BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD,
			 bt_gatt_indicate_func_t_callback_rpc_handler, NULL);

static void bt_gatt_indicate_params_destroy_t_callback_rpc_handler(
						const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_gatt_indicate_params *params;

	params = (struct bt_gatt_indicate_params *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (params->destroy) {
		params->destroy(params);
	}

	nrf_rpc_rsp_send_void(group);

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_rpc_encode_gatt_attr(&ctx, attr);
	nrf_rpc_encode_uint(&ctx, ccc_value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_IS_SUBSCRIBED_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_bool, &result);

	return result;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t result;
	size_t buffer_size_max = 3;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_GET_MTU_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_u16, &result);

	return result;
}

#if defined(CONFIG_BT_GATT_CLIENT)

static void bt_gatt_exchange_mtu_callback_rpc_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;
	struct bt_gatt_exchange_params *params;

	conn = bt_rpc_decode_bt_conn(ctx);
	err = nrf_rpc_decode_uint(ctx);
	params = (struct bt_gatt_exchange_params *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	params->func(conn, err, params);

	nrf_rpc_rsp_send_void(group);

	return;

decoding_error:
	report_decoding_error(BT_GATT_EXCHANGE_MTU_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_exchange_mtu_callback,
	BT_GATT_EXCHANGE_MTU_CALLBACK_RPC_CMD, bt_gatt_exchange_mtu_callback_rpc_handler, NULL);

int bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 8);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_EXCHANGE_MTU_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

#endif /* CONFIG_BT_GATT_CLIENT */

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

	if (attr != NULL && attr->handle != 0) {
		return attr->handle;
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_gatt_attr(&ctx, attr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_ATTR_GET_HANDLE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_u16, &result);

	return result;
}

uint16_t bt_gatt_attr_value_handle(const struct bt_gatt_attr *attr)
{
	uint16_t result = 0;
	struct bt_gatt_chrc *chrc;

	if (attr != NULL && bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		chrc = attr->user_data;
		if (chrc != NULL && chrc->value_handle != 0) {
			result = chrc->value_handle;
		} else {
			result = bt_gatt_attr_get_handle(attr) + 1;
		}
	}

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_GATT_CB_REGISTER_ON_REMOTE_RPC_CMD,
				&ctx, nrf_rpc_rsp_decode_void, NULL);
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

static void bt_gatt_cb_att_mtu_update_call_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct bt_gatt_cb *listener;
	struct bt_conn *conn;
	uint16_t tx;
	uint16_t rx;

	conn = bt_rpc_decode_bt_conn(ctx);
	tx = nrf_rpc_decode_uint(ctx);
	rx = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&gatt_cbs, listener, node) {
		listener->att_mtu_updated(conn, tx, rx);
	}

	nrf_rpc_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_cb_att_mtu_update_call,
			 BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD,
			 bt_gatt_cb_att_mtu_update_call_rpc_handler, NULL);

#if defined(CONFIG_BT_GATT_CLIENT)

static size_t bt_gatt_discover_params_buf_size(const struct bt_gatt_discover_params *data)
{
	return bt_uuid_enc(NULL, data->uuid) + 9;
}

static void bt_gatt_discover_params_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_gatt_discover_params *data)
{
	bt_uuid_enc(encoder, data->uuid);
	nrf_rpc_encode_uint(encoder, data->start_handle);
	nrf_rpc_encode_uint(encoder, data->end_handle);
	nrf_rpc_encode_uint(encoder, data->type);
}

int bt_gatt_discover(struct bt_conn *conn, struct bt_gatt_discover_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 8 + bt_gatt_discover_params_buf_size(params));

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_gatt_discover_params_enc(&ctx, params);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_DISCOVER_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static void bt_gatt_discover_callback_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	uintptr_t params_pointer;
	uint8_t result;
	struct bt_gatt_discover_params *params;
	struct bt_uuid_16 *attr_uuid_16;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_128 _uuid_max;
	} uuid_buffers[2];
	union {
		struct bt_gatt_service_val service;
		struct bt_gatt_include include;
		struct bt_gatt_chrc chrc;
	} user_data;
	struct bt_gatt_attr attr_instance = {
		.user_data = &user_data,
	};
	struct bt_gatt_attr *attr = &attr_instance;

	conn = bt_rpc_decode_bt_conn(ctx);
	params_pointer = nrf_rpc_decode_uint(ctx);
	params = (struct bt_gatt_discover_params *)params_pointer;

	if (nrf_rpc_decode_is_null(ctx)) {
		attr = NULL;
	} else {
		attr->uuid = bt_uuid_dec(ctx, &uuid_buffers[0].uuid);
		attr->handle = nrf_rpc_decode_uint(ctx);
		attr_uuid_16 = (struct bt_uuid_16 *)attr->uuid;
		if (nrf_rpc_decode_is_null(ctx)) {
			attr->user_data = NULL;
		} else if (attr->uuid == NULL || attr->uuid->type != BT_UUID_TYPE_16) {
			LOG_ERR("Invalid attribute UUID");
			goto decoding_done_with_error;
		} else if (attr_uuid_16->val == BT_UUID_GATT_PRIMARY_VAL ||
			attr_uuid_16->val == BT_UUID_GATT_SECONDARY_VAL) {
			user_data.service.uuid = bt_uuid_dec(ctx, &uuid_buffers[1].uuid);
			user_data.service.end_handle = nrf_rpc_decode_uint(ctx);
		} else if (attr_uuid_16->val == BT_UUID_GATT_INCLUDE_VAL) {
			user_data.include.uuid = bt_uuid_dec(ctx, &uuid_buffers[1].uuid);
			user_data.include.start_handle = nrf_rpc_decode_uint(ctx);
			user_data.include.end_handle = nrf_rpc_decode_uint(ctx);
		} else if (attr_uuid_16->val == BT_UUID_GATT_CHRC_VAL) {
			user_data.chrc.uuid = bt_uuid_dec(ctx, &uuid_buffers[1].uuid);
			user_data.chrc.value_handle = nrf_rpc_decode_uint(ctx);
			user_data.chrc.properties = nrf_rpc_decode_uint(ctx);
		} else {
			LOG_ERR("Unsupported attribute UUID");
			goto decoding_done_with_error;
		}
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = params->func(conn, attr, params);

	nrf_rpc_rsp_send_uint(group, result);

	return;

decoding_done_with_error:
	nrf_rpc_decoding_done_and_check(group, ctx);
decoding_error:
	report_decoding_error(BT_GATT_DISCOVER_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_discover_callback, BT_GATT_DISCOVER_CALLBACK_RPC_CMD,
	bt_gatt_discover_callback_rpc_handler, NULL);

static size_t bt_gatt_read_params_buf_size(const struct bt_gatt_read_params *data)
{
	size_t size = 10;

	if (data->handle_count == 0) {
		size += 6 + bt_uuid_enc(NULL, data->by_uuid.uuid);
	} else if (data->handle_count == 1) {
		size += 6;
	} else {
		size += 6 + sizeof(data->multiple.handles[0]) * data->handle_count;
	}
	return size;
}

static void bt_gatt_read_params_enc(struct nrf_rpc_cbor_ctx *encoder,
				    const struct bt_gatt_read_params *data)
{
	nrf_rpc_encode_uint(encoder, data->handle_count);
	if (data->handle_count == 0) {
		nrf_rpc_encode_uint(encoder, data->by_uuid.start_handle);
		nrf_rpc_encode_uint(encoder, data->by_uuid.end_handle);
		bt_uuid_enc(encoder, data->by_uuid.uuid);
	} else if (data->handle_count == 1) {
		nrf_rpc_encode_uint(encoder, data->single.handle);
		nrf_rpc_encode_uint(encoder, data->single.offset);
	} else {
		nrf_rpc_encode_buffer(encoder, data->multiple.handles,
				  sizeof(data->multiple.handles[0]) * data->handle_count);
		nrf_rpc_encode_bool(encoder, data->multiple.variable);
	}
}

int bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 8 + bt_gatt_read_params_buf_size(params));

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_gatt_read_params_enc(&ctx, params);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_READ_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static void bt_gatt_read_callback_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_scratchpad scratchpad;
	struct bt_conn *conn;
	uintptr_t params_pointer;
	uint8_t err;
	uint8_t result;
	struct bt_gatt_read_params *params;
	void *data;
	size_t length;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	err = nrf_rpc_decode_uint(ctx);
	params_pointer = nrf_rpc_decode_uint(ctx);
	params = (struct bt_gatt_read_params *)params_pointer;

	data = nrf_rpc_decode_buffer_into_scratchpad(&scratchpad, &length);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = params->func(conn, err, params, data, (uint16_t)length);

	nrf_rpc_rsp_send_uint(group, result);

	return;

decoding_error:
	report_decoding_error(BT_GATT_READ_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_read_callback, BT_GATT_READ_CALLBACK_RPC_CMD,
	bt_gatt_read_callback_rpc_handler, NULL);

static size_t bt_gatt_write_params_buf_size(const struct bt_gatt_write_params *data)
{
	return 11 + data->length;
}

static void bt_gatt_write_params_enc(struct nrf_rpc_cbor_ctx *encoder,
	const struct bt_gatt_write_params *data)
{
	nrf_rpc_encode_buffer(encoder, data->data, data->length);
	nrf_rpc_encode_uint(encoder, data->handle);
	nrf_rpc_encode_uint(encoder, data->offset);
}

int bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 8 + bt_gatt_write_params_buf_size(params));

	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_gatt_write_params_enc(&ctx, params);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_WRITE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static void bt_gatt_write_callback_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;
	struct bt_gatt_write_params *params;

	conn = bt_rpc_decode_bt_conn(ctx);
	err = nrf_rpc_decode_uint(ctx);
	params = (struct bt_gatt_write_params *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	params->func(conn, err, params);

	nrf_rpc_rsp_send_void(group);

	return;

decoding_error:
	report_decoding_error(BT_GATT_WRITE_CALLBACK_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_write_callback, BT_GATT_WRITE_CALLBACK_RPC_CMD,
	bt_gatt_write_callback_rpc_handler, NULL);

int bt_gatt_write_without_response_cb(struct bt_conn *conn, uint16_t handle,
				      const void *data, uint16_t length,
				      bool sign, bt_gatt_complete_func_t func,
				      void *user_data)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t _data_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 30;

	_data_size = sizeof(uint8_t) * length;
	buffer_size_max += _data_size;

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(_data_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	nrf_rpc_encode_uint(&ctx, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, handle);
	nrf_rpc_encode_uint(&ctx, length);
	nrf_rpc_encode_buffer(&ctx, data, _data_size);
	nrf_rpc_encode_bool(&ctx, sign);
	nrf_rpc_encode_callback(&ctx, func);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)user_data);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_WRITE_WITHOUT_RESPONSE_CB_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static const size_t bt_gatt_subscribe_params_buf_size =
	/* Placeholder for the notify callback. */
	1 +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, subscribe) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, value_handle) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, ccc_handle) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, value) +
#if defined(CONFIG_BT_SMP)
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, min_security) +
#endif
	/* Placeholder for the flags field */
	2;

static void bt_gatt_subscribe_params_enc(struct nrf_rpc_cbor_ctx *encoder,
					 const struct bt_gatt_subscribe_params *data)
{
	nrf_rpc_encode_bool(encoder, data->notify != NULL);
	nrf_rpc_encode_callback(encoder, data->subscribe);
	nrf_rpc_encode_uint(encoder, data->value_handle);
	nrf_rpc_encode_uint(encoder, data->ccc_handle);
	nrf_rpc_encode_uint(encoder, data->value);
#if defined(CONFIG_BT_SMP)
	nrf_rpc_encode_uint(encoder, data->min_security);
#endif /* defined(CONFIG_BT_SMP) */
	nrf_rpc_encode_uint(encoder, (uint16_t)atomic_get(data->flags));
}

int bt_gatt_subscribe(struct bt_conn *conn,
		      struct bt_gatt_subscribe_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 8;

	buffer_size_max += bt_gatt_subscribe_params_buf_size;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);
	bt_gatt_subscribe_params_enc(&ctx, params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_SUBSCRIBE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

int bt_gatt_resubscribe(uint8_t id, const bt_addr_le_t *peer,
			struct bt_gatt_subscribe_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 9;

	buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;

	buffer_size_max += bt_gatt_subscribe_params_buf_size;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_encode_uint(&ctx, id);
	nrf_rpc_encode_buffer(&ctx, peer, sizeof(bt_addr_le_t));

	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);
	bt_gatt_subscribe_params_enc(&ctx, params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_RESUBSCRIBE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

int bt_gatt_unsubscribe(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 8;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_UNSUBSCRIBE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static int bt_rpc_gatt_subscribe_flag_update(struct bt_gatt_subscribe_params *params,
					     uint32_t flags_bit, int val)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 15;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);
	nrf_rpc_encode_uint(&ctx, flags_bit);
	nrf_rpc_encode_int(&ctx, val);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SUBSCRIBE_FLAG_UPDATE_RPC_CMD,
		&ctx, nrf_rpc_rsp_decode_i32, &result);

	return result;
}

int bt_rpc_gatt_subscribe_flag_set(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	return bt_rpc_gatt_subscribe_flag_update(params, flags_bit, 1);
}

int bt_rpc_gatt_subscribe_flag_clear(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	return bt_rpc_gatt_subscribe_flag_update(params, flags_bit, 0);
}

int bt_rpc_gatt_subscribe_flag_get(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	return bt_rpc_gatt_subscribe_flag_update(params, flags_bit, -1);
}

static void bt_gatt_subscribe_params_notify_rpc_handler(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	struct bt_conn *conn;
	struct bt_gatt_subscribe_params *params;
	size_t length;
	uint8_t *data;
	uint8_t result = BT_GATT_ITER_CONTINUE;
	struct nrf_rpc_scratchpad scratchpad;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	params = (struct bt_gatt_subscribe_params *)nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_buffer_into_scratchpad(&scratchpad, &length);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (params->notify != NULL) {
		result = params->notify(conn, params, data, (uint16_t)length);
	}

	nrf_rpc_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_SUBSCRIBE_PARAMS_NOTIFY_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_subscribe_params_notify,
	BT_GATT_SUBSCRIBE_PARAMS_NOTIFY_RPC_CMD, bt_gatt_subscribe_params_notify_rpc_handler, NULL);

static void bt_gatt_subscribe_params_subscribe_rpc_handler(const struct nrf_rpc_group *group,
							   struct nrf_rpc_cbor_ctx *ctx,
							   void *handler_data)
{
	struct bt_conn *conn;
	uint8_t err;
	struct bt_gatt_subscribe_params params;
	struct bt_gatt_subscribe_params *params_ptr;
	struct nrf_rpc_scratchpad scratchpad;
	bt_gatt_subscribe_func_t func;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	err = nrf_rpc_decode_uint(ctx);
	if (nrf_rpc_decode_is_null(ctx)) {
		params_ptr = NULL;
	} else {
		params.value_handle = nrf_rpc_decode_uint(ctx);
		params.ccc_handle = nrf_rpc_decode_uint(ctx);
		params.value = nrf_rpc_decode_uint(ctx);
#if defined(CONFIG_BT_SMP)
		params.min_security = nrf_rpc_decode_uint(ctx);
#endif /* defined(CONFIG_BT_SMP) */
		atomic_set(params.flags, (atomic_val_t)nrf_rpc_decode_uint(ctx));
		params_ptr = &params;
	}
	func = (bt_gatt_subscribe_func_t)nrf_rpc_decode_callback_call(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	if (func != NULL) {
		func(conn, err, params_ptr);
	}
	nrf_rpc_rsp_send_void(group);

	return;
decoding_error:
	report_decoding_error(BT_GATT_SUBSCRIBE_PARAMS_SUBSCRIBE_RPC_CMD, handler_data);
}


NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_subscribe_params_subscribe,
	BT_GATT_SUBSCRIBE_PARAMS_SUBSCRIBE_RPC_CMD,
	bt_gatt_subscribe_params_subscribe_rpc_handler, NULL);

#endif /* CONFIG_BT_GATT_CLIENT */
