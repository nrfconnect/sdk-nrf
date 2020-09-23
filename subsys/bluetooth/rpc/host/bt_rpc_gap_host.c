/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"


#ifndef __GENERATOR
#define UNUSED __attribute__((unused)) /* TODO: Improve generator to avoid this workaround */
#else
#define UNUSED ;
#endif


K_MUTEX_DEFINE(bt_rpc_gap_cache_mutex);
#define LOCK_GAP_CACHE() k_mutex_lock(&bt_rpc_gap_cache_mutex, K_FOREVER)
#define UNLOCK_GAP_CACHE() k_mutex_unlock(&bt_rpc_gap_cache_mutex)

static void report_decoding_error(uint8_t cmd_evt_id, void* data) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static void bt_rpc_get_check_table_rpc_handler(CborValue *_value, void *_handler_data)/*####%BoCa*/
{                                                                                     /*#####@raA*/

	struct nrf_rpc_cbor_ctx _ctx;                                                 /*#######%A*/
	size_t size;                                                                  /*#######TW*/
	uint8_t * data;                                                               /*#######hm*/
	size_t _buffer_size_max = 5;                                                  /*#######Yg*/
	struct ser_scratchpad _scratchpad;                                            /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                   /*##EZKHjKY*/

	size = ser_decode_uint(_value);                                               /*####%Cl/C*/
	data = ser_scratchpad_get(&_scratchpad, sizeof(uint8_t) * size);              /*#####@bC8*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	bt_rpc_get_check_table(data, size);                                           /*##Dju7zA4*/

	_buffer_size_max += sizeof(uint8_t) * size;                                   /*##CFMA56g*/

	{                                                                             /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                           /*#####@zxs*/

		ser_encode_buffer(&_ctx.encoder, data, sizeof(uint8_t) * size);       /*##DNObIzM*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                    /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                       /*####%BIlG*/
	}                                                                             /*#####@TnU*/

	return;                                                                       /*######%Fc*/
decoding_error:                                                                       /*#######jF*/
	report_decoding_error(BT_RPC_GET_CHECK_TABLE_RPC_CMD, _handler_data);         /*#######3H*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                            /*#######@U*/

}                                                                                     /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_get_check_table, BT_RPC_GET_CHECK_TABLE_RPC_CMD,/*####%Bg3e*/
	bt_rpc_get_check_table_rpc_handler, NULL);                                          /*#####@qXw*/


static inline void bt_ready_cb_t_callback(int err,
					  uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_ready_cb_t));
	SERIALIZE(EVENT);

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%AZLc*/
	size_t _buffer_size_max = 8;                                             /*#####@cfw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_int(&_ctx.encoder, err);                                      /*####%A5c9*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                  /*#####@gpY*/

	nrf_rpc_cbor_evt_no_err(&bt_rpc_grp,                                     /*####%BEEf*/
		BT_READY_CB_T_CALLBACK_RPC_EVT, &_ctx);                          /*#####@NOk*/
}

CBKPROXY_HANDLER(bt_ready_cb_t_encoder, bt_ready_cb_t_callback, (int err), (err));

static void bt_enable_rpc_handler(CborValue *_value, void *_handler_data)        /*####%Bims*/
{                                                                                /*#####@U54*/

	bt_ready_cb_t cb;                                                        /*####%AX3q*/
	int _result;                                                             /*#####@ImM*/

	cb = (bt_ready_cb_t)ser_decode_callback(_value, bt_ready_cb_t_encoder);  /*##Ctv5nEY*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_enable(cb);                                                 /*##DqdsuHg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%FX*/
decoding_error:                                                                  /*######yR5*/
	report_decoding_error(BT_ENABLE_RPC_CMD, _handler_data);                 /*######@B4*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_enable, BT_ENABLE_RPC_CMD,               /*####%BmEu*/
	bt_enable_rpc_handler, NULL);                                            /*#####@rg4*/



#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) || defined(__GENERATOR)

static void bt_set_name_rpc_handler(CborValue *_value, void *_handler_data)      /*####%BgLQ*/
{                                                                                /*#####@EdU*/

	const char * name;                                                       /*######%AW*/
	int _result;                                                             /*######V1+*/
	struct ser_scratchpad _scratchpad;                                       /*######@Wo*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	name = ser_decode_str_sp(&_scratchpad);                                  /*##Ct1cGC4*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_set_name(name);                                             /*##Du4mNCE*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%Fb*/
decoding_error:                                                                  /*#######LD*/
	report_decoding_error(BT_SET_NAME_RPC_CMD, _handler_data);               /*#######8V*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@Y*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_name, BT_SET_NAME_RPC_CMD,           /*####%Bsnj*/
	bt_set_name_rpc_handler, NULL);                                          /*#####@Z6Q*/

static bool bt_get_name_out(char *name, size_t size)
{
	const char *src;

	src = bt_get_name();

	if (src == NULL) {
		strcpy(name, "");
		return false;
	} else {
		strncpy(name, src, size);
		return true;
	}
}

static void bt_get_name_out_rpc_handler(CborValue *_value, void *_handler_data)  /*####%Buty*/
{                                                                                /*#####@Q0A*/

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	bool _result;                                                            /*#######dn*/
	size_t size;                                                             /*########x*/
	size_t _name_strlen;                                                     /*########+*/
	char * name;                                                             /*########1*/
	size_t _buffer_size_max = 6;                                             /*########4*/
	struct ser_scratchpad _scratchpad;                                       /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	size = ser_decode_uint(_value);                                          /*##Ckcz6jM*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	name = ser_scratchpad_get(&_scratchpad, size);                           /*##D4mwWl8*/

	_result = bt_get_name_out(name, size);                                   /*##DoiOW8M*/

	_name_strlen = strlen(name);                                             /*####%CFOk*/
	_buffer_size_max += _name_strlen;                                        /*#####@f8c*/

	{                                                                        /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                      /*#####@zxs*/

		ser_encode_bool(&_ctx.encoder, _result);                         /*####%DEUe*/
		ser_encode_str(&_ctx.encoder, name, _name_strlen);               /*#####@0+w*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                  /*####%BIlG*/
	}                                                                        /*#####@TnU*/

	return;                                                                  /*######%FV*/
decoding_error:                                                                  /*#######ED*/
	report_decoding_error(BT_GET_NAME_OUT_RPC_CMD, _handler_data);           /*#######bw*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@M*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_get_name_out, BT_GET_NAME_OUT_RPC_CMD,   /*####%BuEw*/
	bt_get_name_out_rpc_handler, NULL);                                      /*#####@65I*/

#endif /* defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) || defined(__GENERATOR) */

static void bt_set_id_addr_rpc_handler(CborValue *_value, void *_handler_data)   /*####%BvyT*/
{                                                                                /*#####@wb0*/

	bt_addr_le_t _addr_data;                                                 /*######%AX*/
	const bt_addr_le_t * addr;                                               /*######+Ee*/
	int _result;                                                             /*######@rM*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));     /*##CmLJMDg*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_set_id_addr(addr);                                          /*##DuZOZN8*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%Fa*/
decoding_error:                                                                  /*######5Mq*/
	report_decoding_error(BT_SET_ID_ADDR_RPC_CMD, _handler_data);            /*######@9c*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_id_addr, BT_SET_ID_ADDR_RPC_CMD,     /*####%BhZm*/
	bt_set_id_addr_rpc_handler, NULL);                                       /*#####@pXg*/

