/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/att.h"
#include "bluetooth/gatt.h"
#include "bluetooth/conn.h"

#include "bt_rpc_common.h"
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

static uint32_t copy_buffer[(CONFIG_BT_RPC_COPY_BUFFER_SIZE + 3) / 4];
static size_t copy_buffer_length = 0;

static uint8_t service_map[sizeof(copy_buffer) / sizeof(struct bt_gatt_attr)];

static struct bt_gatt_service *current_service;
static size_t current_service_attr_max;
static int current_service_index;


#ifndef __GENERATOR
#define UNUSED __attribute__((unused)) /* TODO: Improve generator to avoid this workaround */
#else
#define UNUSED ;
#endif

void *copy_buffer_reserve(size_t size)
{
	void *result;

	size = (size + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1);

	if (size >= sizeof(copy_buffer) - copy_buffer_length) {
		LOG_ERR("Copy buffer overflow. Increase CONFIG_BT_RPC_COPY_BUFFER_SIZE.");
		return NULL;
	}

	result = &((uint8_t *)copy_buffer)[copy_buffer_length];
	copy_buffer_length += size;

	return result;
}


void bt_uuid_dec(CborValue *_value, struct bt_uuid *_data)                       /*####%BtNY*/
{                                                                                /*#####@wF4*/

	_data->type = ser_decode_uint(_value);                                   /*##CvPSinc*/

}                                                                                /*##B9ELNqo*/

void *bt_gatt_complete_func_t_encoder; // TODO: implemente

void bt_gatt_notify_params_dec(struct ser_scratchpad *_scratchpad, struct bt_gatt_notify_params *_data)     /*####%Bne7*/
{                                                                                                           /*#####@rXA*/

	CborValue *_value = _scratchpad->value;                                                             /*##AU3cSLw*/

	if (ser_decode_is_null(_value)) {                                                                   /*########%*/
		_data->uuid = NULL;                                                                         /*########C*/
		ser_decode_skip(_value);                                                                    /*########o*/
	} else {                                                                                            /*########y*/
		_data->uuid = ser_scratchpad_get(_scratchpad, sizeof(struct bt_uuid));                      /*########w*/
		bt_uuid_dec(_value, _data->uuid);                                                           /*########N*/
	}                                                                                                   /*########I*/
	_data->data = (const void *)(uintptr_t)ser_decode_uint(_value);                                     /*########k*/
	_data->len = ser_decode_uint(_value);                                                               /*#########*/
	_data->func = (bt_gatt_complete_func_t)ser_decode_callback(_value, bt_gatt_complete_func_t_encoder);/*#########*/
	_data->user_data = (void *)(uintptr_t)ser_decode_uint(_value);                                      /*########@*/

}                                                                                                           /*##B9ELNqo*/

static void bt_gatt_notify_cb_rpc_handler(CborValue *_value, void *_handler_data)/*####%BlJm*/
{                                                                                /*#####@K6c*/

	struct bt_conn * conn;                                                   /*######%Ae*/
	struct bt_gatt_notify_params params;                                     /*#######o2*/
	int _result;                                                             /*#######bp*/
	struct ser_scratchpad _scratchpad;                                       /*#######@I*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	conn = decode_bt_conn(_value);                                           /*####%Ck+d*/
	bt_gatt_notify_params_dec(&_scratchpad, &params);                        /*#####@ZFA*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_gatt_notify_cb(conn, &params);                              /*##DrDjw04*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%FU*/
decoding_error:                                                                  /*#######KB*/
	report_decoding_error(BT_GATT_NOTIFY_CB_RPC_CMD, _handler_data);         /*#######tF*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@w*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_gatt_notify_cb, BT_GATT_NOTIFY_CB_RPC_CMD,/*####%BonW*/
	bt_gatt_notify_cb_rpc_handler, NULL);                                     /*#####@NDk*/


static int bt_rpc_gatt_start_service(uint8_t remote_service_index, size_t attr_count)
{
	struct bt_gatt_service_static *service;
	struct bt_gatt_attr *attrs;
	int service_index;

	service = copy_buffer_reserve(sizeof(struct bt_gatt_service_static));
	attrs = copy_buffer_reserve(sizeof(struct bt_gatt_attr) * attr_count);

	if (service == NULL || attrs == NULL) {
		return -NRF_ENOMEM;
	}

	service->attr_count = 0;
	service->attrs = attrs;

	service_index = add_service(service);

	if (service_index < 0) {
		return service_index;
	} else if (service_index != (int)remote_service_index) {
		return -NRF_EINVAL;
	}

	current_service = service;
	current_service_attr_max = attr_count;
	current_service_index = service_index;

	return 0;
}


