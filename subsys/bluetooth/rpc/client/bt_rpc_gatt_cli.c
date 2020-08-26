/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

SERIALIZE(GROUP(bt_rpc_grp));
SERIALIZE(OPAQUE_STRUCT(void));
SERIALIZE(FILTERED_STRUCT(struct bt_conn, 3, encode_bt_conn, decode_bt_conn));


#ifndef __GENERATOR
#define UNUSED __attribute__((unused)) /* TODO: Improve generator to avoid this workaround */
#else
#define UNUSED ;
#endif


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


UNUSED
static const size_t bt_uuid_buf_size = 2;                                        /*##BhKOb3g*/

void bt_uuid_enc(CborEncoder *_encoder, const struct bt_uuid *_data)             /*####%BlBm*/
{                                                                                /*#####@oyY*/

	SERIALIZE(STRUCT(struct bt_uuid));

	ser_encode_uint(_encoder, _data->type);                                  /*##A/17YtQ*/

}                                                                                /*##B9ELNqo*/



size_t bt_gatt_notify_params_buf_size(const struct bt_gatt_notify_params *_data) /*####%BrKt*/
{                                                                                /*#####@yBI*/

	size_t _buffer_size_max = 18;                                            /*##ARGP9z4*/

	_buffer_size_max += (_data->uuid == NULL) ? 1 : 2;                       /*##CAuhAMM*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

size_t bt_gatt_notify_params_sp_size(const struct bt_gatt_notify_params *_data)  /*####%BkZv*/
{                                                                                /*#####@tI8*/

	size_t _scratchpad_size = 0;                                             /*##ATz5YrA*/

	_scratchpad_size += (_data->uuid == NULL) ? 0 : sizeof(struct bt_uuid);  /*##ENO6SLE*/

	return _scratchpad_size;                                                 /*##BRWAmyU*/

}                                                                                /*##B9ELNqo*/

void bt_gatt_notify_params_enc(CborEncoder *_encoder, const struct bt_gatt_notify_params *_data)/*####%BiD8*/
{                                                                                               /*#####@Y84*/

	SERIALIZE(STRUCT(struct bt_gatt_notify_params));
	SERIALIZE(NULLABLE(uuid));
	SERIALIZE(DEL(attr));

	if (_data->uuid == NULL) {                                                              /*########%*/
		ser_encode_null(_encoder);                                                      /*########A*/
	} else {                                                                                /*########4*/
		bt_uuid_enc(_encoder, _data->uuid);                                             /*########c*/
	}                                                                                       /*########m*/
	ser_encode_uint(_encoder, (uintptr_t)_data->data);                                      /*########k*/
	ser_encode_uint(_encoder, _data->len);                                                  /*########7*/
	ser_encode_callback(_encoder, _data->func);                                             /*########Q*/
	ser_encode_uint(_encoder, (uintptr_t)_data->user_data);                                 /*########@*/

}                                                                                               /*##B9ELNqo*/

static int send_init_done(void) {
	return -0;
}

static int bt_rpc_gatt_start_service(uint8_t service_index, size_t attr_count)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AT*/
	int _result;                                                             /*######ewD*/
	size_t _buffer_size_max = 7;                                             /*######@6Y*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, service_index);                           /*####%A53t*/
	ser_encode_uint(&_ctx.encoder, attr_count);                              /*#####@gs4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_START_SERVICE_RPC_CMD,  /*####%BCaC*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@mjc*/

	return _result;                                                          /*##BX7TDLc*/
}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	return -0;                                                          
}