static void bt_id_get_rpc_handler(CborValue *_value, void *_handler_data)              /*####%Bgag*/
{                                                                                      /*#####@m9Q*/

	struct nrf_rpc_cbor_ctx _ctx;                                                  /*#######%A*/
	size_t _count_data;                                                            /*#######Uj*/
	size_t * count = &_count_data;                                                 /*#######2X*/
	bt_addr_le_t * addrs;                                                          /*########C*/
	size_t _buffer_size_max = 10;                                                  /*########k*/
	struct ser_scratchpad _scratchpad;                                             /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                    /*##EZKHjKY*/

	*count = ser_decode_uint(_value);                                              /*##CgKRZw4*/

	if (!ser_decoding_done_and_check(_value)) {                                    /*######%FE*/
		goto decoding_error;                                                   /*######QTM*/
	}                                                                              /*######@1Y*/

	addrs = ser_scratchpad_get(&_scratchpad, *count * sizeof(bt_addr_le_t));       /*##DyLjbkg*/

	bt_id_get(addrs, count);                                                       /*##DsYIgTQ*/

	_buffer_size_max += *count * sizeof(bt_addr_le_t);                             /*##CO2r4Ws*/

	{                                                                              /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                            /*#####@zxs*/

		ser_encode_uint(&_ctx.encoder, *count);                                /*####%DKBB*/
		ser_encode_buffer(&_ctx.encoder, addrs, *count * sizeof(bt_addr_le_t));/*#####@ACo*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                     /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                        /*####%BIlG*/
	}                                                                              /*#####@TnU*/

	return;                                                                        /*######%FR*/
decoding_error:                                                                        /*#######Cw*/
	report_decoding_error(BT_ID_GET_RPC_CMD, _handler_data);                       /*#######dX*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                             /*#######@Q*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_get, BT_ID_GET_RPC_CMD,               /*####%BgXJ*/
	bt_id_get_rpc_handler, NULL);                                            /*#####@Qlc*/

static void bt_id_create_rpc_handler(CborValue *_value, void *_handler_data)     /*####%BnmX*/
{                                                                                /*#####@/TY*/

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	int _result;                                                             /*#######RE*/
	bt_addr_le_t _addr_data;                                                 /*########X*/
	bt_addr_le_t * addr;                                                     /*########x*/
	uint8_t * irk;                                                           /*########i*/
	size_t _buffer_size_max = 13;                                            /*########Y*/
	struct ser_scratchpad _scratchpad;                                       /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));     /*####%CgHr*/
	irk = ser_decode_buffer_sp(&_scratchpad);                                /*#####@AWw*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_id_create(addr, irk);                                       /*##Di/SBrY*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*####%CN0D*/
	_buffer_size_max += !irk ? 0 : sizeof(uint8_t) * 16;                     /*#####@JH4*/

	{                                                                        /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                      /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                          /*######%DK*/
		ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));    /*######N94*/
	ser_encode_buffer(&_ctx.encoder, irk, sizeof(uint8_t) * 16);             /*######@cE*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                  /*####%BIlG*/
	}                                                                        /*#####@TnU*/

	return;                                                                  /*######%FQ*/
decoding_error:                                                                  /*#######y4*/
	report_decoding_error(BT_ID_CREATE_RPC_CMD, _handler_data);              /*#######hE*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@c*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_create, BT_ID_CREATE_RPC_CMD,         /*####%BjKU*/
	bt_id_create_rpc_handler, NULL);                                         /*#####@jso*/

static void bt_id_reset_rpc_handler(CborValue *_value, void *_handler_data)      /*####%BoL+*/
{                                                                                /*#####@SEk*/

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	int _result;                                                             /*########W*/
	uint8_t id;                                                              /*########1*/
	bt_addr_le_t _addr_data;                                                 /*########3*/
	bt_addr_le_t * addr;                                                     /*########8*/
	uint8_t * irk;                                                           /*########A*/
	size_t _buffer_size_max = 13;                                            /*########k*/
	struct ser_scratchpad _scratchpad;                                       /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	id = ser_decode_uint(_value);                                            /*######%Cu*/
	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));     /*######qQx*/
	irk = ser_decode_buffer_sp(&_scratchpad);                                /*######@Eg*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_id_reset(id, addr, irk);                                    /*##DuJfNtE*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*####%CN0D*/
	_buffer_size_max += !irk ? 0 : sizeof(uint8_t) * 16;                     /*#####@JH4*/

	{                                                                        /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                      /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                          /*######%DK*/
		ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));    /*######N94*/
	ser_encode_buffer(&_ctx.encoder, irk, sizeof(uint8_t) * 16);             /*######@cE*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                  /*####%BIlG*/
	}                                                                        /*#####@TnU*/

	return;                                                                  /*######%FS*/
decoding_error:                                                                  /*#######LD*/
	report_decoding_error(BT_ID_RESET_RPC_CMD, _handler_data);               /*#######5v*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@w*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_reset, BT_ID_RESET_RPC_CMD,           /*####%BrEb*/
	bt_id_reset_rpc_handler, NULL);                                          /*#####@RSc*/

static void bt_id_delete_rpc_handler(CborValue *_value, void *_handler_data)     /*####%Bn4o*/
{                                                                                /*#####@h9M*/

	uint8_t id;                                                              /*####%Abyj*/
	int _result;                                                             /*#####@HaA*/

	id = ser_decode_uint(_value);                                            /*##Cgs+Vr8*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_id_delete(id);                                              /*##DilWuoM*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%Fe*/
decoding_error:                                                                  /*######HJL*/
	report_decoding_error(BT_ID_DELETE_RPC_CMD, _handler_data);              /*######@yo*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_delete, BT_ID_DELETE_RPC_CMD,         /*####%BibD*/
	bt_id_delete_rpc_handler, NULL);                                         /*#####@9Jg*/

void bt_data_dec(struct ser_scratchpad *_scratchpad, struct bt_data *_data)      /*####%Bne5*/
{                                                                                /*#####@+3g*/

	CborValue *_value = _scratchpad->value;                                  /*##AU3cSLw*/

	_data->type = ser_decode_uint(_value);                                   /*######%Ck*/
	_data->data_len = ser_decode_uint(_value);                               /*######cDJ*/
	_data->data = ser_decode_buffer_sp(_scratchpad);                         /*######@Vg*/

}                                                                                /*##B9ELNqo*/

void bt_le_scan_param_dec(CborValue *_value, struct bt_le_scan_param *_data)     /*####%Bjc4*/
{                                                                                /*#####@EWA*/

	_data->type = ser_decode_uint(_value);                                   /*#######%C*/
	_data->options = ser_decode_uint(_value);                                /*#######sD*/
	_data->interval = ser_decode_uint(_value);                               /*########m*/
	_data->window = ser_decode_uint(_value);                                 /*########T*/
	_data->timeout = ser_decode_uint(_value);                                /*########W*/
	_data->interval_coded = ser_decode_uint(_value);                         /*########c*/
	_data->window_coded = ser_decode_uint(_value);                           /*########@*/

}                                                                                /*##B9ELNqo*/


size_t net_buf_simple_sp_size(struct net_buf_simple *_data)
{
	return SCRATCHPAD_ALIGN(_data->len);
}

size_t net_buf_simple_buf_size(struct net_buf_simple *_data)
{
	return 3 + _data->len;
}

