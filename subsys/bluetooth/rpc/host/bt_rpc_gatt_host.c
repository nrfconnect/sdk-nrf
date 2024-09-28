/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_gatt_common.h"
#include "bt_rpc_common.h"
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <zephyr/logging/log.h>

#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
#error "CONFIG_BT_GATT_AUTO_DISCOVER_CCC is not supported by the RPC GATT"
#endif

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

struct remote_svc {
	struct bt_gatt_service *service;
	size_t attr_max;
	uint32_t index;
};

struct bt_gatt_discover_container {
	struct bt_gatt_discover_params params;
	uintptr_t remote_pointer;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_128 _max_uuid_128;
	};
};

struct bt_gatt_read_container {
	struct bt_gatt_read_params params;
	uintptr_t remote_pointer;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_128 _max_uuid_128;
	};
	uint16_t handles[0];
};

struct bt_gatt_write_container {
	struct bt_gatt_write_params params;
	uintptr_t remote_pointer;
	uint8_t data[0];
};

struct bt_gatt_subscribe_container {
	struct bt_gatt_subscribe_params params;
	uintptr_t remote_pointer;
	sys_snode_t node;
};

#if defined(CONFIG_BT_GATT_CLIENT)

static sys_slist_t subscribe_containers = SYS_SLIST_STATIC_INIT(&subscribe_containers);

K_MUTEX_DEFINE(subscribe_containers_mutex);

#endif

static struct bt_uuid const *const uuid_primary = BT_UUID_GATT_PRIMARY;
static struct bt_uuid const *const uuid_secondary = BT_UUID_GATT_SECONDARY;
static struct bt_uuid const *const uuid_chrc = BT_UUID_GATT_CHRC;
static struct bt_uuid const *const uuid_ccc = BT_UUID_GATT_CCC;
static struct bt_uuid const *const uuid_cep = BT_UUID_GATT_CEP;
static struct bt_uuid const *const uuid_cud = BT_UUID_GATT_CUD;
static struct bt_uuid const *const uuid_cpf = BT_UUID_GATT_CPF;

static uint32_t gatt_buffer_data[DIV_ROUND_UP(CONFIG_BT_RPC_GATT_BUFFER_SIZE, sizeof(uint32_t))];
static struct net_buf_simple gatt_buffer = {.data = (uint8_t *)gatt_buffer_data,
					    .len = 0,
					    .size = sizeof(gatt_buffer_data),
					    .__buf = (uint8_t *)gatt_buffer_data};

static struct remote_svc current_service;

static inline void *bt_rpc_gatt_add(struct net_buf_simple *buf, size_t size)
{
	return net_buf_simple_add(buf, WB_UP(size));
}

void bt_rpc_encode_gatt_attr(struct nrf_rpc_cbor_ctx *encoder, const struct bt_gatt_attr *attr)
{
	uint32_t attr_index;
	int err;

	err = bt_rpc_gatt_attr_to_index(attr, &attr_index);
	__ASSERT(err = 0, "Service attribute not found. Service database might be out of sync");

	nrf_rpc_encode_uint(encoder, attr_index);
}

const struct bt_gatt_attr *bt_rpc_decode_gatt_attr(struct nrf_rpc_cbor_ctx *ctx)
{
	uint32_t attr_index;

	attr_index = nrf_rpc_decode_uint(ctx);

	return bt_rpc_gatt_index_to_attr(attr_index);
}

static struct bt_uuid *bt_uuid_svc_dec(struct nrf_rpc_cbor_ctx *ctx)
{
	struct bt_uuid *uuid;
	size_t buffer_size;
	const void *buffer_ptr = nrf_rpc_decode_buffer_ptr_and_size(ctx, &buffer_size);

	if (buffer_ptr == NULL) {
		return NULL;
	}

	if (buffer_size == sizeof(struct bt_uuid_16)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_16));
	} else if (buffer_size == sizeof(struct bt_uuid_32)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_32));
	} else if (buffer_size == sizeof(struct bt_uuid_128)) {
		uuid = bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_uuid_128));
	} else {
		nrf_rpc_decoder_invalid(ctx, ZCBOR_ERR_WRONG_TYPE);
		return NULL;
	}

	if (!uuid) {
		nrf_rpc_decoder_invalid(ctx, ZCBOR_ERR_NO_PAYLOAD);
		return NULL;
	}

	memcpy(uuid, buffer_ptr, buffer_size);

	return uuid;
}

#if defined(CONFIG_BT_GATT_CLIENT)

static void bt_uuid_enc(struct nrf_rpc_cbor_ctx *encoder, const struct bt_uuid *uuid)
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
			nrf_rpc_encoder_invalid(encoder);
			return;
		}
	}
	nrf_rpc_encode_buffer(encoder, uuid, size);
}

