
/* Client side of bluetooth API over nRF RPC.
 */

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"


SERIALIZE(GROUP(bt_rpc_grp));

SERIALIZE(RAW_STRUCT(bt_addr_le_t));


static void report_decoding_error(uint8_t cmd_evt_id, void* DATA) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}


int bt_enable(bt_ready_cb_t cb)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Aa*/
	int _result;                                                             /*######Qso*/
	size_t _buffer_size_max = 5;                                             /*######@uA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_callback(&_ctx.encoder, cb);                                  /*##AxNS7A4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ENABLE_RPC_CMD,                  /*####%BKRK*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@M9g*/

	return _result;                                                          /*##BX7TDLc*/
}

static void bt_ready_cb_t_callback_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bv/b*/
{                                                                                     /*#####@hiY*/

	int err;                                                                      /*####%AcxU*/
	bt_ready_cb_t callback_slot;                                                  /*#####@1I0*/

	err = ser_decode_int(_value);                                                 /*####%CqoU*/
	callback_slot = (bt_ready_cb_t)ser_decode_callback_slot(_value);              /*#####@+7U*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%AE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	callback_slot(err);                                                           /*##DnrVhJE*/

	return;                                                                       /*######%B6*/
decoding_error:                                                                       /*#######k7*/
	report_decoding_error(BT_READY_CB_T_CALLBACK_RPC_EVT, _handler_data);         /*#######Bb*/
}                                                                                     /*#######@4*/


size_t bt_data_buf_size(const struct bt_data *_data)                             /*####%Bmy4*/
{                                                                                /*#####@ED0*/

	size_t _buffer_size_max = 0;                                             /*##AW2oACE*/

	_buffer_size_max += sizeof(uint8_t) * _data->data_len;                   /*##CBQ2Maw*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

size_t bt_data_sp_size(const struct bt_data *_data)                              /*####%Btzm*/
{                                                                                /*#####@Oy8*/

	size_t _scratchpad_size = 0;                                             /*##ATz5YrA*/

	_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * _data->data_len); /*##EGeYfNs*/

	return _scratchpad_size;                                                 /*##BRWAmyU*/

}                                                                                /*##B9ELNqo*/

void bt_data_enc(CborEncoder *_encoder, const struct bt_data *_data)                /*####%BumI*/
{                                                                                   /*#####@wlE*/

	SERIALIZE(STRUCT(struct bt_data));
	SERIALIZE(SIZE_PARAM(data, data_len));

	ser_encode_uint(_encoder, _data->type);                                     /*######%A8*/
	ser_encode_uint(_encoder, _data->data_len);                                 /*######fsW*/
	ser_encode_buffer(_encoder, _data->data, sizeof(uint8_t) * _data->data_len);/*######@EY*/

}                                                                                   /*##B9ELNqo*/


size_t bt_le_adv_param_sp_size(const struct bt_le_adv_param *_data)                   /*####%Bjyc*/
{                                                                                     /*#####@ibE*/

	size_t _scratchpad_size = 0;                                                  /*##ATz5YrA*/

	_scratchpad_size += !_data->peer ? 0 : SCRATCHPAD_ALIGN(sizeof(bt_addr_le_t));/*##EL1Agek*/

	return _scratchpad_size;                                                      /*##BRWAmyU*/

}                                                                                     /*##B9ELNqo*/

size_t bt_le_adv_param_buf_size(const struct bt_le_adv_param *_data)             /*####%BiYp*/
{                                                                                /*#####@Q8I*/

	size_t _buffer_size_max = 0;                                             /*##AW2oACE*/

	_buffer_size_max += !_data->peer ? 0 : 2 + sizeof(bt_addr_le_t);         /*##CASBfgs*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

void bt_le_adv_param_enc(CborEncoder *_encoder, const struct bt_le_adv_param *_data)/*####%BmWY*/
{                                                                                   /*#####@Uhs*/

	SERIALIZE(STRUCT(struct bt_le_adv_param));
	SERIALIZE(NULLABLE(peer));

	ser_encode_uint(_encoder, _data->id);                                       /*#######%A*/
	ser_encode_uint(_encoder, _data->sid);                                      /*#######9y*/
	ser_encode_uint(_encoder, _data->secondary_max_skip);                       /*########+*/
	ser_encode_uint(_encoder, _data->options);                                  /*########D*/
	ser_encode_uint(_encoder, _data->interval_min);                             /*########E*/
	ser_encode_uint(_encoder, _data->interval_max);                             /*########E*/
	ser_encode_buffer(_encoder, _data->peer, sizeof(bt_addr_le_t));             /*########@*/

}                                                                                   /*##B9ELNqo*/

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	SERIALIZE(SIZE_PARAM(ad, ad_len));
	SERIALIZE(SIZE_PARAM(sd, sd_len));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	int _result;                                                             /*#######Vr*/
	size_t _scratchpad_size = 0;                                             /*#######45*/
	size_t _buffer_size_max = 37;                                            /*#######io*/
	size_t _i;                                                               /*########@*/

	_buffer_size_max += bt_le_adv_param_buf_size(param);                     /*########%*/
	for (_i = 0; _i < ad_len; _i++) {                                        /*########C*/
		_buffer_size_max += bt_data_buf_size(&ad[_i]);                   /*########N*/
		_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));    /*########1*/
		_scratchpad_size += bt_data_sp_size(&ad[_i]);                    /*########8*/
	}                                                                        /*########F*/
	for (_i = 0; _i < sd_len; _i++) {                                        /*########7*/
		_buffer_size_max += bt_data_buf_size(&sd[_i]);                   /*########o*/
		_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));    /*#########*/
		_scratchpad_size += bt_data_sp_size(&sd[_i]);                    /*#########*/
	}                                                                        /*########@*/

	_scratchpad_size += bt_le_adv_param_sp_size(param);                      /*##EEZ/Gv4*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	bt_le_adv_param_enc(&_ctx.encoder, param);                               /*########%*/
	ser_encode_uint(&_ctx.encoder, ad_len);                                  /*########A*/
	for (_i = 0; _i < ad_len; _i++) {                                        /*########+*/
		bt_data_enc(&_ctx.encoder, &ad[_i]);                             /*########o*/
	}                                                                        /*########a*/
	ser_encode_uint(&_ctx.encoder, sd_len);                                  /*########E*/
	for (_i = 0; _i < sd_len; _i++) {                                        /*########B*/
		bt_data_enc(&_ctx.encoder, &sd[_i]);                             /*########g*/
	}                                                                        /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_START_RPC_CMD,            /*####%BNew*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@Uc8*/

	return _result;                                                          /*##BX7TDLc*/
}