void net_buf_simple_enc(CborEncoder *_encoder, struct net_buf_simple *_data)
{
	SERIALIZE(CUSTOM_STRUCT(struct net_buf_simple));

	ser_encode_buffer(_encoder, _data->data, _data->len);
}


static inline void bt_le_scan_cb_t_callback(const bt_addr_le_t *addr,
					    int8_t rssi, uint8_t adv_type,
					    struct net_buf_simple *buf,
					    uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_le_scan_cb_t *));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AQ*/
	size_t _scratchpad_size = 0;                                             /*######Krt*/
	size_t _buffer_size_max = 15;                                            /*######@KM*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*####%CE/i*/
	_buffer_size_max += net_buf_simple_buf_size(buf);                        /*#####@n2Y*/

	_scratchpad_size += net_buf_simple_sp_size(buf);                         /*##EGDrauo*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));            /*#######%A*/
	ser_encode_int(&_ctx.encoder, rssi);                                     /*#######7/*/
	ser_encode_uint(&_ctx.encoder, adv_type);                                /*#######+b*/
	net_buf_simple_enc(&_ctx.encoder, buf);                                  /*#######AI*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                  /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,   /*####%BAqD*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@cTU*/
}


CBKPROXY_HANDLER(bt_le_scan_cb_t_encoder, bt_le_scan_cb_t_callback,
		 (const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		  struct net_buf_simple *buf), (addr, rssi, adv_type, buf));

void bt_le_adv_param_dec(struct ser_scratchpad *_scratchpad, struct bt_le_adv_param *_data)/*####%Bklr*/
{                                                                                          /*#####@MEU*/

	CborValue *_value = _scratchpad->value;                                            /*##AU3cSLw*/

	_data->id = ser_decode_uint(_value);                                               /*#######%C*/
	_data->sid = ser_decode_uint(_value);                                              /*#######uD*/
	_data->secondary_max_skip = ser_decode_uint(_value);                               /*########8*/
	_data->options = ser_decode_uint(_value);                                          /*########H*/
	_data->interval_min = ser_decode_uint(_value);                                     /*########O*/
	_data->interval_max = ser_decode_uint(_value);                                     /*########8*/
	_data->peer = ser_decode_buffer_sp(_scratchpad);                                   /*########@*/

}                                                                                          /*##B9ELNqo*/

static void bt_le_adv_start_rpc_handler(CborValue *_value, void *_handler_data)  /*####%BkMc*/
{                                                                                /*#####@+8A*/

	struct bt_le_adv_param param;                                            /*#######%A*/
	size_t ad_len;                                                           /*########c*/
	struct bt_data * ad;                                                     /*########X*/
	size_t sd_len;                                                           /*########6*/
	struct bt_data * sd;                                                     /*########P*/
	int _result;                                                             /*########U*/
	struct ser_scratchpad _scratchpad;                                       /*########4*/
	size_t _i;                                                               /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                              /*##EZKHjKY*/

	bt_le_adv_param_dec(&_scratchpad, &param);                               /*########%*/
	ad_len = ser_decode_uint(_value);                                        /*########C*/
	ad = ser_scratchpad_get(&_scratchpad, ad_len * sizeof(struct bt_data));  /*########j*/
	if (ad == NULL) {                                                        /*########c*/
		goto decoding_error;                                             /*########r*/
	}                                                                        /*########S*/
	for (_i = 0; _i < ad_len; _i++) {                                        /*########q*/
		bt_data_dec(&_scratchpad, &ad[_i]);                              /*########E*/
	}                                                                        /*#########*/
	sd_len = ser_decode_uint(_value);                                        /*#########*/
	sd = ser_scratchpad_get(&_scratchpad, sd_len * sizeof(struct bt_data));  /*#########*/
	if (sd == NULL) {                                                        /*#########*/
		goto decoding_error;                                             /*#########*/
	}                                                                        /*#########*/
	for (_i = 0; _i < sd_len; _i++) {                                        /*#########*/
		bt_data_dec(&_scratchpad, &sd[_i]);                              /*#########*/
	}                                                                        /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_le_adv_start(&param, ad, ad_len, sd, sd_len);               /*##DovAJXw*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%FX*/
decoding_error:                                                                  /*#######q+*/
	report_decoding_error(BT_LE_ADV_START_RPC_CMD, _handler_data);           /*#######5i*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######@o*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_start, BT_LE_ADV_START_RPC_CMD,   /*####%BpKn*/
	bt_le_adv_start_rpc_handler, NULL);                                      /*#####@Slg*/

static void bt_le_adv_update_data_rpc_handler(CborValue *_value, void *_handler_data)/*####%Buyg*/
{                                                                                    /*#####@rwo*/

	size_t ad_len;                                                               /*#######%A*/
	struct bt_data * ad;                                                         /*#######Wg*/
	size_t sd_len;                                                               /*########y*/
	struct bt_data * sd;                                                         /*########y*/
	int _result;                                                                 /*########s*/
	struct ser_scratchpad _scratchpad;                                           /*########8*/
	size_t _i;                                                                   /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                  /*##EZKHjKY*/

	ad_len = ser_decode_uint(_value);                                            /*########%*/
	ad = ser_scratchpad_get(&_scratchpad, ad_len * sizeof(struct bt_data));      /*########C*/
	if (ad == NULL) {                                                            /*########j*/
		goto decoding_error;                                                 /*########J*/
	}                                                                            /*########0*/
	for (_i = 0; _i < ad_len; _i++) {                                            /*########C*/
		bt_data_dec(&_scratchpad, &ad[_i]);                                  /*########i*/
	}                                                                            /*########M*/
	sd_len = ser_decode_uint(_value);                                            /*#########*/
	sd = ser_scratchpad_get(&_scratchpad, sd_len * sizeof(struct bt_data));      /*#########*/
	if (sd == NULL) {                                                            /*#########*/
		goto decoding_error;                                                 /*#########*/
	}                                                                            /*#########*/
	for (_i = 0; _i < sd_len; _i++) {                                            /*#########*/
		bt_data_dec(&_scratchpad, &sd[_i]);                                  /*#########*/
	}                                                                            /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	_result = bt_le_adv_update_data(ad, ad_len, sd, sd_len);                     /*##DkrIIuE*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                   /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

	return;                                                                      /*######%Fd*/
decoding_error:                                                                      /*#######Y1*/
	report_decoding_error(BT_LE_ADV_UPDATE_DATA_RPC_CMD, _handler_data);         /*#######wc*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                           /*#######@I*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_update_data, BT_LE_ADV_UPDATE_DATA_RPC_CMD,/*####%BpKp*/
	bt_le_adv_update_data_rpc_handler, NULL);                                         /*#####@XTI*/

static void bt_le_adv_stop_rpc_handler(CborValue *_value, void *_handler_data)   /*####%BlBQ*/
{                                                                                /*#####@4G8*/

	int _result;                                                             /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                      /*##FGkSPWY*/

	_result = bt_le_adv_stop();                                              /*##Du9+9xY*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_stop, BT_LE_ADV_STOP_RPC_CMD,     /*####%BpS4*/
	bt_le_adv_stop_rpc_handler, NULL);                                       /*#####@7tk*/