static struct bt_uuid *bt_uuid_cli_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_uuid *uuid)
{
	return (struct bt_uuid *)nrf_rpc_decode_buffer(ctx, uuid, sizeof(struct bt_uuid_128));
}

static void report_encoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

#endif /* CONFIG_BT_GATT_CLIENT */

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

static void bt_rpc_gatt_start_service_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	uint8_t service_index;
	size_t attr_count;
	int result;

	service_index = nrf_rpc_decode_uint(ctx);
	attr_count = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_start_service(service_index, attr_count);

	nrf_rpc_rsp_send_int(group, result);

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

static void bt_normal_attr_read_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	struct bt_normal_attr_read_res *res = (struct bt_normal_attr_read_res *)handler_data;

	res->read_len = nrf_rpc_decode_int(ctx);
	nrf_rpc_decode_buffer(ctx, res->buf, (res->read_len > 0) ? res->read_len : 0);
}

static ssize_t bt_rpc_normal_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_normal_attr_read_res result;
	size_t buffer_size_max = 19;
	size_t scratchpad_size = 0;
	uint8_t read_buf[len];

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(len);

	nrf_rpc_encode_uint(&ctx, scratchpad_size);
	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_rpc_encode_gatt_attr(&ctx, attr);
	nrf_rpc_encode_uint(&ctx, len);
	nrf_rpc_encode_uint(&ctx, offset);

	result.buf = read_buf;
	result.read_len = 0;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_ATTR_READ_RPC_CMD, &ctx,
				bt_normal_attr_read_rsp, &result);

	if (result.read_len < 0) {
		return result.read_len;
	} else {
		return bt_gatt_attr_read(conn, attr, buf, len, 0, result.buf, result.read_len);
	}
}

static ssize_t bt_rpc_normal_attr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					const void *buf, uint16_t len, uint16_t offset,
					uint8_t flags)
{
	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t buffer_size_max = 26;
	size_t scratchpad_size = 0;

	buffer_size_max += len;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(len);

	nrf_rpc_encode_uint(&ctx, scratchpad_size);
	bt_rpc_encode_bt_conn(&ctx, conn);
	bt_rpc_encode_gatt_attr(&ctx, attr);
	nrf_rpc_encode_uint(&ctx, len);
	nrf_rpc_encode_uint(&ctx, offset);
	nrf_rpc_encode_uint(&ctx, flags);
	nrf_rpc_encode_buffer(&ctx, buf, len);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_ATTR_WRITE_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_i32, &result);

	return result;
}

static void add_user_attr(struct bt_gatt_attr *attr, const struct bt_uuid *uuid, uint16_t data)
{
	attr->uuid = uuid;
	attr->read = (data & BT_RPC_GATT_ATTR_READ_PRESENT_FLAG) ? bt_rpc_normal_attr_read : NULL;
	attr->write =
		(data & BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG) ? bt_rpc_normal_attr_write : NULL;
	attr->user_data =
		(void *)((current_service.index << 16) | current_service.service->attr_count);
	attr->handle = 0;
	attr->perm = (uint8_t)data;
}

static void add_srv_attr(struct bt_gatt_attr *attr, const struct bt_uuid *service_uuid,
			 const struct bt_uuid *attr_uuid)
{
	attr->uuid = attr_uuid;
	attr->read = bt_gatt_attr_read_service;
	attr->write = NULL;
	attr->user_data = (void *)service_uuid;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;
}

static int add_chrc_attr(struct bt_gatt_attr *attr, const struct bt_uuid *uuid, uint8_t properties)
{
	struct bt_gatt_chrc *chrc;

	chrc = (struct bt_gatt_chrc *)bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_gatt_chrc));
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

static void bt_rpc_gatt_send_simple_attr_rpc_handler(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{

	uint8_t special_attr;
	uint16_t data;
	int result;

	struct bt_uuid *uuid;

	uuid = bt_uuid_svc_dec(ctx);

	special_attr = nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_send_simple_attr(special_attr, uuid, data);

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD, handler_data);
}

static int add_cep_attr(struct bt_gatt_attr *attr, uint16_t properties)
{
	struct bt_gatt_cep *cep;

	cep = (struct bt_gatt_cep *)bt_rpc_gatt_add(&gatt_buffer, sizeof(struct bt_gatt_cep));
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
	attr->read = bt_gatt_attr_read_cud;
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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_encode_uint(&ctx, index);
	nrf_rpc_encode_uint(&ctx, value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_CHANGED_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static ssize_t bt_ccc_cfg_write_call(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     uint16_t value)
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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, index);
	nrf_rpc_encode_uint(&ctx, value);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_WRITE_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_i32, &result);

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

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, index);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_CB_CCC_CFG_MATCH_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_bool, &result);

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

	ccc->cfg_changed =
		(param & BT_RPC_GATT_CCC_CFG_CHANGE_PRESENT_FLAG) ? bt_ccc_cfg_changed_call : NULL;
	ccc->cfg_write =
		(param & BT_RPC_GATT_CCC_CFG_WRITE_PRESENT_FLAG) ? bt_ccc_cfg_write_call : NULL;
	ccc->cfg_match =
		(param & BT_RPC_GATT_CCC_CFG_MATCH_PRESET_FLAG) ? bt_ccc_cfg_match_call : NULL;

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

