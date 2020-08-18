
/* Client side of bluetooth API over nRF RPC.
 */

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"


SERIALIZE(GROUP(bt_rpc_grp));

SERIALIZE(RAW_STRUCT(bt_addr_le_t));
SERIALIZE(OPAQUE_STRUCT(struct ));
SERIALIZE(FILTERED_STRUCT(struct bt_le_ext_adv, 3, encode_bt_le_ext_adv, decode_bt_le_ext_adv));


#ifndef NRF_RPC_GENERATOR
#define UNUSED __attribute__((unused))
#else
#define UNUSED ;
#endif

struct bt_le_ext_adv {
	struct bt_le_ext_adv_cb *cb;
};

static struct bt_le_ext_adv adv_pool[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static struct bt_le_ext_adv *decode_bt_le_ext_adv(CborValue *value)
{
	uint16_t index;

	index = ser_decode_uint(value);

	if (index >= ARRAY_SIZE(adv_pool)) {
		if (index > ARRAY_SIZE(adv_pool)) {
			ser_decoder_invalid(value, CborErrorDataTooLarge);
		}
		return NULL;
	} else {
		return &adv_pool[index];
	}
}

static void encode_bt_le_ext_adv(CborEncoder *encoder, struct bt_le_ext_adv *value)
{
	uint16_t index;

	if (value == NULL) {
		index = ARRAY_SIZE(adv_pool);
	} else {
		index = value - adv_pool;
		__ASSERT(index < ARRAY_SIZE(adv_pool), "Invalid bt_le_ext_adv pointer");
	}

	ser_encode_uint(encoder, index);
}


static void report_decoding_error(uint8_t cmd_evt_id, void* data) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}


struct bt_rpc_get_check_table_rpc_res                                            /*####%Bm7o*/
{                                                                                /*#####@Dt0*/

	size_t size;                                                             /*####%CQ4t*/
	uint8_t * data;                                                          /*#####@r1M*/

};                                                                               /*##B985gv0*/

static void bt_rpc_get_check_table_rpc_rsp(CborValue *_value, void *_handler_data)/*####%Bn5Q*/
{                                                                                 /*#####@VUY*/

	struct bt_rpc_get_check_table_rpc_res *_res =                             /*####%AXIK*/
		(struct bt_rpc_get_check_table_rpc_res *)_handler_data;           /*#####@Gnk*/

	ser_decode_buffer(_value, _res->data, sizeof(uint8_t) * (_res->size));    /*##DcVdAPg*/

}                                                                                 /*##B9ELNqo*/

static void bt_rpc_get_check_table(uint8_t *data, size_t size)
{
	SERIALIZE(SIZE_PARAM(data, size));
	SERIALIZE(OUT(data));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ab*/
	struct bt_rpc_get_check_table_rpc_res _result;                           /*#######em*/
	size_t _scratchpad_size = 0;                                             /*#######+J*/
	size_t _buffer_size_max = 10;                                            /*#######@w*/

	_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * size);            /*##EIzRmJk*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, size);                                    /*##A2nPHkE*/

	_result.size = size;                                                     /*####%CxN2*/
	_result.data = data;                                                     /*#####@rIE*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_GET_CHECK_TABLE_RPC_CMD,     /*####%BCAl*/
		&_ctx, bt_rpc_get_check_table_rpc_rsp, &_result);                /*#####@1FM*/
}

static void validate_config()
{
	size_t size = bt_rpc_calc_check_table_size();
	uint8_t data[size];
	static bool validated = false;

	if (!validated) {
		validated = true;
		bt_rpc_get_check_table(data, size);
		if (!bt_rpc_validate_check_table(data, size)) {
			nrf_rpc_err(-EINVAL, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp,
				    BT_ENABLE_RPC_CMD, NRF_RPC_PACKET_TYPE_CMD);
		}
	}
}


int bt_enable(bt_ready_cb_t cb)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Aa*/
	int _result;                                                             /*######Qso*/
	size_t _buffer_size_max = 5;                                             /*######@uA*/

	validate_config();

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_callback(&_ctx.encoder, cb);                                  /*##AxNS7A4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ENABLE_RPC_CMD,                  /*####%BKRK*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@M9g*/

	return _result;                                                          /*##BX7TDLc*/
}