size_t bt_le_oob_buf_size(const struct bt_le_oob *_data)                         /*####%Bq7Y*/
{                                                                                /*#####@wVE*/

	size_t _buffer_size_max = 13;                                            /*##Adi6hug*/

	_buffer_size_max += sizeof(bt_addr_le_t);                                /*######%CH*/
	_buffer_size_max += 16 * sizeof(uint8_t);                                /*######4Ws*/
	_buffer_size_max += 16 * sizeof(uint8_t);                                /*######@J0*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

void bt_le_oob_enc(CborEncoder *_encoder, const struct bt_le_oob *_data)         /*####%Bot9*/
{                                                                                /*#####@mlg*/

	SERIALIZE(STRUCT(struct bt_le_oob));
	SERIALIZE(STRUCT_INLINE(le_sc_data));

	ser_encode_buffer(_encoder, &_data->addr, sizeof(bt_addr_le_t));         /*######%A/*/
	ser_encode_buffer(_encoder, _data->le_sc_data.r, 16 * sizeof(uint8_t));  /*######M73*/
	ser_encode_buffer(_encoder, _data->le_sc_data.c, 16 * sizeof(uint8_t));  /*######@n8*/

}                                                                                /*##B9ELNqo*/

#if defined(CONFIG_BT_EXT_ADV) || defined(__GENERATOR)

static struct bt_le_ext_adv_cb ext_adv_cb_cache[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static uint8_t ext_adv_cb_cache_map[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
BT_RPC_POOL_DEFINE(ext_adv_cb_cache_pool, CONFIG_BT_EXT_ADV_MAX_ADV_SET);


UNUSED
static const size_t bt_le_ext_adv_sent_info_buf_size = 2;                        /*##Bmv+ZKI*/

void bt_le_ext_adv_sent_info_enc(CborEncoder *_encoder, const struct bt_le_ext_adv_sent_info *_data)/*####%Bj+O*/
{                                                                                                   /*#####@MwM*/

	SERIALIZE(STRUCT(struct bt_le_ext_adv_sent_info));

	ser_encode_uint(_encoder, _data->num_sent);                                                 /*##A6QnKbg*/

}                                                                                                   /*##B9ELNqo*/

UNUSED
static const size_t bt_le_ext_adv_connected_info_buf_size = 3;                   /*##BlJ3Oas*/

void bt_le_ext_adv_connected_info_enc(CborEncoder *_encoder, const struct bt_le_ext_adv_connected_info *_data)/*####%Bq/K*/
{                                                                                                             /*#####@F68*/

	SERIALIZE(STRUCT(struct bt_le_ext_adv_connected_info));

	encode_bt_conn(_encoder, _data->conn);                                                                /*##Ay+jNcI*/

}                                                                                                             /*##B9ELNqo*/

size_t bt_le_ext_adv_scanned_info_sp_size(const struct bt_le_ext_adv_scanned_info *_data)/*####%Bo2u*/
{                                                                                        /*#####@XAI*/

	size_t _scratchpad_size = 0;                                                     /*##ATz5YrA*/

	_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));                      /*##EGltS+k*/

	return _scratchpad_size;                                                         /*##BRWAmyU*/

}                                                                                        /*##B9ELNqo*/

size_t bt_le_ext_adv_scanned_info_buf_size(const struct bt_le_ext_adv_scanned_info *_data)/*####%Bkaj*/
{                                                                                         /*#####@WaQ*/

	size_t _buffer_size_max = 3;                                                      /*##AZjzmP0*/

	_buffer_size_max += sizeof(bt_addr_le_t);                                         /*##CN3eLBo*/

	return _buffer_size_max;                                                          /*##BWmN6G8*/

}                                                                                         /*##B9ELNqo*/

void bt_le_ext_adv_scanned_info_enc(CborEncoder *_encoder, const struct bt_le_ext_adv_scanned_info *_data)/*####%BrZr*/
{                                                                                                         /*#####@PPI*/

	SERIALIZE(STRUCT(struct bt_le_ext_adv_scanned_info));

	ser_encode_buffer(_encoder, _data->addr, sizeof(bt_addr_le_t));                                   /*##AwIBavw*/

}                                                                                                         /*##B9ELNqo*/

static inline
void bt_le_ext_adv_cb_sent_callback(struct bt_le_ext_adv *adv,
				    struct bt_le_ext_adv_sent_info *info,
				    uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_le_ext_adv_cb_sent));

	struct nrf_rpc_cbor_ctx _ctx;                                               /*####%ATwe*/
	size_t _buffer_size_max = 10;                                               /*#####@Gjo*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                 /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, (uintptr_t)adv);                             /*######%Aw*/
	bt_le_ext_adv_sent_info_enc(&_ctx.encoder, info);                           /*######FJo*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                     /*######@LM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_SENT_CALLBACK_RPC_CMD,/*####%BCt2*/
		&_ctx, ser_rsp_simple_void, NULL);                                  /*#####@8kU*/
}

CBKPROXY_HANDLER(bt_le_ext_adv_cb_sent_encoder, bt_le_ext_adv_cb_sent_callback,
	(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info),
	(adv, info));

static inline
void bt_le_ext_adv_cb_connected_callback(struct bt_le_ext_adv *adv,
				    struct bt_le_ext_adv_connected_info *info,
				    uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_le_ext_adv_cb_connected));

	struct nrf_rpc_cbor_ctx _ctx;                                                    /*####%ARzi*/
	size_t _buffer_size_max = 11;                                                    /*#####@x8o*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                      /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, (uintptr_t)adv);                                  /*######%A5*/
	bt_le_ext_adv_connected_info_enc(&_ctx.encoder, info);                           /*######BD8*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                          /*######@2Q*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_CONNECTED_CALLBACK_RPC_CMD,/*####%BNbj*/
		&_ctx, ser_rsp_simple_void, NULL);                                       /*#####@raQ*/
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
	SERIALIZE(CALLBACK(bt_le_ext_adv_cb_scanned));

	struct nrf_rpc_cbor_ctx _ctx;                                                  /*######%Ad*/
	size_t _scratchpad_size = 0;                                                   /*######WM+*/
	size_t _buffer_size_max = 13;                                                  /*######@9c*/

	_buffer_size_max += bt_le_ext_adv_scanned_info_buf_size(info);                 /*##CCVQerQ*/

	_scratchpad_size += bt_le_ext_adv_scanned_info_sp_size(info);                  /*##EOUtvz8*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                    /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                              /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, (uintptr_t)adv);                                /*######%Az*/
	bt_le_ext_adv_scanned_info_enc(&_ctx.encoder, info);                           /*######VQt*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                        /*######@qQ*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CB_SCANNED_CALLBACK_RPC_CMD,/*####%BN8J*/
		&_ctx, ser_rsp_simple_void, NULL);                                     /*#####@5t0*/
}

CBKPROXY_HANDLER(bt_le_ext_adv_cb_scanned_encoder,
	bt_le_ext_adv_cb_scanned_callback,
	(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info),
	(adv, info));

void bt_le_ext_adv_cb_dec(CborValue *_value, struct bt_le_ext_adv_cb *_data)                                           /*####%BguJ*/
{                                                                                                                      /*#####@Ks0*/

	_data->sent = (bt_le_ext_adv_cb_sent)ser_decode_callback(_value, bt_le_ext_adv_cb_sent_encoder);               /*######%Ck*/
	_data->connected = (bt_le_ext_adv_cb_connected)ser_decode_callback(_value, bt_le_ext_adv_cb_connected_encoder);/*######EVg*/
	_data->scanned = (bt_le_ext_adv_cb_scanned)ser_decode_callback(_value, bt_le_ext_adv_cb_scanned_encoder);      /*######@Xc*/

}                                                                                                                      /*##B9ELNqo*/