static void bt_rpc_gatt_send_desc_attr_rpc_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	uint8_t special_attr;
	uint16_t param;
	size_t size;
	uint8_t *buffer;
	int result;
	struct nrf_rpc_scratchpad scratchpad;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	special_attr = nrf_rpc_decode_uint(ctx);
	param = nrf_rpc_decode_uint(ctx);
	size = nrf_rpc_decode_uint(ctx);
	buffer = nrf_rpc_decode_buffer_into_scratchpad(&scratchpad, NULL);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_rpc_gatt_send_desc_attr(special_attr, param, buffer, size);

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_send_desc_attr, BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,
			 bt_rpc_gatt_send_desc_attr_rpc_handler, NULL);

#if defined(CONFIG_BT_GATT_DYNAMIC_DB)
static int bt_rpc_gatt_end_service(void)
{
	int err;

	err = bt_gatt_service_register(current_service.service);

	current_service.service = NULL;
	current_service.attr_max = 0;

	return err;
}

static void bt_rpc_gatt_end_service_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	int result;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result = bt_rpc_gatt_end_service();

	nrf_rpc_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_end_service, BT_RPC_GATT_END_SERVICE_RPC_CMD,
			 bt_rpc_gatt_end_service_rpc_handler, NULL);

static void bt_rpc_gatt_service_unregister_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	int result = 0;
	uint16_t svc_index;
	const struct bt_gatt_service *svc;

	svc_index = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
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

	nrf_rpc_rsp_send_int(group, result);
	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_service_unregister,
			 BT_RPC_GATT_SERVICE_UNREGISTER_RPC_CMD,
			 bt_rpc_gatt_service_unregister_rpc_handler, NULL);
#endif /* CONFIG_BT_GATT_DYNAMIC_DB */

static inline void bt_gatt_complete_func_t_callback(struct bt_conn *conn, void *user_data,
						    uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 13;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)user_data);
	nrf_rpc_encode_callback_call(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_COMPLETE_FUNC_T_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

NRF_RPC_CBKPROXY_HANDLER(bt_gatt_complete_func_t_encoder, bt_gatt_complete_func_t_callback,
			 (struct bt_conn *conn, void *user_data), (conn, user_data));

static void bt_gatt_notify_params_dec(struct nrf_rpc_scratchpad *scratchpad,
				      struct bt_gatt_notify_params *data)
{

	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->attr = bt_rpc_decode_gatt_attr(ctx);
	data->len = nrf_rpc_decode_uint(ctx);
	data->data = nrf_rpc_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->func = (bt_gatt_complete_func_t)nrf_rpc_decode_callbackd(
		ctx, bt_gatt_complete_func_t_encoder);
	data->user_data = (void *)(uintptr_t)nrf_rpc_decode_uint(ctx);

	data->uuid = (struct bt_uuid *)nrf_rpc_decode_buffer_into_scratchpad(scratchpad, NULL);
}

static void bt_gatt_notify_cb_rpc_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_gatt_notify_params params;
	int result;
	struct nrf_rpc_scratchpad scratchpad;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_gatt_notify_params_dec(&scratchpad, &params);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_notify_cb(conn, &params);

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_NOTIFY_CB_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_notify_cb, BT_GATT_NOTIFY_CB_RPC_CMD,
			 bt_gatt_notify_cb_rpc_handler, NULL);

static void bt_gatt_indicate_params_dec(struct nrf_rpc_scratchpad *scratchpad,
					struct bt_gatt_indicate_params *data)
{

	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;

	data->attr = bt_rpc_decode_gatt_attr(ctx);
	data->len = nrf_rpc_decode_uint(ctx);
	data->data = nrf_rpc_decode_buffer_into_scratchpad(scratchpad, NULL);
	data->_ref = nrf_rpc_decode_uint(ctx);

	data->uuid = (struct bt_uuid *)nrf_rpc_decode_buffer_into_scratchpad(scratchpad, NULL);
}

