
#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"



static void report_decoding_error(uint8_t cmd_evt_id, void* data) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

struct bt_le_ext_adv_cache {
	bt_le_ext_adv_cb cb;
};

struct bt_le_ext_adv_cache adv_cache[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static uint16_t adv_pool_base_index = 0;
static uint16_t adv_pool_item_size = 0;
static uint8_t *adv_pool_base_ptr = NULL;

static void calculate_adv_pool_info(uint16_t index, struct bt_le_ext_adv *value)
{
	/* TODO: Create an issue in Zephyr to add bt_le_ext_adv_lookup_index()
	 * and remove this calculations. */
	if (adv_pool_item_size != 0) {
		return;
	} else if (adv_pool_base_ptr == NULL) {
		adv_pool_base_ptr = (uint8_t *)value;
		adv_pool_base_index = index;
	} else if (adv_pool_base_index != index) {
		int ptr_diff = adv_pool_base_ptr - (uint8_t *)value;
		int index_diff = adv_pool_base_index - index;

		adv_pool_item_size = ptr_diff / index_diff;
		adv_pool_base_ptr = ((uint8_t *)value -
				     adv_pool_item_size * index);
	}
}

static struct bt_le_ext_adv *decode_bt_le_ext_adv(CborValue *value)
{
	uint16_t index;

	index = ser_decode_uint(value);

	if (index >= CONFIG_BT_EXT_ADV_MAX_ADV_SET) {
		if (index > CONFIG_BT_EXT_ADV_MAX_ADV_SET) {
			ser_decoder_invalid(value, CborErrorDataTooLarge);
		}
		return NULL;
	} else if (adv_pool_item_size == 0) {
		return (struct bt_le_ext_adv *)adv_pool_base_ptr;
	} else {
		return (struct bt_le_ext_adv *)(adv_pool_base_ptr + index * adv_pool_item_size);
	}
}

static void encode_bt_le_ext_adv(CborEncoder *encoder, struct bt_le_ext_adv *value)
{
	uint16_t index;

	if (value == NULL) {
		index = CONFIG_BT_EXT_ADV_MAX_ADV_SET;
	} else {
		index = bt_le_ext_adv_get_index(value);
		calculate_adv_pool_info(index, value);
	}

	ser_encode_uint(encoder, index);
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

	return;                                                                       /*#######%B*/
decoding_error:                                                                       /*#######6u*/
	report_decoding_error(BT_RPC_GET_CHECK_TABLE_RPC_CMD, _handler_data);         /*#######0o*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                            /*#######OU*/
}                                                                                     /*########@*/

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

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_le_adv_start(&param, ad, ad_len, sd, sd_len);               /*##DovAJXw*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                               /*##Eq1r7Tg*/

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

	nrf_rpc_cbor_decoding_done(_value);                                      /*##FGkSPWY*/

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


static void bt_le_scan_start_rpc_handler(CborValue *_value, void *_handler_data)     /*####%BtgB*/
{                                                                                    /*#####@oaA*/

	struct bt_le_scan_param param;                                               /*######%Aa*/
	bt_le_scan_cb_t *cb;                                                         /*######eZM*/
	int _result;                                                                 /*######@I4*/

	_result = 0;

	bt_le_scan_param_dec(_value, &param);                                        /*####%CtFV*/
	cb = (bt_le_scan_cb_t *)ser_decode_callback(_value, bt_le_scan_cb_t_encoder);/*#####@F4o*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
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

	return;                                                                  /*#######%B*/
decoding_error:                                                                  /*#######zA*/
	report_decoding_error(BT_SET_NAME_RPC_CMD, _handler_data);               /*#######Ua*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######eM*/
}                                                                                /*########@*/

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

	return;                                                                  /*#######%B*/
decoding_error:                                                                  /*#######7b*/
	report_decoding_error(BT_GET_NAME_OUT_RPC_CMD, _handler_data);           /*#######DV*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######Lc*/
}                                                                                /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_get_name_out, BT_GET_NAME_OUT_RPC_CMD,   /*####%BuEw*/
	bt_get_name_out_rpc_handler, NULL);                                      /*#####@65I*/

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

	return;                                                                  /*######%B8*/
decoding_error:                                                                  /*#######PY*/
	report_decoding_error(BT_SET_ID_ADDR_RPC_CMD, _handler_data);            /*#######vX*/
}                                                                                /*#######@s*/

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

	return;                                                                        /*#######%B*/