static void bt_le_ext_adv_create_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bhf+*/
{                                                                                   /*#####@/Lg*/

	struct nrf_rpc_cbor_ctx _ctx;                                               /*#######%A*/
	int _result;                                                                /*#######SJ*/
	struct bt_le_adv_param param;                                               /*########k*/
	struct bt_le_ext_adv *_adv_data;                                            /*########z*/
	struct bt_le_ext_adv ** adv = &_adv_data;                                   /*########c*/
	size_t _buffer_size_max = 10;                                               /*########Q*/
	struct ser_scratchpad _scratchpad;                                          /*########@*/

	int cb_index = -1;
	size_t adv_index;
	struct bt_le_ext_adv_cb *cb = NULL;

	_result = 0;

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                 /*##EZKHjKY*/

	bt_le_adv_param_dec(&_scratchpad, &param);                                  /*##CgwNiC4*/

	if (ser_decode_is_undefined(_value)) {
		ser_decode_skip(_value);
	} else {
		cb_index = bt_rpc_pool_reserve(&ext_adv_cb_cache_pool);
		if (cb_index >= 0) {
			cb = &ext_adv_cb_cache[cb_index];
			bt_le_ext_adv_cb_dec(_value, cb);
		} else {
			_adv_data = NULL;
			_result = -ENOMEM;
		}
	}

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);

	if (_result == 0) {
		_result = bt_le_ext_adv_create(&param, cb, adv);
	}

	if (_result == 0) {
		adv_index = bt_le_ext_adv_get_index(_adv_data);
		ext_adv_cb_cache_map[adv_index] = cb_index;
	} else {
		bt_rpc_pool_release(&ext_adv_cb_cache_pool, cb_index);
	}

	{                                                                           /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                         /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                             /*####%DA5r*/
		ser_encode_uint(&_ctx.encoder, (uintptr_t)(*adv));                  /*#####@hLA*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                  /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                     /*####%BIlG*/
	}                                                                           /*#####@TnU*/

	return;                                                                     /*######%Fc*/
decoding_error:                                                                     /*#######dF*/
	report_decoding_error(BT_LE_EXT_ADV_CREATE_RPC_CMD, _handler_data);         /*#######r5*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                          /*#######@Q*/

	bt_rpc_pool_release(&ext_adv_cb_cache_pool, cb_index);

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_create, BT_LE_EXT_ADV_CREATE_RPC_CMD,/*####%BmDg*/
	bt_le_ext_adv_create_rpc_handler, NULL);                                        /*#####@/Eg*/


void bt_le_ext_adv_start_param_dec(CborValue *_value, struct bt_le_ext_adv_start_param *_data)/*####%Bmcv*/
{                                                                                             /*#####@efo*/

	_data->timeout = ser_decode_uint(_value);                                             /*####%CqFq*/
	_data->num_events = ser_decode_uint(_value);                                          /*#####@gQs*/

}                                                                                             /*##B9ELNqo*/

static void bt_le_ext_adv_start_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bp9Z*/
{                                                                                  /*#####@3s0*/

	struct bt_le_ext_adv * adv;                                                /*######%AW*/
	struct bt_le_ext_adv_start_param param;                                    /*######OdA*/
	int _result;                                                               /*######@zU*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                     /*####%Ch45*/
	bt_le_ext_adv_start_param_dec(_value, &param);                             /*#####@ke4*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_le_ext_adv_start(adv, &param);                                /*##DunCU3s*/

	ser_rsp_send_int(_result);                                                 /*##BPC96+4*/

	return;                                                                    /*######%Fe*/
decoding_error:                                                                    /*######A6P*/
	report_decoding_error(BT_LE_EXT_ADV_START_RPC_CMD, _handler_data);         /*######@wg*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_start, BT_LE_EXT_ADV_START_RPC_CMD,/*####%Br9O*/
	bt_le_ext_adv_start_rpc_handler, NULL);                                       /*#####@Euo*/

static void bt_le_ext_adv_stop_rpc_handler(CborValue *_value, void *_handler_data)/*####%BtKz*/
{                                                                                 /*#####@fXM*/

	struct bt_le_ext_adv * adv;                                               /*####%AX6g*/
	int _result;                                                              /*#####@WOc*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                    /*##CovnGwc*/

	if (!ser_decoding_done_and_check(_value)) {                               /*######%FE*/
		goto decoding_error;                                              /*######QTM*/
	}                                                                         /*######@1Y*/

	_result = bt_le_ext_adv_stop(adv);                                        /*##DrNBryg*/

	ser_rsp_send_int(_result);                                                /*##BPC96+4*/

	return;                                                                   /*######%FR*/
decoding_error:                                                                   /*######P2m*/
	report_decoding_error(BT_LE_EXT_ADV_STOP_RPC_CMD, _handler_data);         /*######@14*/

}                                                                                 /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_stop, BT_LE_EXT_ADV_STOP_RPC_CMD,/*####%BghF*/
	bt_le_ext_adv_stop_rpc_handler, NULL);                                      /*#####@FAA*/

static void bt_le_ext_adv_set_data_rpc_handler(CborValue *_value, void *_handler_data)/*####%BsJY*/
{                                                                                     /*#####@uhw*/

	struct bt_le_ext_adv * adv;                                                   /*#######%A*/
	size_t ad_len;                                                                /*########e*/
	struct bt_data * ad;                                                          /*########Q*/
	size_t sd_len;                                                                /*########V*/
	struct bt_data * sd;                                                          /*########b*/
	int _result;                                                                  /*########N*/
	struct ser_scratchpad _scratchpad;                                            /*########I*/
	size_t _i;                                                                    /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                   /*##EZKHjKY*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                        /*########%*/
	ad_len = ser_decode_uint(_value);                                             /*########C*/
	ad = ser_scratchpad_get(&_scratchpad, ad_len * sizeof(struct bt_data));       /*########l*/
	if (ad == NULL) {                                                             /*########x*/
		goto decoding_error;                                                  /*########I*/
	}                                                                             /*########r*/
	for (_i = 0; _i < ad_len; _i++) {                                             /*########6*/
		bt_data_dec(&_scratchpad, &ad[_i]);                                   /*########U*/
	}                                                                             /*#########*/
	sd_len = ser_decode_uint(_value);                                             /*#########*/
	sd = ser_scratchpad_get(&_scratchpad, sd_len * sizeof(struct bt_data));       /*#########*/
	if (sd == NULL) {                                                             /*#########*/
		goto decoding_error;                                                  /*#########*/
	}                                                                             /*#########*/
	for (_i = 0; _i < sd_len; _i++) {                                             /*#########*/
		bt_data_dec(&_scratchpad, &sd[_i]);                                   /*#########*/
	}                                                                             /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	_result = bt_le_ext_adv_set_data(adv, ad, ad_len, sd, sd_len);                /*##Dm9i5+c*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                    /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                    /*##BPC96+4*/

	return;                                                                       /*######%FW*/
decoding_error:                                                                       /*#######PJ*/
	report_decoding_error(BT_LE_EXT_ADV_SET_DATA_RPC_CMD, _handler_data);         /*#######8x*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                            /*#######@I*/

}                                                                                     /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_set_data, BT_LE_EXT_ADV_SET_DATA_RPC_CMD,/*####%BobS*/
	bt_le_ext_adv_set_data_rpc_handler, NULL);                                          /*#####@YTk*/