static void bt_gatt_indicate_func_t_callback(struct bt_conn *conn,
					     struct bt_gatt_indicate_params *params, uint8_t err)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 10;
	struct bt_rpc_gatt_indication_params *rpc_params;

	rpc_params = CONTAINER_OF(params, struct bt_rpc_gatt_indication_params, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, err);
	nrf_rpc_encode_uint(&ctx, rpc_params->param_addr);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_FUNC_T_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void bt_gatt_indicate_params_destroy_t_callback(struct bt_gatt_indicate_params *params)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 5;
	struct bt_rpc_gatt_indication_params *rpc_params;

	rpc_params = CONTAINER_OF(params, struct bt_rpc_gatt_indication_params, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_encode_uint(&ctx, rpc_params->param_addr);

	k_free(rpc_params);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_INDICATE_PARAMS_DESTROY_T_CALLBACK_RPC_CMD,
				&ctx, nrf_rpc_rsp_decode_void, NULL);
}

static void bt_gatt_indicate_rpc_handler(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_rpc_gatt_indication_params *params;
	int result;
	struct nrf_rpc_scratchpad scratchpad;

	params = (struct bt_rpc_gatt_indication_params *)k_malloc(sizeof(*params));

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_gatt_indicate_params_dec(&scratchpad, &params->params);
	params->param_addr = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	params->params.func = bt_gatt_indicate_func_t_callback;
	params->params.destroy = bt_gatt_indicate_params_destroy_t_callback;

	result = bt_gatt_indicate(conn, &params->params);

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	k_free(params);
	report_decoding_error(BT_GATT_INDICATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_indicate, BT_GATT_INDICATE_RPC_CMD,
			 bt_gatt_indicate_rpc_handler, NULL);

static void bt_gatt_is_subscribed_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	uint16_t ccc_value;
	bool result;

	conn = bt_rpc_decode_bt_conn(ctx);
	attr = bt_rpc_decode_gatt_attr(ctx);
	ccc_value = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_is_subscribed(conn, attr, ccc_value);

	nrf_rpc_rsp_send_bool(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_IS_SUBSCRIBED_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_is_subscribed, BT_GATT_IS_SUBSCRIBED_RPC_CMD,
			 bt_gatt_is_subscribed_rpc_handler, NULL);

static void bt_gatt_get_mtu_rpc_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	uint16_t result;

	conn = bt_rpc_decode_bt_conn(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_get_mtu(conn);

	nrf_rpc_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_GET_MTU_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_get_mtu, BT_GATT_GET_MTU_RPC_CMD,
			 bt_gatt_get_mtu_rpc_handler, NULL);

#if defined(CONFIG_BT_GATT_CLIENT)

struct bt_gatt_exchange_mtu_container {
	struct bt_gatt_exchange_params params;
	uintptr_t remote_pointer;
};

static void bt_gatt_exchange_mtu_callback(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_exchange_params *params)
{
	struct bt_gatt_exchange_mtu_container *container;
	struct nrf_rpc_cbor_ctx ctx;

	container = CONTAINER_OF(params, struct bt_gatt_exchange_mtu_container, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 10);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, err);
	nrf_rpc_encode_uint(&ctx, container->remote_pointer);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_EXCHANGE_MTU_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	k_free(container);
}

static void bt_gatt_exchange_mtu_rpc_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_gatt_exchange_mtu_container *container;
	int result;

	conn = bt_rpc_decode_bt_conn(ctx);
	container = k_malloc(sizeof(struct bt_gatt_exchange_mtu_container));
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	container->remote_pointer = nrf_rpc_decode_uint(ctx);
	container->params.func = bt_gatt_exchange_mtu_callback;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_exchange_mtu(conn, &container->params);

	if (result < 0) {
		k_free(container);
	}

	nrf_rpc_rsp_send_int(group, result);

	return;

decoding_error:
	k_free(container);
alloc_error:
	report_decoding_error(BT_GATT_EXCHANGE_MTU_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_exchange_mtu, BT_GATT_EXCHANGE_MTU_RPC_CMD,
			 bt_gatt_exchange_mtu_rpc_handler, NULL);

#endif /* CONFIG_BT_GATT_CLIENT */