static void bt_rpc_gatt_start_service_rpc_handler(CborValue *_value, void *_handler_data)/*####%Brek*/
{                                                                                        /*#####@cGw*/

	uint8_t service_index;                                                           /*######%AW*/
	size_t attr_count;                                                               /*######vB0*/
	int _result;                                                                     /*######@jQ*/

	service_index = ser_decode_uint(_value);                                         /*####%Cq49*/
	attr_count = ser_decode_uint(_value);                                            /*#####@6Sw*/

	if (!ser_decoding_done_and_check(_value)) {                                      /*######%FE*/
		goto decoding_error;                                                     /*######QTM*/
	}                                                                                /*######@1Y*/

	_result = bt_rpc_gatt_start_service(service_index, attr_count);                  /*##DrfWttU*/

	ser_rsp_send_int(_result);                                                       /*##BPC96+4*/

	return;                                                                          /*######%FR*/
decoding_error:                                                                          /*######7VP*/
	report_decoding_error(BT_RPC_GATT_START_SERVICE_RPC_CMD, _handler_data);         /*######@VU*/

}                                                                                        /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_start_service, BT_RPC_GATT_START_SERVICE_RPC_CMD,/*####%BlbU*/
	bt_rpc_gatt_start_service_rpc_handler, NULL);                                             /*#####@obo*/


int attr_to_index(const struct bt_gatt_attr *attr)
{
	int service_index;
	size_t service_map_index;
	size_t diff = (uint8_t *)attr - (uint8_t *)copy_buffer;
	
	if (diff > sizeof(copy_buffer) - sizeof(struct bt_gatt_attr)) {
		return -NRF_EINVAL;
	}

	service_map_index = diff / sizeof(struct bt_gatt_attr);

	service_index = service_map[service_map_index];

	return bt_rpc_gatt_srv_attr_to_index(service_index, attr);
}