static size_t bt_uuid_gatt_buf_size(const struct bt_uuid *uuid) {
	switch (uuid->type)
	{
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

static void bt_uuid_gatt_enc(CborEncoder *encoder, const struct bt_uuid *uuid)
{
	switch (uuid->type)
	{
	case BT_UUID_TYPE_16:
		ser_encode_buffer(encoder,
			&((const struct bt_uuid_16 *)uuid)->val,
			sizeof(((const struct bt_uuid_16 *)uuid)->val));
		break;
	
	case BT_UUID_TYPE_32:
		ser_encode_buffer(encoder,
			&((const struct bt_uuid_32 *)uuid)->val,
			sizeof(((const struct bt_uuid_32 *)uuid)->val));
		break;
	
	case BT_UUID_TYPE_128:
		ser_encode_buffer(encoder,
			&((const struct bt_uuid_128 *)uuid)->val,
			sizeof(((const struct bt_uuid_128 *)uuid)->val));
		break;
	
	default:
		ser_encoder_invalid(encoder);
		break;
	}
}

static int bt_rpc_gatt_send_simple_attr(uint8_t special_attr, const struct bt_uuid *uuid, uint16_t data)
{
	SERIALIZE(DEL(uuid));

	struct nrf_rpc_cbor_ctx _ctx;                                             /*######%Aa*/
	int _result;                                                              /*######Qso*/
	size_t _buffer_size_max = 5;                                              /*######@uA*/

	_buffer_size_max += bt_uuid_gatt_buf_size(uuid);

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	bt_uuid_gatt_enc(&_ctx.encoder, uuid);

	ser_encode_uint(&_ctx.encoder, special_attr);                             /*####%AzFW*/
	ser_encode_uint(&_ctx.encoder, data);                                     /*#####@4LE*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,/*####%BI2J*/
		&_ctx, ser_rsp_simple_i32, &_result);                             /*#####@M64*/

	return _result;                                                           /*##BX7TDLc*/
}

static int send_normal_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	uint16_t data = attr->perm;

	if (attr->read != NULL) {
		data |= BT_RPC_GATT_ATTR_READ_PRESENT_FLAG;
	}

	if (attr->write != NULL) {
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
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

	__ASSERT(chrc->value_handle == 0, "Only default value of value_handle is implemented!");

	return bt_rpc_gatt_send_simple_attr(special_attr, chrc->uuid, chrc->properties);
}

static int bt_rpc_gatt_send_desc_attr(uint8_t special_attr, uint8_t rw_sync, uint16_t param, uint8_t *buffer, size_t size)
{
	SERIALIZE(SIZE_PARAM(buffer, size));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	size_t _buffer_size;                                                     /*#######fe*/
	int _result;                                                             /*#######pD*/
	size_t _scratchpad_size = 0;                                             /*#######5E*/
	size_t _buffer_size_max = 22;                                            /*########@*/

	_buffer_size = sizeof(uint8_t) * size;                                   /*####%CNdY*/
	_buffer_size_max += _buffer_size;                                        /*#####@FRU*/

	_scratchpad_size += SCRATCHPAD_ALIGN(_buffer_size);                      /*##EJIyO2c*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, special_attr);                            /*#######%A*/
	ser_encode_uint(&_ctx.encoder, rw_sync);                                 /*#######1J*/
	ser_encode_uint(&_ctx.encoder, param);                                   /*#######jE*/
	ser_encode_uint(&_ctx.encoder, size);                                    /*#######ls*/
	ser_encode_buffer(&_ctx.encoder, buffer, _buffer_size);                  /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD, /*####%BLqZ*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@WaY*/

	return _result;                                                          /*##BX7TDLc*/
}

static int send_cep_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_gatt_cep *cep = (struct bt_gatt_cep *)attr->user_data;
	
	return bt_rpc_gatt_send_desc_attr(special_attr, (uintptr_t)attr->write, cep->properties, NULL, 0);
}

static int send_cud_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	char *cud = (char *)attr->user_data;
	
	return bt_rpc_gatt_send_desc_attr(special_attr, (uintptr_t)attr->write, attr->perm, cud, strlen(cud) + 1);
}

static int send_cpf_attr(uint8_t special_attr, const struct bt_gatt_attr *attr)
{
	struct bt_gatt_cpf *cpf = (struct bt_gatt_cpf *)attr->user_data;
	
	return bt_rpc_gatt_send_desc_attr(special_attr, (uintptr_t)attr->write, 0, (uint8_t *)cpf, sizeof(struct bt_gatt_cpf));
}

static int bt_rpc_gatt_end_service(void)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AX*/
	int _result;                                                             /*######56+*/
	size_t _buffer_size_max = 0;                                             /*######@io*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_END_SERVICE_RPC_CMD,    /*####%BI1p*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@CO0*/

	return _result;                                                          /*##BX7TDLc*/
}

static int send_service_static(const struct bt_gatt_service_static *svc)
{
	int res;
	size_t i;
	const struct bt_gatt_attr *attr;
	uintptr_t special_attr;
	int service_index;

	service_index = add_service(svc);
	if (service_index < 0) {
		return service_index;
	}

	LOG_DBG("Sending static service %d", service_index);
	
	res = bt_rpc_gatt_start_service(service_index, svc->attr_count);
	if (res < 0) {
		return res;
	}

	for (i = 0; i < svc->attr_count; i++) {

		attr = &svc->attrs[i];

		special_attr = (uintptr_t)attr->read;

		if (special_attr < BT_RPC_GATT_ATTR_SPECIAL_FIRST) {
			res = send_normal_attr(0, attr);
		} else {
			switch (special_attr)
			{
			case BT_RPC_GATT_ATTR_SPECIAL_SERVICE:
				res = send_service_attr(special_attr, attr);
				break;

			case BT_RPC_GATT_ATTR_SPECIAL_SECONDARY:
				res = send_service_attr(special_attr, attr);
				break;

			case BT_RPC_GATT_ATTR_SPECIAL_CHRC:
				res = send_chrc_attr(special_attr, attr);
				break;
			
			case BT_RPC_GATT_ATTR_SPECIAL_CCC:
				//res = send_ccc_attr(attr, service_index, i);
				break;

			case BT_RPC_GATT_ATTR_SPECIAL_CEP:
				res = send_cep_attr(special_attr, attr);
				break;

			case BT_RPC_GATT_ATTR_SPECIAL_CUD:
				res = send_cud_attr(special_attr, attr);
				break;

			case BT_RPC_GATT_ATTR_SPECIAL_CPF:
				res = send_cpf_attr(special_attr, attr);
				break;

			default:
				res = -EINVAL;
				break;
			}
		}

		if (res < 0) {
			return res;
		}
	}
	
	res = bt_rpc_gatt_end_service();
	if (res < 0) {
		return res;
	}

	return res;
}

int bt_rpc_gatt_init(void)
{
	Z_STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		send_service_static(svc);
	}

	return send_init_done();
}