static void bt_gatt_attr_get_handle_rpc_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	const struct bt_gatt_attr *attr;
	uint16_t result;

	attr = bt_rpc_decode_gatt_attr(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_attr_get_handle(attr);

	nrf_rpc_rsp_send_uint(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_ATTR_GET_HANDLE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_attr_get_handle, BT_GATT_ATTR_GET_HANDLE_RPC_CMD,
			 bt_gatt_attr_get_handle_rpc_handler, NULL);

static void bt_gatt_cb_att_mtu_update_call(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 9;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, tx);
	nrf_rpc_encode_uint(&ctx, rx);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_CB_ATT_MTU_UPDATE_CALL_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static struct bt_gatt_cb bt_gatt_cb_data = {.att_mtu_updated = bt_gatt_cb_att_mtu_update_call};

static void bt_gatt_cb_register_handler(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	bt_gatt_cb_register(&bt_gatt_cb_data);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_cb_register, BT_LE_GATT_CB_REGISTER_ON_REMOTE_RPC_CMD,
			 bt_gatt_cb_register_handler, NULL);

#if defined(CONFIG_BT_GATT_CLIENT)

static uint8_t bt_gatt_discover_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	uint8_t result;
	struct bt_gatt_discover_container *container;
	struct nrf_rpc_cbor_ctx ctx;

	container = CONTAINER_OF(params, struct bt_gatt_discover_container, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 53);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, container->remote_pointer);

	if (attr == NULL) {
		nrf_rpc_encode_null(&ctx);
	} else {
		struct bt_uuid_16 *attr_uuid_16 = (struct bt_uuid_16 *)attr->uuid;

		bt_uuid_enc(&ctx, attr->uuid);
		nrf_rpc_encode_uint(&ctx, attr->handle);
		if (attr->user_data == NULL) {
			nrf_rpc_encode_null(&ctx);
		} else if (attr->uuid->type != BT_UUID_TYPE_16) {
			goto unsupported_exit;
		} else if (attr_uuid_16->val == BT_UUID_GATT_PRIMARY_VAL ||
			   attr_uuid_16->val == BT_UUID_GATT_SECONDARY_VAL) {
			struct bt_gatt_service_val *service;

			service = (struct bt_gatt_service_val *)attr->user_data;
			bt_uuid_enc(&ctx, service->uuid);
			nrf_rpc_encode_uint(&ctx, service->end_handle);
		} else if (attr_uuid_16->val == BT_UUID_GATT_INCLUDE_VAL) {
			struct bt_gatt_include *include;

			include = (struct bt_gatt_include *)attr->user_data;
			bt_uuid_enc(&ctx, include->uuid);
			nrf_rpc_encode_uint(&ctx, include->start_handle);
			nrf_rpc_encode_uint(&ctx, include->end_handle);
		} else if (attr_uuid_16->val == BT_UUID_GATT_CHRC_VAL) {
			struct bt_gatt_chrc *chrc;

			chrc = (struct bt_gatt_chrc *)attr->user_data;
			bt_uuid_enc(&ctx, chrc->uuid);
			nrf_rpc_encode_uint(&ctx, chrc->value_handle);
			nrf_rpc_encode_uint(&ctx, chrc->properties);
		} else {
			goto unsupported_exit;
		}
	}

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_DISCOVER_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_u8, &result);

	if (result == BT_GATT_ITER_STOP || attr == NULL) {
		k_free(container);
	}

	return result;

unsupported_exit:
	k_free(container);
	report_encoding_error(BT_GATT_DISCOVER_CALLBACK_RPC_CMD);
	return BT_GATT_ITER_STOP;
}

static void bt_gatt_discover_params_dec(struct nrf_rpc_cbor_ctx *ctx,
					struct bt_gatt_discover_params *data)
{
	data->uuid = bt_uuid_cli_dec(ctx, (struct bt_uuid *)data->uuid);
	data->start_handle = nrf_rpc_decode_uint(ctx);
	data->end_handle = nrf_rpc_decode_uint(ctx);
	data->type = nrf_rpc_decode_uint(ctx);
}

static void bt_gatt_discover_rpc_handler(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int result;
	struct bt_conn *conn;
	struct bt_gatt_discover_container *container;

	container = k_malloc(sizeof(struct bt_gatt_discover_container));
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	container->params.uuid = &container->uuid;

	conn = bt_rpc_decode_bt_conn(ctx);
	bt_gatt_discover_params_dec(ctx, &container->params);
	container->remote_pointer = nrf_rpc_decode_uint(ctx);

	container->params.func = bt_gatt_discover_callback;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_discover(conn, &container->params);

	if (result < 0) {
		k_free(container);
	}

	nrf_rpc_rsp_send_int(group, result);

	return;

decoding_error:
	k_free(container);
alloc_error:
	report_decoding_error(BT_GATT_DISCOVER_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_discover, BT_GATT_DISCOVER_RPC_CMD,
			 bt_gatt_discover_rpc_handler, NULL);

static uint8_t bt_gatt_read_callback(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params, const void *data,
				     uint16_t length)
{
	uint8_t result;
	struct bt_gatt_read_container *container;
	struct nrf_rpc_cbor_ctx ctx;

	container = CONTAINER_OF(params, struct bt_gatt_read_container, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 20 + length);

	nrf_rpc_encode_uint(&ctx, NRF_RPC_SCRATCHPAD_ALIGN(length));
	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, err);
	nrf_rpc_encode_uint(&ctx, container->remote_pointer);
	nrf_rpc_encode_buffer(&ctx, data, length);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_READ_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_u8, &result);

	if (result == BT_GATT_ITER_STOP || data == NULL || params->handle_count == 1) {
		k_free(container);
	}

	return result;
}