static void bt_le_ext_adv_update_param_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bo1A*/
{                                                                                         /*#####@5sY*/

	struct bt_le_ext_adv * adv;                                                       /*######%AY*/
	struct bt_le_adv_param param;                                                     /*#######mK*/
	int _result;                                                                      /*#######M9*/
	struct ser_scratchpad _scratchpad;                                                /*#######@c*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                       /*##EZKHjKY*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                            /*####%Cpjp*/
	bt_le_adv_param_dec(&_scratchpad, &param);                                        /*#####@fv4*/

	if (!ser_decoding_done_and_check(_value)) {                                       /*######%FE*/
		goto decoding_error;                                                      /*######QTM*/
	}                                                                                 /*######@1Y*/

	_result = bt_le_ext_adv_update_param(adv, &param);                                /*##DjIgW08*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                        /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                        /*##BPC96+4*/

	return;                                                                           /*######%FS*/
decoding_error:                                                                           /*#######ss*/
	report_decoding_error(BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD, _handler_data);         /*#######R4*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                                /*#######@Q*/

}                                                                                         /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_update_param, BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD,/*####%Brbi*/
	bt_le_ext_adv_update_param_rpc_handler, NULL);                                              /*#####@5SI*/

static void bt_le_ext_adv_delete_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bng3*/
{                                                                                   /*#####@ydU*/

	struct bt_le_ext_adv * adv;                                                 /*####%AX6g*/
	int _result;                                                                /*#####@WOc*/

	size_t adv_index;
	size_t cb_index;

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                      /*##CovnGwc*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	adv_index = bt_le_ext_adv_get_index(adv);

	_result = bt_le_ext_adv_delete(adv);                                        /*##Dk4ka24*/

	if (adv_index <= CONFIG_BT_EXT_ADV_MAX_ADV_SET) {
		cb_index = ext_adv_cb_cache_map[adv_index];
		bt_rpc_pool_release(&ext_adv_cb_cache_pool, cb_index);
	}

	ser_rsp_send_int(_result);                                                  /*##BPC96+4*/

	return;                                                                     /*######%Fc*/
decoding_error:                                                                     /*######6uF*/
	report_decoding_error(BT_LE_EXT_ADV_DELETE_RPC_CMD, _handler_data);         /*######@Zw*/

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_delete, BT_LE_EXT_ADV_DELETE_RPC_CMD,/*####%BoK+*/
	bt_le_ext_adv_delete_rpc_handler, NULL);                                        /*#####@Hzk*/

static void bt_le_ext_adv_get_index_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bju3*/
{                                                                                      /*#####@GBw*/

	struct bt_le_ext_adv * adv;                                                    /*####%AUD1*/
	uint8_t _result;                                                               /*#####@iW8*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                         /*##CovnGwc*/

	if (!ser_decoding_done_and_check(_value)) {                                    /*######%FE*/
		goto decoding_error;                                                   /*######QTM*/
	}                                                                              /*######@1Y*/

	_result = bt_le_ext_adv_get_index(adv);                                        /*##DtNeWKM*/

	ser_rsp_send_uint(_result);                                                    /*##BJsBF7s*/

	return;                                                                        /*######%Fb*/
decoding_error:                                                                        /*######SXU*/
	report_decoding_error(BT_LE_EXT_ADV_GET_INDEX_RPC_CMD, _handler_data);         /*######@Mo*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_get_index, BT_LE_EXT_ADV_GET_INDEX_RPC_CMD,/*####%Blm4*/
	bt_le_ext_adv_get_index_rpc_handler, NULL);                                           /*#####@q5Y*/

void bt_le_ext_adv_info_dec(CborValue *_value, struct bt_le_ext_adv_info *_data) /*####%BvJh*/
{                                                                                /*#####@D8M*/

	_data->id = ser_decode_uint(_value);                                     /*####%CvH0*/
	_data->tx_power = ser_decode_int(_value);                                /*#####@OLs*/

}                                                                                /*##B9ELNqo*/

static void bt_le_ext_adv_get_info_rpc_handler(CborValue *_value, void *_handler_data)/*####%BjWD*/
{                                                                                     /*#####@+G0*/

	const struct bt_le_ext_adv * adv;                                             /*######%Ad*/
	struct bt_le_ext_adv_info info;                                               /*######eHl*/
	int _result;                                                                  /*######@Ok*/

	adv = (const struct bt_le_ext_adv *)ser_decode_uint(_value);                  /*####%Cqhr*/
	bt_le_ext_adv_info_dec(_value, &info);                                        /*#####@swc*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	_result = bt_le_ext_adv_get_info(adv, &info);                                 /*##Dn4aMCw*/

	ser_rsp_send_int(_result);                                                    /*##BPC96+4*/

	return;                                                                       /*######%FT*/
decoding_error:                                                                       /*######Qga*/
	report_decoding_error(BT_LE_EXT_ADV_GET_INFO_RPC_CMD, _handler_data);         /*######@Lk*/

}                                                                                     /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_get_info, BT_LE_EXT_ADV_GET_INFO_RPC_CMD,/*####%BvCT*/
	bt_le_ext_adv_get_info_rpc_handler, NULL);                                          /*#####@lMA*/

static void bt_le_ext_adv_oob_get_local_rpc_handler(CborValue *_value, void *_handler_data)/*####%BkEH*/
{                                                                                          /*#####@oTk*/

	struct nrf_rpc_cbor_ctx _ctx;                                                      /*#######%A*/
	int _result;                                                                       /*#######Yy*/
	struct bt_le_ext_adv * adv;                                                        /*#######T2*/
	struct bt_le_oob _oob_data;                                                        /*########w*/
	struct bt_le_oob * oob = &_oob_data;                                               /*########4*/
	size_t _buffer_size_max = 5;                                                       /*########@*/

	adv = (struct bt_le_ext_adv *)ser_decode_uint(_value);                             /*##CovnGwc*/

	if (!ser_decoding_done_and_check(_value)) {                                        /*######%FE*/
		goto decoding_error;                                                       /*######QTM*/
	}                                                                                  /*######@1Y*/

	_result = bt_le_ext_adv_oob_get_local(adv, oob);                                   /*##Drtpehc*/

	_buffer_size_max += bt_le_oob_buf_size(oob);                                       /*##CDZJbOk*/

	{                                                                                  /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                                    /*####%DCYN*/
		bt_le_oob_enc(&_ctx.encoder, oob);                                         /*#####@uAQ*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                            /*####%BIlG*/
	}                                                                                  /*#####@TnU*/

	return;                                                                            /*######%FS*/
decoding_error:                                                                            /*######+6v*/
	report_decoding_error(BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD, _handler_data);         /*######@6g*/

}                                                                                          /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_oob_get_local, BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD,/*####%Bo7+*/
	bt_le_ext_adv_oob_get_local_rpc_handler, NULL);                                               /*#####@PoM*/

#endif /* defined(CONFIG_BT_EXT_ADV) || defined(__GENERATOR) */

#if defined(CONFIG_BT_OBSERVER) || defined(__GENERATOR)

