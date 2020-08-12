
#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"


static void report_decoding_error(uint8_t cmd_evt_id, void* DATA) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}



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

	if (!ser_decoding_done_and_check(_value)) {                              /*######%AE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_enable(cb);                                                 /*##DqdsuHg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%B+*/
decoding_error:                                                                  /*#######XK*/
	report_decoding_error(BT_ENABLE_RPC_CMD, _handler_data);                 /*#######Ex*/
}                                                                                /*#######@I*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_enable, BT_ENABLE_RPC_CMD,               /*####%BmEu*/
	bt_enable_rpc_handler, NULL);                                            /*#####@rg4*/


void bt_data_dec(struct ser_scratchpad *_scratchpad, struct bt_data *_data)      /*####%Bne5*/
{                                                                                /*#####@+3g*/

	CborValue *_value = _scratchpad->value;                                  /*##AU3cSLw*/

	_data->type = ser_decode_uint(_value);                                   /*######%Ck*/
	_data->data_len = ser_decode_uint(_value);                               /*######cDJ*/
	_data->data = ser_decode_buffer_sp(_scratchpad);                         /*######@Vg*/

}                                                                                /*##B9ELNqo*/

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

	if (!ser_decoding_done_and_check(_value)) {                              /*######%AE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_le_adv_start(&param, ad, ad_len, sd, sd_len);               /*##DovAJXw*/

	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*#######%B*/
decoding_error:                                                                  /*#######3c*/
	report_decoding_error(BT_LE_ADV_START_RPC_CMD, _handler_data);           /*#######56*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######jI*/
}                                                                                /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_start, BT_LE_ADV_START_RPC_CMD,   /*####%BpKn*/
	bt_le_adv_start_rpc_handler, NULL);                                      /*#####@Slg*/

static void bt_le_adv_stop_rpc_handler(CborValue *_value, void *_handler_data)   /*####%BlBQ*/
{                                                                                /*#####@4G8*/

	int _result;                                                             /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                      /*##AGkSPWY*/

	_result = bt_le_adv_stop();                                              /*##Du9+9xY*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_stop, BT_LE_ADV_STOP_RPC_CMD,     /*####%BpS4*/
	bt_le_adv_stop_rpc_handler, NULL);                                       /*#####@7tk*/

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
	return _data->len;
}

SERIALIZE(STRUCT_BUFFER_CONST(net_buf_simple, 3));

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

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AY*/
	size_t _scratchpad_size = 0;                                             /*######rAF*/
	size_t _buffer_size_max = 18;                                            /*######@eg*/

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


static void bt_le_scan_start_rpc_handler(CborValue *_value, void *_handler_data)     /*####%BtgB*/
{                                                                                    /*#####@oaA*/

	struct bt_le_scan_param param;                                               /*######%Aa*/
	bt_le_scan_cb_t *cb;                                                         /*######eZM*/
	int _result;                                                                 /*######@I4*/

	_result = 0;

	bt_le_scan_param_dec(_value, &param);                                        /*####%CtFV*/
	cb = (bt_le_scan_cb_t *)ser_decode_callback(_value, bt_le_scan_cb_t_encoder);/*#####@F4o*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%AE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	_result = bt_le_scan_start(&param, cb);                                      /*##DiKuFZA*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

	return;                                                                      /*######%By*/
decoding_error:                                                                      /*#######Xo*/
	report_decoding_error(BT_LE_SCAN_START_RPC_CMD, _handler_data);              /*#######QA*/
}                                                                                    /*#######@8*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_start, BT_LE_SCAN_START_RPC_CMD, /*####%BnWD*/
	bt_le_scan_start_rpc_handler, NULL);                                     /*#####@Jb0*/