static void bt_gatt_read_params_dec(struct nrf_rpc_cbor_ctx *ctx, struct bt_gatt_read_params *data)
{
	if (data->handle_count == 0) {
		data->by_uuid.start_handle = nrf_rpc_decode_uint(ctx);
		data->by_uuid.end_handle = nrf_rpc_decode_uint(ctx);
		data->by_uuid.uuid = bt_uuid_cli_dec(ctx, (struct bt_uuid *)data->by_uuid.uuid);
	} else if (data->handle_count == 1) {
		data->single.handle = nrf_rpc_decode_uint(ctx);
		data->single.offset = nrf_rpc_decode_uint(ctx);
	} else {
		nrf_rpc_decode_buffer(ctx, data->multiple.handles,
				      sizeof(data->multiple.handles[0]) * data->handle_count);
		data->multiple.variable = nrf_rpc_decode_bool(ctx);
	}
}

static void bt_gatt_read_rpc_handler(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	struct bt_gatt_read_container *container;
	int result;
	size_t handle_count;

	conn = bt_rpc_decode_bt_conn(ctx);
	handle_count = nrf_rpc_decode_uint(ctx);
	container = k_malloc(sizeof(struct bt_gatt_read_container) +
			     sizeof(container->params.multiple.handles[0]) * handle_count);
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	container->params.handle_count = handle_count;
	container->params.by_uuid.uuid = &container->uuid;
	container->params.multiple.handles = container->handles;

	bt_gatt_read_params_dec(ctx, &container->params);
	container->remote_pointer = nrf_rpc_decode_uint(ctx);
	container->params.func = bt_gatt_read_callback;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_read(conn, &container->params);

	if (result < 0) {
		k_free(container);
	}

	nrf_rpc_rsp_send_int(group, result);

	return;

decoding_error:
	k_free(container);
alloc_error:
	report_decoding_error(BT_GATT_READ_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_read, BT_GATT_READ_RPC_CMD, bt_gatt_read_rpc_handler,
			 NULL);

static void bt_gatt_write_callback(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_write_params *params)
{
	struct bt_gatt_write_container *container;
	struct nrf_rpc_cbor_ctx ctx;

	container = CONTAINER_OF(params, struct bt_gatt_write_container, params);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, 10);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, err);
	nrf_rpc_encode_uint(&ctx, container->remote_pointer);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_WRITE_CALLBACK_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	k_free(container);
}