static void bt_le_scan_start_rpc_handler(CborValue *_value, void *_handler_data)     /*####%BtgB*/
{                                                                                    /*#####@oaA*/

	struct bt_le_scan_param param;                                               /*######%Aa*/
	bt_le_scan_cb_t *cb;                                                         /*######eZM*/
	int _result;                                                                 /*######@I4*/

	bt_le_scan_param_dec(_value, &param);                                        /*####%CtFV*/
	cb = (bt_le_scan_cb_t *)ser_decode_callback(_value, bt_le_scan_cb_t_encoder);/*#####@F4o*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	_result = bt_le_scan_start(&param, cb);                                      /*##DiKuFZA*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

	return;                                                                      /*######%FR*/
decoding_error:                                                                      /*######Kcd*/
	report_decoding_error(BT_LE_SCAN_START_RPC_CMD, _handler_data);              /*######@z0*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_start, BT_LE_SCAN_START_RPC_CMD, /*####%BnWD*/
	bt_le_scan_start_rpc_handler, NULL);                                     /*#####@Jb0*/

static void bt_le_scan_stop_rpc_handler(CborValue *_value, void *_handler_data)  /*####%BjGj*/
{                                                                                /*#####@TTc*/

	int _result;                                                             /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                      /*##FGkSPWY*/

	_result = bt_le_scan_stop();                                             /*##DjTVckk*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_stop, BT_LE_SCAN_STOP_RPC_CMD,   /*####%BtK8*/
	bt_le_scan_stop_rpc_handler, NULL);                                      /*#####@Dss*/


size_t bt_le_scan_recv_info_sp_size(const struct bt_le_scan_recv_info *_data)    /*####%Br2r*/
{                                                                                /*#####@MFQ*/

	size_t _scratchpad_size = 0;                                             /*##ATz5YrA*/

	_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));              /*##EGltS+k*/

	return _scratchpad_size;                                                 /*##BRWAmyU*/

}                                                                                /*##B9ELNqo*/

size_t bt_le_scan_recv_info_buf_size(const struct bt_le_scan_recv_info *_data)   /*####%BqRr*/
{                                                                                /*#####@tMY*/

	size_t _buffer_size_max = 21;                                            /*##AZIQJqE*/

	_buffer_size_max += sizeof(bt_addr_le_t);                                /*##CN3eLBo*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

void bt_le_scan_recv_info_enc(CborEncoder *_encoder, const struct bt_le_scan_recv_info *_data)/*####%BnPD*/
{                                                                                             /*#####@UcA*/

	SERIALIZE(STRUCT(struct bt_le_scan_recv_info));

	ser_encode_buffer(_encoder, _data->addr, sizeof(bt_addr_le_t));                       /*########%*/
	ser_encode_uint(_encoder, _data->sid);                                                /*########A*/
	ser_encode_int(_encoder, _data->rssi);                                                /*########y*/
	ser_encode_int(_encoder, _data->tx_power);                                            /*########c*/
	ser_encode_uint(_encoder, _data->adv_type);                                           /*########K*/
	ser_encode_uint(_encoder, _data->adv_props);                                          /*########5*/
	ser_encode_uint(_encoder, _data->interval);                                           /*########8*/
	ser_encode_uint(_encoder, _data->primary_phy);                                        /*########I*/
	ser_encode_uint(_encoder, _data->secondary_phy);                                      /*########@*/

}                                                                                             /*##B9ELNqo*/

void bt_le_scan_cb_recv(const struct bt_le_scan_recv_info *info,
			struct net_buf_simple *buf)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	size_t _scratchpad_size = 0;                                             /*######Sre*/
	size_t _buffer_size_max = 5;                                             /*######@mg*/

	_buffer_size_max += bt_le_scan_recv_info_buf_size(info);                 /*####%CFur*/
	_buffer_size_max += net_buf_simple_buf_size(buf);                        /*#####@/ZU*/

	_scratchpad_size += bt_le_scan_recv_info_sp_size(info);                  /*####%EOQg*/
	_scratchpad_size += net_buf_simple_sp_size(buf);                         /*#####@vZ8*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	bt_le_scan_recv_info_enc(&_ctx.encoder, info);                           /*####%Axiq*/
	net_buf_simple_enc(&_ctx.encoder, buf);                                  /*#####@N88*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_RECV_RPC_CMD,         /*####%BAPd*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@MIw*/
}

void bt_le_scan_cb_timeout(void)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%ATMv*/
	size_t _buffer_size_max = 0;                                             /*#####@1d4*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_CB_TIMEOUT_RPC_CMD,      /*####%BBaD*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@880*/
}

static struct bt_le_scan_cb scan_cb = {
	.recv = bt_le_scan_cb_recv,
	.timeout = bt_le_scan_cb_timeout,
};

static void bt_le_scan_cb_register_on_remote_rpc_handler(CborValue *_value, void *_handler_data)/*####%BkDX*/
{                                                                                               /*#####@c6E*/

	nrf_rpc_cbor_decoding_done(_value);                                                     /*##FGkSPWY*/

	SERIALIZE(CUSTOM_EXECUTE);
	bt_le_scan_cb_register(&scan_cb);

	ser_rsp_send_void();                                                                    /*##BEYGLxw*/

}                                                                                               /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_register_on_remote, BT_LE_SCAN_CB_REGISTER_ON_REMOTE_RPC_CMD,/*####%BhkM*/
	bt_le_scan_cb_register_on_remote_rpc_handler, NULL);                                                    /*#####@+9k*/

#endif /* defined(CONFIG_BT_OBSERVER) || defined(__GENERATOR) */

#if defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR)

static void bt_le_whitelist_add_rpc_handler(CborValue *_value, void *_handler_data)/*####%BvAc*/
{                                                                                  /*#####@JS8*/

	bt_addr_le_t _addr_data;                                                   /*######%AX*/
	const bt_addr_le_t * addr;                                                 /*######+Ee*/
	int _result;                                                               /*######@rM*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));       /*##CmLJMDg*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_le_whitelist_add(addr);                                       /*##DhFK5fs*/

	ser_rsp_send_int(_result);                                                 /*##BPC96+4*/

	return;                                                                    /*######%FQ*/
decoding_error:                                                                    /*######au+*/
	report_decoding_error(BT_LE_WHITELIST_ADD_RPC_CMD, _handler_data);         /*######@mE*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_whitelist_add, BT_LE_WHITELIST_ADD_RPC_CMD,/*####%BgQk*/
	bt_le_whitelist_add_rpc_handler, NULL);                                       /*#####@5W4*/

static void bt_le_whitelist_rem_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bq5A*/
{                                                                                  /*#####@p48*/

	bt_addr_le_t _addr_data;                                                   /*######%AX*/
	const bt_addr_le_t * addr;                                                 /*######+Ee*/
	int _result;                                                               /*######@rM*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));       /*##CmLJMDg*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_le_whitelist_rem(addr);                                       /*##Dpt0chE*/

	ser_rsp_send_int(_result);                                                 /*##BPC96+4*/

	return;                                                                    /*######%FX*/
decoding_error:                                                                    /*######6cv*/
	report_decoding_error(BT_LE_WHITELIST_REM_RPC_CMD, _handler_data);         /*######@ss*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_whitelist_rem, BT_LE_WHITELIST_REM_RPC_CMD,/*####%Bins*/
	bt_le_whitelist_rem_rpc_handler, NULL);                                       /*#####@M5A*/