int bt_set_name(const char *name)
{
	SERIALIZE(STR(name));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	size_t _name_strlen;                                                     /*#######bk*/
	int _result;                                                             /*#######HH*/
	size_t _scratchpad_size = 0;                                             /*#######eY*/
	size_t _buffer_size_max = 10;                                            /*########@*/

	_name_strlen = strlen(name);                                             /*####%CFOk*/
	_buffer_size_max += _name_strlen;                                        /*#####@f8c*/

	_scratchpad_size += SCRATCHPAD_ALIGN(_name_strlen + 1);                  /*##EObZcVc*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_str(&_ctx.encoder, name, _name_strlen);                       /*##A/RUZRo*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_NAME_RPC_CMD,                /*####%BKgG*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@0j4*/

	return _result;                                                          /*##BX7TDLc*/
}

struct bt_get_name_out_rpc_res                                                   /*####%Bp8J*/
{                                                                                /*#####@zYc*/

	bool _result;                                                            /*######%CW*/
	size_t size;                                                             /*######pln*/
	char * name;                                                             /*######@DY*/

};                                                                               /*##B985gv0*/

static void bt_get_name_out_rpc_rsp(CborValue *_value, void *_handler_data)      /*####%BoL7*/
{                                                                                /*#####@fvE*/

	struct bt_get_name_out_rpc_res *_res =                                   /*####%AbU8*/
		(struct bt_get_name_out_rpc_res *)_handler_data;                 /*#####@c5k*/

	_res->_result = ser_decode_bool(_value);                                 /*####%DYid*/
	ser_decode_str(_value, _res->name, (_res->size));                        /*#####@AWM*/

}                                                                                /*##B9ELNqo*/

static bool bt_get_name_out(char *name, size_t size)
{
	SERIALIZE(OUT(name));
	SERIALIZE(STR(name));
	SERIALIZE(SIZE_PARAM(name, size));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AY*/
	struct bt_get_name_out_rpc_res _result;                                  /*#######X2*/
	size_t _scratchpad_size = 0;                                             /*#######Jd*/
	size_t _buffer_size_max = 10;                                            /*#######@I*/

	_scratchpad_size += SCRATCHPAD_ALIGN(size);                              /*##EB+3ycA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, size);                                    /*##A2nPHkE*/

	_result.size = size;                                                     /*####%C5M6*/
	_result.name = name;                                                     /*#####@4K4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_GET_NAME_OUT_RPC_CMD,            /*####%BG0o*/
		&_ctx, bt_get_name_out_rpc_rsp, &_result);                       /*#####@sSg*/

	return _result._result;                                                  /*##BW0ge3U*/
}

const char *bt_get_name(void)
{
	static char bt_name_cache[CONFIG_BT_DEVICE_NAME_MAX + 1];
	bool not_null;

	not_null = bt_get_name_out(bt_name_cache, sizeof(bt_name_cache));
	return not_null ? bt_name_cache : NULL;
}

int bt_set_id_addr(const bt_addr_le_t *addr)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	int _result;                                                             /*######RDP*/
	size_t _buffer_size_max = 3;                                             /*######@sI*/

	validate_config();

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*##CHCgvU0*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));            /*##A9rQrRg*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_ID_ADDR_RPC_CMD,             /*####%BG6w*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@qi0*/

	return _result;                                                          /*##BX7TDLc*/
}

struct bt_id_get_rpc_res                                                         /*####%Bmxc*/
{                                                                                /*#####@ZAo*/

	size_t * count;                                                          /*####%CcVF*/
	bt_addr_le_t * addrs;                                                    /*#####@Nco*/

};                                                                               /*##B985gv0*/

static void bt_id_get_rpc_rsp(CborValue *_value, void *_handler_data)                 /*####%BluI*/
{                                                                                     /*#####@1Q4*/

	struct bt_id_get_rpc_res *_res =                                              /*####%AYtj*/
		(struct bt_id_get_rpc_res *)_handler_data;                            /*#####@fr0*/

	*(_res->count) = ser_decode_uint(_value);                                     /*####%DZiC*/
	ser_decode_buffer(_value, _res->addrs, *(_res->count) * sizeof(bt_addr_le_t));/*#####@Ip8*/

}                                                                                     /*##B9ELNqo*/

