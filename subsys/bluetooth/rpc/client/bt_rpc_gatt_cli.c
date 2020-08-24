
/* Client side of bluetooth API over nRF RPC.
 */

#include <sys/types.h>

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/att.h"
#include "bluetooth/gatt.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

SERIALIZE(GROUP(bt_rpc_grp));
SERIALIZE(OPAQUE_STRUCT(void));
SERIALIZE(FILTERED_STRUCT(struct bt_conn, 3, encode_bt_conn, decode_bt_conn));


#ifndef NRF_RPC_GENERATOR
#define UNUSED __attribute__((unused))
#else
#define UNUSED ;
#endif

#if defined(NRF_RPC_GENERATOR)
#define _
#endif


static const struct bt_gatt_service_static *services[CONFIG_BT_RPC_GATT_SRV_MAX];
static size_t service_count = 0;

static int attr_to_index(struct bt_gatt_attr *attr)
{
	int attr_index;
	int service_index;
	const struct bt_gatt_service_static *service;

	service_index = 0;
	do
	{
		if (service_index >= service_count) {
			return -1;
		}
		service = services[service_index];
		if (attr >= service->attrs && attr < &service->attrs[service->attr_count]) {
			break;
		}
		service_index++;
	} while (true);

	attr_index = attr - service->attrs;

	return (service_index << 16) | (attr_index & 0xFFFF);
}

static const struct bt_gatt_attr *index_to_attr(int index)
{
	uint16_t attr_index = index & 0xFFFF;
	uint8_t service_index = index >> 16;
	const struct bt_gatt_service_static *service;

	if (service_index >= service_count) {
		return NULL;
	}

	service = services[service_index];

	if (attr_index >= service->attr_count) {
		return NULL;
	}

	return &service->attrs[attr_index];
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

	LOG_DBG("handle 0x%04x offset %u length %u", attr->handle, offset,
	       len);

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

static int bt_rpc_gatt_start_service(size_t attr_count) {
	return -0;
}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	return -0;                                                          
}

int add_service(const struct bt_gatt_service_static *service)
{
	int service_index;

	if (service_count >= CONFIG_BT_RPC_GATT_SRV_MAX) {
		LOG_ERR("Too many services send by BT_RPC. Increase CONFIG_BT_RPC_GATT_SRV_MAX.");
		return -ENOMEM;
	}

	service_index = service_count;
	services[service_index] = service;
	service_count++;

	return service_index;
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
	
	res = bt_rpc_gatt_start_service(svc->attr_count);
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

	return 0;
}

int bt_rpc_gatt_init(void)
{
	Z_STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc) {
		send_service_static(svc);
	}

	return send_init_done();
}

#if 0

size_t calc_gatt_attr_size(struct bt_gatt_attr *attr)
{
	size_t total_size = 0;

	total_size += sizeof(struct bt_gatt_attr);

	if ((uintptr_t)attr->read >= _BT_GATT_ATTR_RW_SPECIAL_FIRST
		|| (uintptr_t)attr->write >= _BT_GATT_ATTR_RW_SPECIAL_FIRST) {

		if (attr->read == _BT_GATT_ATTR_READ_SERVICE) {
			struct bt_uuid *uuid = (struct bt_uuid *)attr->user_data;
			switch (uuid->type) {
			case BT_UUID_TYPE_16:
				total_size += sizeof(struct bt_uuid_16);
				break;
			case BT_UUID_TYPE_32:
				total_size += sizeof(struct bt_uuid_32);
				break;
			default:
				total_size += sizeof(struct bt_uuid_128);
				break;
			}
		} else if (attr->read == _BT_GATT_ATTR_READ_INCLUDED) {
			// none
		} else if (attr->read == _BT_GATT_ATTR_READ_CHRC) {
			total_size += sizeof(struct bt_gatt_chrc);
		}
	}

	for (i = 0; i < svc->attr_count; i++) {
		total_size += calc_gatt_attr_size(&svc->attrs[i]);
	}
}

int bt_rpc_gatt_start_service(size_t attr_count)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Aa*/
	int _result;                                                             /*######Qso*/
	size_t _buffer_size_max = 5;                                             /*######@uA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, attr_count);                              /*##A29iav8*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GATT_START_SERVICE_RPC_CMD,  /*####%BCaC*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@mjc*/

	return _result;                                                          /*##BX7TDLc*/
}

static void send_attr(struct bt_gatt_attr *attr)
{
	if ((uintptr_t)attr->read >= _BT_GATT_ATTR_RW_SPECIAL_FIRST
		|| (uintptr_t)attr->write >= _BT_GATT_ATTR_RW_SPECIAL_FIRST) {

		if (attr->read == _BT_GATT_ATTR_READ_SERVICE) {
			struct bt_uuid *uuid = (struct bt_uuid *)attr->user_data;
			switch (uuid->type) {
			case BT_UUID_TYPE_16:
				total_size += sizeof(struct bt_uuid_16);
				break;
			case BT_UUID_TYPE_32:
				total_size += sizeof(struct bt_uuid_32);
				break;
			default:
				total_size += sizeof(struct bt_uuid_128);
				break;
			}
		} else if (attr->read == _BT_GATT_ATTR_READ_INCLUDED) {
			// none
		} else if (attr->read == _BT_GATT_ATTR_READ_CHRC) {
			total_size += sizeof(struct bt_gatt_chrc);
		}
	}

	for (i = 0; i < svc->attr_count; i++) {
		total_size += calc_gatt_attr_size(&svc->attrs[i]);
	}
}



void send_normal_attr(struct bt_gatt_attr *attr)
{
	
}

void send_service_attr(struct bt_gatt_attr *attr)
{
	struct bt_uuid *uuid = (struct bt_uuid *)attr->user_data;

	send_special_attr(BT_RPC_GATT_ATTR_SERVICE, attr->perm, uuid);
}

void send_included_attr(struct bt_gatt_attr *attr)
{
	size_t attr_index;
	struct bt_gatt_attr *included_attr =
		(struct bt_gatt_attr *)attr->user_data;

	attr_index = get_attr_index(included_attr);

	send_special_attr(BT_RPC_GATT_ATTR_INCLUDED, attr->perm, attr_index);
}

void send_chrc_attr(struct bt_gatt_attr *attr)
{
	struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

	send_special_attr(BT_RPC_GATT_ATTR_INCLUDED, attr->perm, chrc->uuid, chrc->value_handle, chrc->properties);
}

void send_cud_attr(struct bt_gatt_attr *attr)
{
	const char *value = (const char *)user_data;

	send_special_attr(BT_RPC_GATT_ATTR_INCLUDED, attr->perm, value);
	struct bt_gatt_cpf
}

void send_cpf_attr(struct bt_gatt_attr *attr)
{
	const struct bt_gatt_cpf *cpf = (const struct bt_gatt_cpf *)user_data;

	send_special_attr(BT_RPC_GATT_ATTR_INCLUDED, attr->perm, cpf);
}

#endif