SERIALIZE(STRUCT_BUFFER_CONST(bt_data, 9));                                      /*##BpOvlm4*/

SERIALIZE(STRUCT_BUFFER_CONST(bt_le_adv_param, 22));                             /*##BpIi8ZQ*/


int bt_le_adv_stop(void)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AX*/
	int _result;                                                             /*######56+*/
	size_t _buffer_size_max = 0;                                             /*######@io*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_STOP_RPC_CMD,             /*####%BIpx*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@3mk*/

	return _result;                                                          /*##BX7TDLc*/
}

void bt_le_scan_param_enc(CborEncoder *_encoder, const struct bt_le_scan_param *_data)/*####%BgZ3*/
{                                                                                     /*#####@RoA*/

	SERIALIZE(STRUCT(struct bt_le_scan_param));

	ser_encode_uint(_encoder, _data->type);                                       /*#######%A*/
	ser_encode_uint(_encoder, _data->options);                                    /*#######4W*/
	ser_encode_uint(_encoder, _data->interval);                                   /*########D*/
	ser_encode_uint(_encoder, _data->window);                                     /*########I*/
	ser_encode_uint(_encoder, _data->timeout);                                    /*########n*/
	ser_encode_uint(_encoder, _data->interval_coded);                             /*########Q*/
	ser_encode_uint(_encoder, _data->window_coded);                               /*########@*/

}                                                                                     /*##B9ELNqo*/

int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	int _result;                                                             /*######PI0*/
	size_t _buffer_size_max = 27;                                            /*######@Mk*/

	_result = 0;

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	bt_le_scan_param_enc(&_ctx.encoder, param);                              /*####%A+tp*/
	ser_encode_callback(&_ctx.encoder, cb);                                  /*#####@nI8*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SCAN_START_RPC_CMD,           /*####%BEk6*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@M28*/

	return _result;                                                          /*##BX7TDLc*/
}

SERIALIZE(STRUCT_BUFFER_CONST(bt_le_scan_param, 22));                            /*##BvJvxzY*/


void net_buf_simple_dec(struct ser_scratchpad *_scratchpad, struct net_buf_simple *_data)
{
	CborValue *_value = _scratchpad->value;

	_data->len = ser_decode_buffer_size(_value);
	_data->data = ser_decode_buffer_sp(_scratchpad);
	_data->size = _data->len;
	_data->__buf = _data->data;
}


NRF_RPC_CBOR_EVT_DECODER(bt_rpc_grp, bt_ready_cb_t_callback, BT_READY_CB_T_CALLBACK_RPC_EVT,/*####%BqdK*/
	bt_ready_cb_t_callback_rpc_handler, NULL);                                          /*#####@nrU*/

static void bt_le_scan_cb_t_callback_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bv5m*/
{                                                                                       /*#####@99E*/

	bt_addr_le_t _addr_data;                                                        /*#######%A*/
	const bt_addr_le_t * addr;                                                      /*#######fR*/
	int8_t rssi;                                                                    /*########0*/
	uint8_t adv_type;                                                               /*########/*/
	struct net_buf_simple buf;                                                      /*########V*/
	bt_le_scan_cb_t * callback_slot;                                                /*########E*/
	struct ser_scratchpad _scratchpad;                                              /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                     /*##EZKHjKY*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));            /*#######%C*/
	rssi = ser_decode_int(_value);                                                  /*#######iC*/
	adv_type = ser_decode_uint(_value);                                             /*#######Jv*/
	net_buf_simple_dec(&_scratchpad, &buf);                                         /*#######5M*/
	callback_slot = (bt_le_scan_cb_t *)ser_decode_callback_slot(_value);            /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                     /*######%AE*/
		goto decoding_error;                                                    /*######QTM*/
	}                                                                               /*######@1Y*/

	callback_slot(addr, rssi, adv_type, &buf);                                      /*##DgC7dGo*/

	SER_SCRATCHPAD_FREE(&_scratchpad);                                              /*##Eq1r7Tg*/

	ser_rsp_send_void();                                                            /*##BEYGLxw*/

	return;                                                                         /*#######%B*/
decoding_error:                                                                         /*#######wP*/
	report_decoding_error(BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD, _handler_data);         /*#######ev*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                              /*#######9M*/
}                                                                                       /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_t_callback, BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,/*####%Bvg5*/
	bt_le_scan_cb_t_callback_rpc_handler, NULL);                                            /*#####@vlo*/