static size_t bt_uuid_gatt_buf_size(const struct bt_uuid *uuid)
{
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

static struct bt_uuid *bt_uuid_gatt_dec(CborValue *value)
{
	uint8_t type;
	struct bt_uuid *result;
	void *dst;
	size_t buffer_size = ser_decode_buffer_size(value);

	if (buffer_size == sizeof(((struct bt_uuid_16 *)NULL)->val)) {

		struct bt_uuid_16 *uuid;
		
		uuid = copy_buffer_reserve(sizeof(struct bt_uuid_16));
		dst = &uuid->val;
		result = &uuid->uuid;
		type = BT_UUID_TYPE_16;

	} else if (buffer_size == sizeof(((struct bt_uuid_32 *)NULL)->val)) {

		struct bt_uuid_32 *uuid;
		
		uuid = copy_buffer_reserve(sizeof(struct bt_uuid_32));
		dst = &uuid->val;
		result = &uuid->uuid;
		type = BT_UUID_TYPE_32;

	} else if (buffer_size == sizeof(((struct bt_uuid_128 *)NULL)->val)) {

		struct bt_uuid_128 *uuid;
		
		uuid = copy_buffer_reserve(sizeof(struct bt_uuid_128));
		dst = &uuid->val;
		result = &uuid->uuid;
		type = BT_UUID_TYPE_128;

	} else {
		ser_decoder_invalid(value, CborErrorIllegalType);
		return NULL;
	}

	if (!result) {
		ser_decoder_invalid(value, CborErrorOutOfMemory);
		return NULL;
	}

	ser_decode_buffer(value, dst, buffer_size);

	return result;
}

static void add_user_attr(struct bt_gatt_attr *attr, const struct bt_uuid *uuid, uint16_t data)
{
	attr->uuid = uuid;
	attr->read = (data & BT_RPC_GATT_ATTR_READ_PRESENT_FLAG) ?
		     bt_rpc_normal_attr_read : NULL;
	attr->write = (data & BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG) ?
		      bt_rpc_normal_attr_write : NULL;
	attr->user_data = (void *)((current_service_index << 16) |
				   current_service->attr_count);
	attr->handle = 0;
	attr->perm = (uint8_t)data;
}


static void add_srv_attr(struct bt_gatt_attr *attr, const struct bt_uuid *service_uuid, const struct bt_uuid *attr_uuid)
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

	chrc = (struct bt_gatt_chrc *)copy_buffer_reserve(sizeof(struct bt_gatt_chrc));
	
	if (chrc == NULL) {
		return -ENOMEM;
	}

	chrc->uuid = uuid;
	chrc->properties = properties;
	chrc->value_handle = 0;

	attr->uuid = &uuid_chrc;
	attr->read = bt_gatt_attr_read_chrc;
	attr->write = NULL;
	attr->user_data = (void *)chrc;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

static int bt_rpc_gatt_send_simple_attr(uint8_t special_attr, const struct bt_uuid *uuid, uint16_t data)
{
	int res = 0;
	struct bt_gatt_attr *attr;

	if (current_service == NULL ||
	    current_service->attr_count >= current_service_attr_max) {
		return -ENOMEM;
	}

	attr = &current_service->attr[current_service->attr_count];

	switch (special_attr)
	{
	case 0:
		add_user_attr(attr, uuid, data);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_SERVICE:
		add_srv_attr(attr, uuid, &uuid_primary);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_SECONDARY:
		add_srv_attr(attr, uuid, &uuid_secondary);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_CHRC:
		res = add_chrc_attr(attr, uuid, data);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_CEP:
		add_cep_attr(attr, uuid, data);
		break;	
	default:
		return -EINVAL;
	}

	if (res == 0) {
		current_service->attr_count++;
	}
	
	return res;
}

static void bt_rpc_gatt_send_simple_attr_rpc_handler(CborValue *_value, void *_handler_data)/*####%BqG/*/
{                                                                                           /*#####@maw*/

	uint8_t special_attr;                                                               /*######%AV*/
	uint16_t data;                                                                      /*######psE*/
	int _result;                                                                        /*######@m4*/

	struct bt_uuid *uuid;

	uuid = bt_uuid_gatt_dec(_value);

	special_attr = ser_decode_uint(_value);                                             /*####%Cg1/*/
	data = ser_decode_uint(_value);                                                     /*#####@RKA*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	_result = bt_rpc_gatt_send_simple_attr(special_attr, uuid, data);

	ser_rsp_send_int(_result);                                                          /*##BPC96+4*/

	return;                                                                             /*######%Fb*/
decoding_error:                                                                             /*######vuO*/
	report_decoding_error(BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD, _handler_data);         /*######@kY*/

}                                                                                           /*##B9ELNqo*/


static ssize_t read_cep_sync(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int res;
	struct bt_gatt_cep *value = attr->user_data;

	res = bt_rpc_gatt_get_cep_value(attr);
	if (res < 0) {
		return res;
	}

	value->properties = res;

	return bt_gatt_attr_read_cep(conn, attr, buf, len, offset);
}


static int add_cep_attr(struct bt_gatt_attr *attr, uint8_t rw_sync, uint16_t properties)
{
	struct bt_gatt_cep *cep;

	cep = (struct bt_gatt_cep *)copy_buffer_reserve(sizeof(struct bt_gatt_cep));
	
	if (cep == NULL) {
		return -ENOMEM;
	}

	cep->properties = properties;

	attr->uuid = &uuid_cep;
	attr->read = (rw_sync & BT_RPC_GATT_ATTR_SPECIAL_READ_SYNC) ? read_cep_sync : bt_gatt_attr_read_cep;
	attr->write = NULL;
	attr->user_data = (void *)cep;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

struct cud_container
{
	uint16_t max_size;
	char text[1];
};


static ssize_t read_cud_sync(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	int res;
	char *text = attr->user_data;
	struct cud_container *container;

	container = CONTAINER_OF(text, struct cud_container, text);

	res = bt_rpc_gatt_get_cud_value(attr, container->text, container->max_size);
	if (res < 0) {
		return res;
	}

	return bt_gatt_attr_read_cud(conn, attr, buf, len, offset);
}

static int add_cud_attr(struct bt_gatt_attr *attr, uint8_t rw_sync, uint8_t perm, char *text, size_t size)
{
	if (rw_sync & BT_RPC_GATT_ATTR_SPECIAL_READ_SYNC) {
		struct cud_container *container;
		size_t max_size = MAX(size, CONFIG_BT_RPC_CUD_STRING_LENGTH_MAX + 1);

		container = (struct cud_container *)copy_buffer_reserve(offsetof(struct cud_container, data) + max_size);
		
		if (container == NULL) {
			return -ENOMEM;
		}

		container->max_size = max_size;

		attr->read = read_cud_sync;
		attr->user_data = container->data;
	} else {
		attr->read =  bt_gatt_attr_read_cud;
		attr->user_data = copy_buffer_reserve(size);
		if (attr->user_data == NULL) {
			return -ENOMEM;
		}
	}

	attr->uuid = &uuid_cud;
	attr->write = NULL;
	attr->handle = 0;
	attr->perm = perm;
	memcpy(attr->user_data, text, size);

	return 0;
}

static ssize_t read_cpf_sync(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	int res;
	struct bt_gatt_cpf *value = attr->user_data;

	res = bt_rpc_gatt_get_cpf_value(attr, value);
	if (res < 0) {
		return res;
	}

	return bt_gatt_attr_read_cpf(conn, attr, buf, len, offset);
}

static int add_cpf_attr(struct bt_gatt_attr *attr, uint8_t rw_sync, uint8_t *buffer, size_t size)
{
	struct bt_gatt_cpf *cpf;

	cpf = (struct bt_gatt_cpf *)copy_buffer_reserve(sizeof(struct bt_gatt_cpf));
	
	if (cpf == NULL || size != sizeof(struct bt_gatt_cpf)) {
		return -ENOMEM;
	}

	memcpy(cpf, buffer, size);

	attr->uuid = &uuid_cpf;
	attr->read = (rw_sync & BT_RPC_GATT_ATTR_SPECIAL_READ_SYNC) ? read_cpf_sync : bt_gatt_attr_read_cpf;
	attr->write = NULL;
	attr->user_data = (void *)cpf;
	attr->handle = 0;
	attr->perm = BT_GATT_PERM_READ;

	return 0;
}

static int bt_rpc_gatt_send_desc_attr(uint8_t special_attr, uint8_t rw_sync, uint16_t param, uint8_t *buffer, size_t size)
{

	int res = 0;
	struct bt_gatt_attr *attr;

	if (current_service == NULL ||
	    current_service->attr_count >= current_service_attr_max) {
		return -ENOMEM;
	}

	attr = &current_service->attr[current_service->attr_count];

	switch (special_attr)
	{
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_CEP:
		add_cep_attr(attr, rw_sync, param);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_CUD:
		add_cud_attr(attr, rw_sync, param, buffer, size);
		break;
	case (uint8_t)BT_RPC_GATT_ATTR_SPECIAL_CPF:
		add_cpf_attr(attr, rw_sync, buffer, size);
		break;
	default:
		return -EINVAL;
	}

	if (res == 0) {
		current_service->attr_count++;
	}
	
	return res;
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_send_simple_attr, BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,/*####%BgAv*/
	bt_rpc_gatt_send_simple_attr_rpc_handler, NULL);                                                /*#####@ah8*/

static void bt_rpc_gatt_send_desc_attr_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bjop*/
{                                                                                         /*#####@IJw*/

	uint8_t special_attr;                                                             /*#######%A*/
	uint8_t rw_sync;                                                                  /*#######UO*/
	uint16_t param;                                                                   /*########L*/
	size_t size;                                                                      /*########e*/
	uint8_t * buffer;                                                                 /*########6*/
	int _result;                                                                      /*########U*/
	struct ser_scratchpad _scratchpad;                                                /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                       /*##EZKHjKY*/

	special_attr = ser_decode_uint(_value);                                           /*#######%C*/
	rw_sync = ser_decode_uint(_value);                                                /*#######kI*/
	param = ser_decode_uint(_value);                                                  /*#######Nn*/
	size = ser_decode_uint(_value);                                                   /*#######Po*/
	buffer = ser_decode_buffer_sp(&_scratchpad);                                      /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                       /*######%FE*/
		goto decoding_error;                                                      /*######QTM*/
	}                                                                                 /*######@1Y*/

	_result = bt_rpc_gatt_send_desc_attr(special_attr, rw_sync, param, buffer, size); /*##DhCddd0*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                        /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                        /*##BPC96+4*/

	return;                                                                           /*######%FR*/
decoding_error:                                                                           /*#######/R*/
	report_decoding_error(BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD, _handler_data);         /*#######gX*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                                /*#######@s*/

}                                                                                         /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_send_desc_attr, BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,/*####%BgQJ*/
	bt_rpc_gatt_send_desc_attr_rpc_handler, NULL);                                              /*#####@fws*/

static int bt_rpc_gatt_end_service(void)
{
	int res;
	
	res = bt_gatt_service_register(current_service);
	current_service = NULL;
	current_service_attr_max = 0;

	return res;
}

static void bt_rpc_gatt_end_service_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bi0y*/
{                                                                                      /*#####@k5U*/

	int _result;                                                                   /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                            /*##FGkSPWY*/

	_result = bt_rpc_gatt_end_service();                                           /*##DusAZ1Y*/

	ser_rsp_send_int(_result);                                                     /*##BPC96+4*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_gatt_end_service, BT_RPC_GATT_END_SERVICE_RPC_CMD,/*####%Bq8s*/
	bt_rpc_gatt_end_service_rpc_handler, NULL);                                           /*#####@xJU*/