decoding_error:                                                                        /*#######wM*/
	report_decoding_error(BT_ID_GET_RPC_CMD, _handler_data);                       /*#######UE*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                             /*#######YA*/
}                                                                                      /*########@*/

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

	return;                                                                  /*#######%B*/
decoding_error:                                                                  /*#######1z*/
	report_decoding_error(BT_ID_CREATE_RPC_CMD, _handler_data);              /*#######Qy*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######Ho*/
}                                                                                /*########@*/

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

	return;                                                                  /*#######%B*/
decoding_error:                                                                  /*#######6p*/
	report_decoding_error(BT_ID_RESET_RPC_CMD, _handler_data);               /*#######d/*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                       /*#######oY*/
}                                                                                /*########@*/

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

	return;                                                                  /*######%B5*/
decoding_error:                                                                  /*#######iP*/
	report_decoding_error(BT_ID_DELETE_RPC_CMD, _handler_data);              /*#######YY*/
}                                                                                /*#######@Y*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_id_delete, BT_ID_DELETE_RPC_CMD,         /*####%BibD*/
	bt_id_delete_rpc_handler, NULL);                                         /*#####@9Jg*/

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

	return;                                                                      /*#######%B*/
decoding_error:                                                                      /*#######+2*/
	report_decoding_error(BT_LE_ADV_UPDATE_DATA_RPC_CMD, _handler_data);         /*#######rK*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                           /*#######Ow*/
}                                                                                    /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_adv_update_data, BT_LE_ADV_UPDATE_DATA_RPC_CMD,/*####%BpKp*/
	bt_le_adv_update_data_rpc_handler, NULL);                                         /*#####@XTI*/

void bt_le_ext_adv_cb_dec(CborValue *_value, struct bt_le_ext_adv_cb *_data)                                           /*####%BguJ*/
{                                                                                                                      /*#####@Ks0*/

	_data->sent = (bt_le_ext_adv_cb_sent)ser_decode_callback(_value, bt_le_ext_adv_cb_sent_encoder);               /*######%Ck*/
	_data->connected = (bt_le_ext_adv_cb_connected)ser_decode_callback(_value, bt_le_ext_adv_cb_connected_encoder);/*######EVg*/
	_data->scanned = (bt_le_ext_adv_cb_scanned)ser_decode_callback(_value, bt_le_ext_adv_cb_scanned_encoder);      /*######@Xc*/

}                                                                                                                      /*##B9ELNqo*/

static void bt_le_ext_adv_create_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bhf+*/
{                                                                                   /*#####@/Lg*/

	struct nrf_rpc_cbor_ctx _ctx;                                               /*#######%A*/
	int _result;                                                                /*########a*/
	struct bt_le_adv_param param;                                               /*########C*/
	struct bt_le_ext_adv_cb cb;                                                 /*########k*/
	struct bt_le_ext_adv *_adv_data;                                            /*########o*/
	struct bt_le_ext_adv ** adv = &_adv_data;                                   /*########d*/
	size_t _buffer_size_max = 8;                                                /*########8*/
	struct ser_scratchpad _scratchpad;                                          /*########@*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                 /*##EZKHjKY*/

	bt_le_adv_param_dec(&_scratchpad, &param);                                  /*####%ClxW*/
	bt_le_ext_adv_cb_dec(_value, &cb);                                          /*#####@lnk*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	_result = bt_le_ext_adv_create(&param, &cb, adv);                           /*##DmWVOT0*/

	{                                                                           /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                         /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                             /*####%DDS3*/
		encode_bt_le_ext_adv(&_ctx.encoder, *adv);                          /*#####@UwQ*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                  /*##Eq1r7Tg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                     /*####%BIlG*/
	}                                                                           /*#####@TnU*/

	return;                                                                     /*#######%B*/
decoding_error:                                                                     /*#######7i*/
	report_decoding_error(BT_LE_EXT_ADV_CREATE_RPC_CMD, _handler_data);         /*#######o6*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                          /*#######pE*/
}                                                                                   /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_ext_adv_create, BT_LE_EXT_ADV_CREATE_RPC_CMD,/*####%BmDg*/
	bt_le_ext_adv_create_rpc_handler, NULL);                                        /*#####@/Eg*/