void bt_id_get(bt_addr_le_t *addrs, size_t *count)
{
	SERIALIZE(OUT(addrs));
	SERIALIZE(SIZE_PARAM_EX(addrs, *$, count));
	SERIALIZE(INOUT(count));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AU*/
	struct bt_id_get_rpc_res _result;                                        /*#######FC*/
	size_t _scratchpad_size = 0;                                             /*#######Cq*/
	size_t _buffer_size_max = 10;                                            /*#######@o*/

	_scratchpad_size += SCRATCHPAD_ALIGN(*count * sizeof(bt_addr_le_t));     /*##EM0RMfw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, *count);                                  /*##A0IY0+8*/

	_result.count = count;                                                   /*####%C9M/*/
	_result.addrs = addrs;                                                   /*#####@zo8*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_GET_RPC_CMD,                  /*####%BLnI*/
		&_ctx, bt_id_get_rpc_rsp, &_result);                             /*#####@SsE*/
}

struct bt_id_create_rpc_res                                                      /*####%BoIG*/
{                                                                                /*#####@PlM*/

	int _result;                                                             /*######%CT*/
	bt_addr_le_t * addr;                                                     /*######Jyh*/
	uint8_t * irk;                                                           /*######@+s*/

};                                                                               /*##B985gv0*/

static void bt_id_create_rpc_rsp(CborValue *_value, void *_handler_data)         /*####%BqRo*/
{                                                                                /*#####@GSg*/

	struct bt_id_create_rpc_res *_res =                                      /*####%AWr1*/
		(struct bt_id_create_rpc_res *)_handler_data;                    /*#####@UyU*/

	_res->_result = ser_decode_int(_value);                                  /*######%DQ*/
	ser_decode_buffer(_value, _res->addr, sizeof(bt_addr_le_t));             /*######lk5*/
	ser_decode_buffer(_value, _res->irk, sizeof(uint8_t) * 16);              /*######@5o*/

}                                                                                /*##B9ELNqo*/

int bt_id_create(bt_addr_le_t *addr, uint8_t *irk)
{
	SERIALIZE(NULLABLE(addr));
	SERIALIZE(INOUT(addr));
	SERIALIZE(NULLABLE(irk));
	SERIALIZE(INOUT(irk));
	SERIALIZE(SIZE(irk, 16));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	size_t _irk_size;                                                        /*#######Xr*/
	struct bt_id_create_rpc_res _result;                                     /*#######Lm*/
	size_t _scratchpad_size = 0;                                             /*#######Ak*/
	size_t _buffer_size_max = 13;                                            /*########@*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*######%CB*/
	_irk_size = !irk ? 0 : sizeof(uint8_t) * 16;                             /*######cKg*/
	_buffer_size_max += _irk_size;                                           /*######@u4*/

	_scratchpad_size += SCRATCHPAD_ALIGN(_irk_size);                         /*##ELqQtKI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));            /*####%Ax4U*/
	ser_encode_buffer(&_ctx.encoder, irk, _irk_size);                        /*#####@/YU*/

	_result.addr = addr;                                                     /*####%CxTc*/
	_result.irk = irk;                                                       /*#####@QsM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_CREATE_RPC_CMD,               /*####%BMH+*/
		&_ctx, bt_id_create_rpc_rsp, &_result);                          /*#####@log*/

	return _result._result;                                                  /*##BW0ge3U*/
}

struct bt_id_reset_rpc_res                                                       /*####%Bs1i*/
{                                                                                /*#####@i/M*/

	int _result;                                                             /*######%CT*/
	bt_addr_le_t * addr;                                                     /*######Jyh*/
	uint8_t * irk;                                                           /*######@+s*/

};                                                                               /*##B985gv0*/

static void bt_id_reset_rpc_rsp(CborValue *_value, void *_handler_data)          /*####%BoSY*/
{                                                                                /*#####@/xk*/

	struct bt_id_reset_rpc_res *_res =                                       /*####%AZXs*/
		(struct bt_id_reset_rpc_res *)_handler_data;                     /*#####@oUo*/

	_res->_result = ser_decode_int(_value);                                  /*######%DQ*/
	ser_decode_buffer(_value, _res->addr, sizeof(bt_addr_le_t));             /*######lk5*/
	ser_decode_buffer(_value, _res->irk, sizeof(uint8_t) * 16);              /*######@5o*/

}                                                                                /*##B9ELNqo*/