static void bt_gatt_write_rpc_handler(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_gatt_write_container *container;
	int result;
	size_t buffer_length;
	const void *buffer_ptr;

	conn = bt_rpc_decode_bt_conn(ctx);
	buffer_ptr = nrf_rpc_decode_buffer_ptr_and_size(ctx, &buffer_length);
	if (buffer_ptr == NULL) {
		goto alloc_error;
	}
	container = k_malloc(sizeof(struct bt_gatt_write_container) + buffer_length);
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	container->params.data = container->data;
	container->params.length = buffer_length;
	memcpy(container->data, buffer_ptr, buffer_length);
	container->params.handle = nrf_rpc_decode_uint(ctx);
	container->params.offset = nrf_rpc_decode_uint(ctx);
	container->remote_pointer = nrf_rpc_decode_uint(ctx);
	container->params.func = bt_gatt_write_callback;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_write(conn, &container->params);

	if (result < 0) {
		k_free(container);
	}

	nrf_rpc_rsp_send_int(group, result);

	return;

decoding_error:
	k_free(container);
alloc_error:
	report_decoding_error(BT_GATT_WRITE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_write, BT_GATT_WRITE_RPC_CMD,
			 bt_gatt_write_rpc_handler, NULL);

static void bt_gatt_write_without_response_cb_rpc_handler(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{

	struct bt_conn *conn;
	uint16_t handle;
	uint16_t length;
	uint8_t *data;
	bool sign;
	bt_gatt_complete_func_t func;
	void *user_data;
	int result;
	struct nrf_rpc_scratchpad scratchpad;

	NRF_RPC_SCRATCHPAD_DECLARE(&scratchpad, ctx);

	conn = bt_rpc_decode_bt_conn(ctx);
	handle = nrf_rpc_decode_uint(ctx);
	length = nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_buffer_into_scratchpad(&scratchpad, NULL);
	sign = nrf_rpc_decode_bool(ctx);
	func = (bt_gatt_complete_func_t)nrf_rpc_decode_callbackd(ctx,
								 bt_gatt_complete_func_t_encoder);
	user_data = (void *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_write_without_response_cb(conn, handle, data, length, sign, func,
						   user_data);

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_WRITE_WITHOUT_RESPONSE_CB_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_write_without_response_cb,
			 BT_GATT_WRITE_WITHOUT_RESPONSE_CB_RPC_CMD,
			 bt_gatt_write_without_response_cb_rpc_handler, NULL);

static struct bt_gatt_subscribe_container *get_subscribe_container(uintptr_t remote_pointer,
								   bool *create)
{
	struct bt_gatt_subscribe_container *container = NULL;

	k_mutex_lock(&subscribe_containers_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&subscribe_containers, container, node) {
		if (container->remote_pointer == remote_pointer) {
			*create = false;
			goto unlock_and_return;
		}
	}

	if (*create) {
		container = k_calloc(1, sizeof(struct bt_gatt_subscribe_container));
		if (container != NULL) {
			container->remote_pointer = remote_pointer;
			sys_slist_append(&subscribe_containers, &container->node);
		}
	} else {
		container = NULL;
	}

unlock_and_return:
	k_mutex_unlock(&subscribe_containers_mutex);
	return container;
}

static void free_subscribe_container(struct bt_gatt_subscribe_container *container)
{
	k_mutex_lock(&subscribe_containers_mutex, K_FOREVER);
	sys_slist_find_and_remove(&subscribe_containers, &container->node);
	k_mutex_unlock(&subscribe_containers_mutex);
	k_free(container);
}

static uint8_t bt_gatt_subscribe_params_notify(struct bt_conn *conn,
					       struct bt_gatt_subscribe_params *params,
					       const void *data, uint16_t length)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t _data_size;
	uint8_t result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 21;
	struct bt_gatt_subscribe_container *container;

	container = CONTAINER_OF(params, struct bt_gatt_subscribe_container, params);

	_data_size = sizeof(uint8_t) * length;
	buffer_size_max += _data_size;

	scratchpad_size += NRF_RPC_SCRATCHPAD_ALIGN(_data_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	nrf_rpc_encode_uint(&ctx, scratchpad_size);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, container->remote_pointer);
	nrf_rpc_encode_buffer(&ctx, data, _data_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_SUBSCRIBE_PARAMS_NOTIFY_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_u8, &result);

	return result;
}

static const size_t bt_gatt_subscribe_params_buf_size =
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, value_handle) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, ccc_handle) +
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, value) +
#if defined(CONFIG_BT_SMP)
	1 + BT_RPC_SIZE_OF_FIELD(struct bt_gatt_subscribe_params, min_security) +
#endif
	/* Placeholder for the flags field */
	2;

static void bt_gatt_subscribe_params_subscribe(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_subscribe_params *params,
					   uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 20;

	if (params != NULL) {
		buffer_size_max += bt_gatt_subscribe_params_buf_size;
	} else {
		buffer_size_max += 1;
	}

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	bt_rpc_encode_bt_conn(&ctx, conn);
	nrf_rpc_encode_uint(&ctx, err);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)params);
	if (params != NULL) {
		nrf_rpc_encode_uint(&ctx, params->value_handle);
		nrf_rpc_encode_uint(&ctx, params->ccc_handle);
		nrf_rpc_encode_uint(&ctx, params->value);
	#if defined(CONFIG_BT_SMP)
		nrf_rpc_encode_uint(&ctx, params->min_security);
	#endif /* defined(CONFIG_BT_SMP) */
		nrf_rpc_encode_uint(&ctx, (uint16_t)atomic_get(params->flags));
	} else {
		nrf_rpc_encode_null(&ctx);
	}
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GATT_SUBSCRIBE_PARAMS_SUBSCRIBE_RPC_CMD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

NRF_RPC_CBKPROXY_HANDLER(bt_gatt_subscribe_params_subscribe_encoder,
			 bt_gatt_subscribe_params_subscribe,
			 (struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_subscribe_params *params),
			 (conn, err, params));

static void bt_gatt_subscribe_params_dec(struct nrf_rpc_cbor_ctx *ctx,
					 struct bt_gatt_subscribe_params *data)
{
	data->notify = nrf_rpc_decode_bool(ctx) ? bt_gatt_subscribe_params_notify : NULL;
	data->subscribe = (bt_gatt_subscribe_func_t)nrf_rpc_decode_callbackd(
		ctx, bt_gatt_subscribe_params_subscribe_encoder);
	data->value_handle = nrf_rpc_decode_uint(ctx);
	data->ccc_handle = nrf_rpc_decode_uint(ctx);
	data->value = nrf_rpc_decode_uint(ctx);

#if defined(CONFIG_BT_SMP)
	data->min_security = nrf_rpc_decode_uint(ctx);
#endif
	atomic_set(data->flags, (atomic_val_t)nrf_rpc_decode_uint(ctx));
}