static void bt_le_whitelist_clear_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bs7f*/
{                                                                                    /*#####@2mU*/

	int _result;                                                                 /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                          /*##FGkSPWY*/

	_result = bt_le_whitelist_clear();                                           /*##DjMtZbs*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_whitelist_clear, BT_LE_WHITELIST_CLEAR_RPC_CMD,/*####%BuWk*/
	bt_le_whitelist_clear_rpc_handler, NULL);                                         /*#####@hqA*/

#endif /* defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR) */

static void bt_le_set_chan_map_rpc_handler(CborValue *_value, void *_handler_data)/*####%Blh4*/
{                                                                                 /*#####@46c*/

	uint8_t * chan_map;                                                       /*######%AT*/
	int _result;                                                              /*######ICz*/
	struct ser_scratchpad _scratchpad;                                        /*######@8E*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                               /*##EZKHjKY*/

	chan_map = ser_decode_buffer_sp(&_scratchpad);                            /*##CqcDErc*/

	if (!ser_decoding_done_and_check(_value)) {                               /*######%FE*/
		goto decoding_error;                                              /*######QTM*/
	}                                                                         /*######@1Y*/

	_result = bt_le_set_chan_map(chan_map);                                   /*##DmtQNns*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                /*##BPC96+4*/

	return;                                                                   /*######%FR*/
decoding_error:                                                                   /*#######wY*/
	report_decoding_error(BT_LE_SET_CHAN_MAP_RPC_CMD, _handler_data);         /*#######zm*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                        /*#######@0*/

}                                                                                 /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_set_chan_map, BT_LE_SET_CHAN_MAP_RPC_CMD,/*####%BtLw*/
	bt_le_set_chan_map_rpc_handler, NULL);                                      /*#####@+Ow*/


static void bt_le_oob_get_local_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bs1C*/
{                                                                                  /*#####@hM0*/

	struct nrf_rpc_cbor_ctx _ctx;                                              /*#######%A*/
	int _result;                                                               /*#######Tg*/
	uint8_t id;                                                                /*#######k5*/
	struct bt_le_oob _oob_data;                                                /*########8*/
	struct bt_le_oob * oob = &_oob_data;                                       /*########U*/
	size_t _buffer_size_max = 5;                                               /*########@*/

	id = ser_decode_uint(_value);                                              /*##Cgs+Vr8*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_le_oob_get_local(id, oob);                                    /*##DqRhAGQ*/

	_buffer_size_max += bt_le_oob_buf_size(oob);                               /*##CDZJbOk*/

	{                                                                          /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                        /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                            /*####%DCYN*/
		bt_le_oob_enc(&_ctx.encoder, oob);                                 /*#####@uAQ*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                    /*####%BIlG*/
	}                                                                          /*#####@TnU*/

	return;                                                                    /*######%FZ*/
decoding_error:                                                                    /*######34P*/
	report_decoding_error(BT_LE_OOB_GET_LOCAL_RPC_CMD, _handler_data);         /*######@q0*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_get_local, BT_LE_OOB_GET_LOCAL_RPC_CMD,/*####%Bhtb*/
	bt_le_oob_get_local_rpc_handler, NULL);                                       /*#####@RcU*/

#if defined(CONFIG_BT_CONN) || defined(__GENERATOR)

static void bt_unpair_rpc_handler(CborValue *_value, void *_handler_data)        /*####%BrQf*/
{                                                                                /*#####@3HE*/

	uint8_t id;                                                              /*######%Ab*/
	bt_addr_le_t _addr_data;                                                 /*#######3O*/
	const bt_addr_le_t * addr;                                               /*#######zO*/
	int _result;                                                             /*#######@s*/

	id = ser_decode_uint(_value);                                            /*####%ClQn*/
	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));     /*#####@yEs*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_unpair(id, addr);                                           /*##DueDslQ*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%Fa*/
decoding_error:                                                                  /*######A/H*/
	report_decoding_error(BT_UNPAIR_RPC_CMD, _handler_data);                 /*######@V4*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_unpair, BT_UNPAIR_RPC_CMD,               /*####%Bk5s*/
	bt_unpair_rpc_handler, NULL);                                            /*#####@0hI*/

#endif /* defined(CONFIG_BT_CONN) || defined(__GENERATOR) */

#if (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP)) || defined(__GENERATOR)

size_t bt_bond_info_buf_size(const struct bt_bond_info *_data)                   /*####%BgGS*/
{                                                                                /*#####@gCc*/

	size_t _buffer_size_max = 3;                                             /*##AZjzmP0*/

	_buffer_size_max += sizeof(bt_addr_le_t);                                /*##CN3eLBo*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

void bt_bond_info_enc(CborEncoder *_encoder, const struct bt_bond_info *_data)   /*####%BrQX*/
{                                                                                /*#####@Utc*/

	SERIALIZE(STRUCT(struct bt_bond_info));

	ser_encode_buffer(_encoder, &_data->addr, sizeof(bt_addr_le_t));         /*##A22TTTE*/

}                                                                                /*##B9ELNqo*/

static inline void bt_foreach_bond_cb_callback(const struct bt_bond_info *info,
					       void *user_data,
					       uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_foreach_bond_cb));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%AZLc*/
	size_t _buffer_size_max = 8;                                             /*#####@cfw*/

	_buffer_size_max += bt_bond_info_buf_size(info);                         /*##CCCMO8s*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	bt_bond_info_enc(&_ctx.encoder, info);                                   /*######%A/*/
	ser_encode_uint(&_ctx.encoder, (uintptr_t)user_data);                    /*######A68*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                  /*######@UM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_FOREACH_BOND_CB_CALLBACK_RPC_CMD,/*####%BD9m*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@OWI*/
}

CBKPROXY_HANDLER(bt_foreach_bond_cb_encoder, bt_foreach_bond_cb_callback,
	(const struct bt_bond_info *info, void *user_data), (info, user_data));

static void bt_foreach_bond_rpc_handler(CborValue *_value, void *_handler_data)            /*####%BsUN*/
{                                                                                          /*#####@qL4*/

	uint8_t id;                                                                        /*######%Aa*/
	bt_foreach_bond_cb func;                                                           /*######qHY*/
	void * user_data;                                                                  /*######@hk*/

	id = ser_decode_uint(_value);                                                      /*######%Cs*/
	func = (bt_foreach_bond_cb)ser_decode_callback(_value, bt_foreach_bond_cb_encoder);/*######8r7*/
	user_data = (void *)ser_decode_uint(_value);                                       /*######@wo*/

	if (!ser_decoding_done_and_check(_value)) {                                        /*######%FE*/
		goto decoding_error;                                                       /*######QTM*/
	}                                                                                  /*######@1Y*/

	bt_foreach_bond(id, func, user_data);                                              /*##DtMyU5M*/

	ser_rsp_send_void();                                                               /*##BEYGLxw*/

	return;                                                                            /*######%Fa*/
decoding_error:                                                                            /*######o1R*/
	report_decoding_error(BT_FOREACH_BOND_RPC_CMD, _handler_data);                     /*######@f8*/

}                                                                                          /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_foreach_bond, BT_FOREACH_BOND_RPC_CMD,   /*####%Bgmc*/
	bt_foreach_bond_rpc_handler, NULL);                                      /*#####@t3Y*/

#endif /* (defined(CONFIG_BT_CONN) && defined(CONFIG_BT_SMP)) || defined(__GENERATOR) */