int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk)
{
	SERIALIZE(INOUT(addr));
	SERIALIZE(NULLABLE(addr));
	SERIALIZE(INOUT(irk));
	SERIALIZE(NULLABLE(irk));
	SERIALIZE(SIZE(irk, 16));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	size_t _irk_size;                                                        /*#######XB*/
	struct bt_id_reset_rpc_res _result;                                      /*#######PU*/
	size_t _scratchpad_size = 0;                                             /*#######NI*/
	size_t _buffer_size_max = 15;                                            /*########@*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*######%CB*/
	_irk_size = !irk ? 0 : sizeof(uint8_t) * 16;                             /*######cKg*/
	_buffer_size_max += _irk_size;                                           /*######@u4*/

	_scratchpad_size += SCRATCHPAD_ALIGN(_irk_size);                         /*##ELqQtKI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, id);                                      /*######%A9*/
	ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));            /*######0qd*/
	ser_encode_buffer(&_ctx.encoder, irk, _irk_size);                        /*######@RU*/

	_result.addr = addr;                                                     /*####%CxTc*/
	_result.irk = irk;                                                       /*#####@QsM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_RESET_RPC_CMD,                /*####%BDbH*/
		&_ctx, bt_id_reset_rpc_rsp, &_result);                           /*#####@DeY*/

	return _result._result;                                                  /*##BW0ge3U*/
}

int bt_id_delete(uint8_t id)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AW*/
	int _result;                                                             /*######sOU*/
	size_t _buffer_size_max = 2;                                             /*######@TQ*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, id);                                      /*##A9BnOB4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ID_DELETE_RPC_CMD,               /*####%BNTu*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@K8I*/

	return _result;                                                          /*##BX7TDLc*/
}

static void bt_ready_cb_t_callback_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bv/b*/
{                                                                                     /*#####@hiY*/

	int err;                                                                      /*####%AcxU*/
	bt_ready_cb_t callback_slot;                                                  /*#####@1I0*/

	err = ser_decode_int(_value);                                                 /*####%CqoU*/
	callback_slot = (bt_ready_cb_t)ser_decode_callback_slot(_value);              /*#####@+7U*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	callback_slot(err);                                                           /*##DnrVhJE*/

	return;                                                                       /*######%B6*/
decoding_error:                                                                       /*#######k7*/
	report_decoding_error(BT_READY_CB_T_CALLBACK_RPC_EVT, _handler_data);         /*#######Bb*/
}                                                                                     /*#######@4*/


size_t bt_data_buf_size(const struct bt_data *_data)                             /*####%Bmy4*/
{                                                                                /*#####@ED0*/

	size_t _buffer_size_max = 9;                                             /*##Ad+vLz8*/

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

	size_t _buffer_size_max = 22;                                            /*##AavCUAw*/

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
	int _result;                                                             /*#######dr*/
	size_t _scratchpad_size = 0;                                             /*#######k2*/
	size_t _buffer_size_max = 15;                                            /*#######pA*/
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


int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len)
{
	SERIALIZE(SIZE_PARAM(ad, ad_len));
	SERIALIZE(SIZE_PARAM(sd, sd_len));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	int _result;                                                             /*#######dr*/
	size_t _scratchpad_size = 0;                                             /*#######k2*/
	size_t _buffer_size_max = 15;                                            /*#######pA*/
	size_t _i;                                                               /*########@*/

	for (_i = 0; _i < ad_len; _i++) {                                        /*########%*/
		_buffer_size_max += bt_data_buf_size(&ad[_i]);                   /*########C*/
		_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));    /*########J*/
		_scratchpad_size += bt_data_sp_size(&ad[_i]);                    /*########/*/
	}                                                                        /*########m*/
	for (_i = 0; _i < sd_len; _i++) {                                        /*########7*/
		_buffer_size_max += bt_data_buf_size(&sd[_i]);                   /*########f*/
		_scratchpad_size += SCRATCHPAD_ALIGN(sizeof(struct bt_data));    /*########M*/
		_scratchpad_size += bt_data_sp_size(&sd[_i]);                    /*#########*/
	}                                                                        /*########@*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	ser_encode_uint(&_ctx.encoder, ad_len);                                  /*#######%A*/
	for (_i = 0; _i < ad_len; _i++) {                                        /*########/*/
		bt_data_enc(&_ctx.encoder, &ad[_i]);                             /*########P*/
	}                                                                        /*########G*/
	ser_encode_uint(&_ctx.encoder, sd_len);                                  /*########S*/
	for (_i = 0; _i < sd_len; _i++) {                                        /*########r*/
		bt_data_enc(&_ctx.encoder, &sd[_i]);                             /*########c*/
	}                                                                        /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_ADV_UPDATE_DATA_RPC_CMD,      /*####%BHqc*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@6aY*/

	return _result;                                                          /*##BX7TDLc*/
}

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