static void bt_gatt_subscribe_rpc_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	struct bt_conn *conn;
	struct bt_gatt_subscribe_container *container;
	bool new_container = true;
	int result;
	uintptr_t remote_pointer;

	conn = bt_rpc_decode_bt_conn(ctx);
	remote_pointer = nrf_rpc_decode_uint(ctx);
	container = get_subscribe_container(remote_pointer, &new_container);
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	bt_gatt_subscribe_params_dec(ctx, &container->params);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_subscribe(conn, &container->params);

	nrf_rpc_rsp_send_int(group, result);

	if (result < 0 && new_container) {
		free_subscribe_container(container);
	}

	return;
decoding_error:
	if (new_container) {
		free_subscribe_container(container);
	}
alloc_error:
	report_decoding_error(BT_GATT_SUBSCRIBE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_subscribe, BT_GATT_SUBSCRIBE_RPC_CMD,
			 bt_gatt_subscribe_rpc_handler, NULL);

static void bt_gatt_resubscribe_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{

	uint8_t id;
	bt_addr_le_t _peer_data;
	const bt_addr_le_t *peer;
	int result;
	uintptr_t remote_pointer;
	struct bt_gatt_subscribe_container *container;
	bool new_container = true;

	id = nrf_rpc_decode_uint(ctx);
	peer = nrf_rpc_decode_buffer(ctx, &_peer_data, sizeof(bt_addr_le_t));
	remote_pointer = nrf_rpc_decode_uint(ctx);
	container = get_subscribe_container(remote_pointer, &new_container);
	if (container == NULL) {
		nrf_rpc_decoding_done_and_check(group, ctx);
		goto alloc_error;
	}
	bt_gatt_subscribe_params_dec(ctx, &container->params);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	result = bt_gatt_resubscribe(id, peer, &container->params);

	nrf_rpc_rsp_send_int(group, result);

	if (result < 0 && new_container) {
		free_subscribe_container(container);
	}

	return;
decoding_error:
	if (new_container) {
		free_subscribe_container(container);
	}
alloc_error:
	report_decoding_error(BT_GATT_RESUBSCRIBE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_resubscribe, BT_GATT_RESUBSCRIBE_RPC_CMD,
			 bt_gatt_resubscribe_rpc_handler, NULL);

static void bt_gatt_unsubscribe_rpc_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct bt_conn *conn;
	int result;
	uintptr_t remote_pointer;
	struct bt_gatt_subscribe_container *container;
	bool new_container = false;

	conn = bt_rpc_decode_bt_conn(ctx);
	remote_pointer = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	container = get_subscribe_container(remote_pointer, &new_container);

	if (container != NULL) {
		result = bt_gatt_unsubscribe(conn, &container->params);
		free_subscribe_container(container);
	} else {
		result = -EINVAL;
	}

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_GATT_UNSUBSCRIBE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_unsubscribe, BT_GATT_UNSUBSCRIBE_RPC_CMD,
			 bt_gatt_unsubscribe_rpc_handler, NULL);

static void bt_rpc_gatt_subscribe_flag_update_rpc_handler(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{
	uint32_t flags_bit;
	int result;
	uintptr_t remote_pointer;
	struct bt_gatt_subscribe_container *container;
	bool new_container = false;
	int val;

	remote_pointer = nrf_rpc_decode_uint(ctx);
	flags_bit = nrf_rpc_decode_uint(ctx);
	val = nrf_rpc_decode_int(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		goto decoding_error;
	}

	container = get_subscribe_container(remote_pointer, &new_container);
	if (container == NULL) {
		result = -EINVAL;
	} else {
		if (atomic_test_bit(container->params.flags, flags_bit)) {
			result = 1;
		} else {
			result = 0;
		}

		if (val == 0) {
			atomic_clear_bit(container->params.flags, flags_bit);
		} else if (val > 0) {
			atomic_set_bit(container->params.flags, flags_bit);
		}
	}

	nrf_rpc_rsp_send_int(group, result);

	return;
decoding_error:
	report_decoding_error(BT_RPC_GATT_SUBSCRIBE_FLAG_UPDATE_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_subscribe_flag_update,
			 BT_RPC_GATT_SUBSCRIBE_FLAG_UPDATE_RPC_CMD,
			 bt_rpc_gatt_subscribe_flag_update_rpc_handler, NULL);

int bt_rpc_gatt_subscribe_flag_set(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	atomic_set_bit(params->flags, flags_bit);
	return 0;
}

int bt_rpc_gatt_subscribe_flag_clear(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	atomic_clear_bit(params->flags, flags_bit);
	return 0;
}

int bt_rpc_gatt_subscribe_flag_get(struct bt_gatt_subscribe_params *params, uint32_t flags_bit)
{
	return atomic_test_bit(params->flags, flags_bit) ? 1 : 0;
}

#endif /* CONFIG_BT_GATT_CLIENT */