SERIALIZE(FIELD_TYPE(struct bt_le_ext_adv_cb, bt_le_ext_adv_cb_sent, sent));
SERIALIZE(FIELD_TYPE(struct bt_le_ext_adv_cb, bt_le_ext_adv_cb_connected, connected));
SERIALIZE(FIELD_TYPE(struct bt_le_ext_adv_cb, bt_le_ext_adv_cb_scanned, scanned));

static const size_t bt_le_ext_adv_cb_buf_size = 15;                              /*##BpmBBn8*/

void bt_le_ext_adv_cb_enc(CborEncoder *_encoder, const struct bt_le_ext_adv_cb *_data)/*####%BmsD*/
{                                                                                     /*#####@xU4*/

	SERIALIZE(STRUCT(struct bt_le_ext_adv_cb));

	ser_encode_callback(_encoder, _data->sent);                                   /*######%Aw*/
	ser_encode_callback(_encoder, _data->connected);                              /*######vXb*/
	ser_encode_callback(_encoder, _data->scanned);                                /*######@Bg*/

}                                                                                     /*##B9ELNqo*/

struct bt_le_ext_adv_create_rpc_res                                              /*####%Bm6e*/
{                                                                                /*#####@dOE*/

	int _result;                                                             /*####%CRV7*/
	struct bt_le_ext_adv ** adv;                                             /*#####@36w*/

};                                                                               /*##B985gv0*/

static void bt_le_ext_adv_create_rpc_rsp(CborValue *_value, void *_handler_data) /*####%Bno+*/
{                                                                                /*#####@e2c*/

	struct bt_le_ext_adv_create_rpc_res *_res =                              /*####%AQ0J*/
		(struct bt_le_ext_adv_create_rpc_res *)_handler_data;            /*#####@yio*/

	_res->_result = ser_decode_int(_value);                                  /*####%DbW3*/
	*(_res->adv) = decode_bt_le_ext_adv(_value);                             /*#####@Q4I*/

}                                                                                /*##B9ELNqo*/

int bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **adv)
{
	SERIALIZE(OUT(adv));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AX*/
	struct bt_le_ext_adv_create_rpc_res _result;                             /*#######Iz*/
	size_t _scratchpad_size = 0;                                             /*#######Fm*/
	size_t _buffer_size_max = 20;                                            /*#######@c*/

	_buffer_size_max += bt_le_adv_param_buf_size(param);                     /*##CKvr4W4*/

	_scratchpad_size += bt_le_adv_param_sp_size(param);                      /*##EEZ/Gv4*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	bt_le_adv_param_enc(&_ctx.encoder, param);                               /*####%A7sT*/
	bt_le_ext_adv_cb_enc(&_ctx.encoder, cb);                                 /*#####@fDU*/

	_result.adv = adv;                                                       /*##Cx5Tf1c*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_EXT_ADV_CREATE_RPC_CMD,       /*####%BOJo*/
		&_ctx, bt_le_ext_adv_create_rpc_rsp, &_result);                  /*#####@vEw*/

	return _result._result;                                                  /*##BW0ge3U*/
}

UNUSED
static const size_t bt_le_scan_param_buf_size = 22;                              /*##BvqYHlg*/

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

	if (!ser_decoding_done_and_check(_value)) {                                     /*######%FE*/
		goto decoding_error;                                                    /*######QTM*/
	}                                                                               /*######@1Y*/

	callback_slot(addr, rssi, adv_type, &buf);                                      /*##DgC7dGo*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                      /*##Eq1r7Tg*/

	ser_rsp_send_void();                                                            /*##BEYGLxw*/

	return;                                                                         /*#######%B*/
decoding_error:                                                                         /*#######wP*/
	report_decoding_error(BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD, _handler_data);         /*#######ev*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                              /*#######9M*/
}                                                                                       /*########@*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_scan_cb_t_callback, BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,/*####%Bvg5*/
	bt_le_scan_cb_t_callback_rpc_handler, NULL);                                            /*#####@vlo*/
