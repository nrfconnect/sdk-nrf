/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/conn.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

SERIALIZE(GROUP(bt_rpc_grp));
SERIALIZE(RAW_STRUCT(bt_addr_le_t));
SERIALIZE(RAW_STRUCT(struct bt_conn_oob_info));
SERIALIZE(FILTERED_STRUCT(struct bt_conn, 3, encode_bt_conn, decode_bt_conn));
SERIALIZE(ENUM(bt_security_t));
SERIALIZE(OPAQUE_STRUCT(void));

static void report_decoding_error(uint8_t cmd_evt_id, void* data) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}


#define LOCK_CONN_INFO() k_mutex_lock(&bt_rpc_conn_mutex, K_FOREVER)
#define UNLOCK_CONN_INFO() k_mutex_unlock(&bt_rpc_conn_mutex)

#ifndef __GENERATOR
#define UNUSED __attribute__((unused)) /* TODO: Improve generator to avoid this workaround */
#else
#define UNUSED ;
#endif

K_MUTEX_DEFINE(bt_rpc_conn_mutex);


struct bt_conn {
	atomic_t ref;
	uint8_t	features[8];
	bt_addr_le_t src;
	bt_addr_le_t dst;
	bt_addr_le_t local;
	bt_addr_le_t remote;
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	struct bt_conn_le_phy_info phy;
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_info data_len;
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */
#if defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	struct bt_le_oob_sc_data oobd_local;
	struct bt_le_oob_sc_data oobd_remote;
#endif /* defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */
};


static struct bt_conn connections[CONFIG_BT_MAX_CONN];


static inline uint8_t get_conn_index(const struct bt_conn *conn)
{
	return (uint8_t)(conn - connections);
}


void encode_bt_conn(CborEncoder *encoder, const struct bt_conn *conn)
{
	if (CONFIG_BT_MAX_CONN > 1) {
		ser_encode_uint(encoder, get_conn_index(conn));
	}
}

struct bt_conn *decode_bt_conn(CborValue *value)
{
	uint8_t index = 0;

	if (CONFIG_BT_MAX_CONN > 1) {
		index = ser_decode_uint(value);
		if (index >= CONFIG_BT_MAX_CONN) {
			ser_decoder_invalid(value, CborErrorIO);
			return NULL;
		}
	}
	return &connections[index];
}


static void bt_conn_remote_update_ref(struct bt_conn *conn, int8_t value)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%Aefv*/
	size_t _buffer_size_max = 5;                                             /*#####@HWE*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A1ux*/
	ser_encode_int(&_ctx.encoder, value);                                    /*#####@iyA*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,  /*####%BCco*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@+0o*/
}

static void bt_conn_ref_local(struct bt_conn *conn)
{
	if (conn) {
		atomic_inc(&conn->ref);
	}
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_inc(&conn->ref);

	if (old == 0) {
		bt_conn_remote_update_ref(conn, +1);
	}

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_dec(&conn->ref);

	if (old == 1) {
		bt_conn_remote_update_ref(conn, -1);
	}
}


static void bt_conn_foreach_cb_callback_rpc_handler(CborValue *_value, void *_handler_data)/*####%BhKM*/
{                                                                                          /*#####@7gY*/

	struct bt_conn * conn;                                                             /*######%AR*/
	void * data;                                                                       /*######02n*/
	bt_conn_foreach_cb callback_slot;                                                  /*######@bY*/

	conn = decode_bt_conn(_value);                                                     /*######%Cr*/
	data = (void *)ser_decode_uint(_value);                                            /*######m/m*/
	callback_slot = (bt_conn_foreach_cb)ser_decode_callback_slot(_value);              /*######@Ug*/

	if (!ser_decoding_done_and_check(_value)) {                                        /*######%FE*/
		goto decoding_error;                                                       /*######QTM*/
	}                                                                                  /*######@1Y*/

	callback_slot(conn, data);                                                         /*##DusYSQc*/

	ser_rsp_send_void();                                                               /*##BEYGLxw*/

	return;                                                                            /*######%Fb*/
decoding_error:                                                                            /*######tl+*/
	report_decoding_error(BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD, _handler_data);         /*######@Ao*/

}                                                                                          /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_foreach_cb_callback, BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD,/*####%Bp1z*/
	bt_conn_foreach_cb_callback_rpc_handler, NULL);                                               /*#####@XU8*/

void bt_conn_foreach(int type, void (*func)(struct bt_conn *conn, void *data),
		     void *data)
{
	SERIALIZE(TYPE(func, bt_conn_foreach_cb));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%AbDw*/
	size_t _buffer_size_max = 15;                                            /*#####@wIo*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_int(&_ctx.encoder, type);                                     /*######%Ay*/
	ser_encode_callback(&_ctx.encoder, func);                                /*######YaG*/
	ser_encode_uint(&_ctx.encoder, (uintptr_t)data);                         /*######@RM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_FOREACH_RPC_CMD,            /*####%BB8N*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@xH4*/
}


struct bt_conn_lookup_addr_le_rpc_res                                            /*####%BvIT*/
{                                                                                /*#####@v3Q*/

	struct bt_conn *_result;                                                 /*##CRH741M*/

};                                                                               /*##B985gv0*/

static void bt_conn_lookup_addr_le_rpc_rsp(CborValue *_value, void *_handler_data)/*####%Bl1w*/
{                                                                                 /*#####@y1Q*/

	struct bt_conn_lookup_addr_le_rpc_res *_res =                             /*####%Aao3*/
		(struct bt_conn_lookup_addr_le_rpc_res *)_handler_data;           /*#####@XUY*/

	_res->_result = decode_bt_conn(_value);                                   /*##DV3Tpdw*/

	bt_conn_ref_local(_res->_result);

}                                                                                 /*##B9ELNqo*/

struct bt_conn *bt_conn_lookup_addr_le(uint8_t id, const bt_addr_le_t *peer)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AQ*/
	struct bt_conn_lookup_addr_le_rpc_res _result;                           /*######Per*/
	size_t _buffer_size_max = 5;                                             /*######@4M*/

	_buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;                     /*##CKH30f0*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, id);                                      /*####%A/jk*/
	ser_encode_buffer(&_ctx.encoder, peer, sizeof(bt_addr_le_t));            /*#####@Y/k*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LOOKUP_ADDR_LE_RPC_CMD,     /*####%BB6q*/
		&_ctx, bt_conn_lookup_addr_le_rpc_rsp, &_result);                /*#####@mD0*/

	return _result._result;                                                  /*##BW0ge3U*/
}

struct bt_conn_get_dst_out_rpc_res                                               /*####%BqWq*/
{                                                                                /*#####@t8s*/

	bool _result;                                                            /*####%CS8k*/
	bt_addr_le_t * dst;                                                      /*#####@4yA*/

};                                                                               /*##B985gv0*/

static void bt_conn_get_dst_out_rpc_rsp(CborValue *_value, void *_handler_data)  /*####%BjK+*/
{                                                                                /*#####@s/Q*/

	struct bt_conn_get_dst_out_rpc_res *_res =                               /*####%AZTi*/
		(struct bt_conn_get_dst_out_rpc_res *)_handler_data;             /*#####@aPU*/

	_res->_result = ser_decode_bool(_value);                                 /*####%DR+D*/
	ser_decode_buffer(_value, _res->dst, sizeof(bt_addr_le_t));              /*#####@IyI*/

}                                                                                /*##B9ELNqo*/

static bool bt_conn_get_dst_out(const struct bt_conn *conn, bt_addr_le_t *dst)
{
	SERIALIZE(OUT(dst));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ac*/
	struct bt_conn_get_dst_out_rpc_res _result;                              /*######qK2*/
	size_t _buffer_size_max = 3;                                             /*######@JY*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	_result.dst = dst;                                                       /*##CwZuN14*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_DST_OUT_RPC_CMD,        /*####%BGpN*/
		&_ctx, bt_conn_get_dst_out_rpc_rsp, &_result);                   /*#####@Hcs*/

	return _result._result;                                                  /*##BW0ge3U*/
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	bool not_null;

	not_null = bt_conn_get_dst_out(conn, (bt_addr_le_t *)&conn->dst);

	if (not_null) {
		return &conn->dst;
	} else {
		return NULL;
	}
}

uint8_t bt_conn_index(struct bt_conn *conn)
{
	return get_conn_index(conn);
}

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

void bt_conn_le_phy_info_dec(CborValue *_value, struct bt_conn_le_phy_info *_data)/*####%Boer*/
{                                                                                 /*#####@VUM*/

	_data->tx_phy = ser_decode_uint(_value);                                  /*####%CkF8*/
	_data->rx_phy = ser_decode_uint(_value);                                  /*#####@sAY*/

}                                                                                 /*##B9ELNqo*/

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

void bt_conn_le_data_len_info_dec(CborValue *_value, struct bt_conn_le_data_len_info *_data)/*####%BjZZ*/
{                                                                                           /*#####@5uc*/

	_data->tx_max_len = ser_decode_uint(_value);                                        /*######%Cu*/
	_data->tx_max_time = ser_decode_uint(_value);                                       /*#######xr*/
	_data->rx_max_len = ser_decode_uint(_value);                                        /*#######yN*/
	_data->rx_max_time = ser_decode_uint(_value);                                       /*#######@Y*/

}                                                                                           /*##B9ELNqo*/

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

void bt_conn_info_dec(CborValue *value, struct bt_conn *conn, struct bt_conn_info *info)
{
	info->type = ser_decode_uint(value);
	info->role = ser_decode_uint(value);
	info->id = ser_decode_uint(value);

	if (info->type == BT_CONN_TYPE_LE) {
		info->le.interval = ser_decode_uint(value);
		info->le.latency = ser_decode_uint(value);
		info->le.timeout = ser_decode_uint(value);
		LOCK_CONN_INFO();
		info->le.src = ser_decode_buffer(value, &conn->src, sizeof(bt_addr_le_t));
		info->le.dst = ser_decode_buffer(value, &conn->dst, sizeof(bt_addr_le_t));
		info->le.local = ser_decode_buffer(value, &conn->local, sizeof(bt_addr_le_t));
		info->le.remote = ser_decode_buffer(value, &conn->remote, sizeof(bt_addr_le_t));
#if defined(CONFIG_BT_USER_PHY_UPDATE)
		if (ser_decode_is_null(value)) {
			info->le.phy = NULL;
			ser_decode_skip(value);
		} else {
			info->le.phy = &conn->phy;
			bt_conn_le_phy_info_dec(value, &conn->phy);
		}
#else
		ser_decode_skip(value);
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		if (ser_decode_is_null(value)) {
			info->le.data_len = NULL;
			ser_decode_skip(value);
		} else {
			info->le.data_len = &conn->data_len;
			bt_conn_le_data_len_info_dec(value, &conn->data_len);
		}
#else
		ser_decode_skip(value);
#endif
		UNLOCK_CONN_INFO();
	} else {
		/* non-LE connection types are not supported. */
		ser_decoder_invalid(value, CborErrorIO);
	}
}

struct bt_conn_get_info_rpc_res                                                  /*####%BsWN*/
{                                                                                /*#####@I84*/

	int _result;                                                             /*##CWc+iOc*/

	struct bt_conn *conn;
	struct bt_conn_info *info;

};                                                                               /*##B985gv0*/

static void bt_conn_get_info_rpc_rsp(CborValue *_value, void *_handler_data)     /*####%BruR*/
{                                                                                /*#####@vQQ*/

	struct bt_conn_get_info_rpc_res *_res =                                  /*####%Aae8*/
		(struct bt_conn_get_info_rpc_res *)_handler_data;                /*#####@QXw*/

	_res->_result = ser_decode_int(_value);                                  /*##Dbnc/yo*/

	bt_conn_info_dec(_value, _res->conn, _res->info);

}                                                                                /*##B9ELNqo*/

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	SERIALIZE(DEL(info));
	SERIALIZE(CUSTOM_RESPONSE);

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ae*/
	struct bt_conn_get_info_rpc_res _result;                                 /*######T1b*/
	size_t _buffer_size_max = 3;                                             /*######@Qw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	_result.conn = (struct bt_conn *)conn;
	_result.info = info;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_INFO_RPC_CMD,           /*####%BItj*/
		&_ctx, bt_conn_get_info_rpc_rsp, &_result);                      /*#####@hcM*/

	return _result._result;                                                  /*##BW0ge3U*/
}

void bt_conn_remote_info_dec(CborValue *value, struct bt_conn *conn, struct bt_conn_remote_info *remote_info)
{
	remote_info->type = ser_decode_uint(value);
	remote_info->version = ser_decode_uint(value);
	remote_info->manufacturer = ser_decode_uint(value);
	remote_info->subversion = ser_decode_uint(value);

	if (remote_info->type == BT_CONN_TYPE_LE && conn != NULL) {
		LOCK_CONN_INFO();
		remote_info->le.features = ser_decode_buffer(value, &conn->features, sizeof(conn->features));
		UNLOCK_CONN_INFO();
	} else {
		/* non-LE connection types are not supported. */
		ser_decoder_invalid(value, CborErrorIO);
	}
}

struct bt_conn_get_remote_info_rpc_res                                           /*####%BrFP*/
{                                                                                /*#####@/hM*/

	int _result;                                                             /*##CWc+iOc*/

	struct bt_conn *conn;
	struct bt_conn_remote_info *remote_info;

};                                                                               /*##B985gv0*/

static void bt_conn_get_remote_info_rpc_rsp(CborValue *_value, void *_handler_data)/*####%BpNe*/
{                                                                                  /*#####@g8o*/

	struct bt_conn_get_remote_info_rpc_res *_res =                             /*####%AZua*/
		(struct bt_conn_get_remote_info_rpc_res *)_handler_data;           /*#####@R88*/

	_res->_result = ser_decode_int(_value);                                    /*##Dbnc/yo*/

	bt_conn_remote_info_dec(_value, _res->conn, _res->remote_info);

}                                                                                  /*##B9ELNqo*/

int bt_conn_get_remote_info(struct bt_conn *conn,
			    struct bt_conn_remote_info *remote_info)
{
	SERIALIZE(DEL(remote_info));
	SERIALIZE(CUSTOM_RESPONSE);

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ae*/
	struct bt_conn_get_remote_info_rpc_res _result;                          /*######k0Q*/
	size_t _buffer_size_max = 3;                                             /*######@pE*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	_result.conn = conn;
	_result.remote_info = remote_info;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_REMOTE_INFO_RPC_CMD,    /*####%BM2x*/
		&_ctx, bt_conn_get_remote_info_rpc_rsp, &_result);               /*#####@nHw*/

	return _result._result;                                                  /*##BW0ge3U*/
}

UNUSED
static const size_t bt_le_conn_param_buf_size = 12;                              /*##BqJzlts*/

void bt_le_conn_param_enc(CborEncoder *_encoder, const struct bt_le_conn_param *_data)/*####%Bt3a*/
{                                                                                     /*#####@jQM*/

	SERIALIZE(STRUCT(struct bt_le_conn_param));

	ser_encode_uint(_encoder, _data->interval_min);                               /*######%A3*/
	ser_encode_uint(_encoder, _data->interval_max);                               /*#######40*/
	ser_encode_uint(_encoder, _data->latency);                                    /*#######yp*/
	ser_encode_uint(_encoder, _data->timeout);                                    /*#######@I*/

}                                                                                     /*##B9ELNqo*/

void bt_le_conn_param_dec(CborValue *_value, struct bt_le_conn_param *_data)     /*####%BmkY*/
{                                                                                /*#####@uTA*/

	_data->interval_min = ser_decode_uint(_value);                           /*######%Cn*/
	_data->interval_max = ser_decode_uint(_value);                           /*#######EV*/
	_data->latency = ser_decode_uint(_value);                                /*#######N4*/
	_data->timeout = ser_decode_uint(_value);                                /*#######@c*/

}                                                                                /*##B9ELNqo*/

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	int _result;                                                             /*######Xa+*/
	size_t _buffer_size_max = 15;                                            /*######@P8*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%AyYA*/
	bt_le_conn_param_enc(&_ctx.encoder, param);                              /*#####@Baw*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_PARAM_UPDATE_RPC_CMD,    /*####%BAbD*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@30I*/

	return _result;                                                          /*##BX7TDLc*/
}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

UNUSED
static const size_t bt_conn_le_data_len_param_buf_size = 6;                      /*##BlxRCAQ*/

void bt_conn_le_data_len_param_enc(CborEncoder *_encoder, const struct bt_conn_le_data_len_param *_data)/*####%Bs1U*/
{                                                                                                       /*#####@fe0*/

	SERIALIZE(STRUCT(struct bt_conn_le_data_len_param));

	ser_encode_uint(_encoder, _data->tx_max_len);                                                   /*####%AwTg*/
	ser_encode_uint(_encoder, _data->tx_max_time);                                                  /*#####@GXI*/

}                                                                                                       /*##B9ELNqo*/

int bt_conn_le_data_len_update(struct bt_conn *conn,
			       const struct bt_conn_le_data_len_param *param)
{
       SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ad*/
	int _result;                                                             /*######voK*/
	size_t _buffer_size_max = 9;                                             /*######@/8*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A8e3*/
	bt_conn_le_data_len_param_enc(&_ctx.encoder, param);                     /*#####@Y8Y*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD, /*####%BMcu*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@3+U*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

UNUSED
static const size_t bt_conn_le_phy_param_buf_size = 7;                           /*##BnRuFXI*/

void bt_conn_le_phy_param_enc(CborEncoder *_encoder, const struct bt_conn_le_phy_param *_data)/*####%BpZB*/
{                                                                                             /*#####@eko*/

	SERIALIZE(STRUCT(struct bt_conn_le_phy_param));

	ser_encode_uint(_encoder, _data->options);                                            /*######%Ax*/
	ser_encode_uint(_encoder, _data->pref_tx_phy);                                        /*######nvG*/
	ser_encode_uint(_encoder, _data->pref_rx_phy);                                        /*######@zY*/

}                                                                                             /*##B9ELNqo*/

int bt_conn_le_phy_update(struct bt_conn *conn,
			  const struct bt_conn_le_phy_param *param)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ac*/
	int _result;                                                             /*######PRx*/
	size_t _buffer_size_max = 10;                                            /*######@Yo*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A6bU*/
	bt_conn_le_phy_param_enc(&_ctx.encoder, param);                          /*#####@COw*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_PHY_UPDATE_RPC_CMD,      /*####%BCZy*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@rts*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Aa*/
	int _result;                                                             /*######Qso*/
	size_t _buffer_size_max = 5;                                             /*######@uA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A+Lp*/
	ser_encode_uint(&_ctx.encoder, reason);                                  /*#####@OkE*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_DISCONNECT_RPC_CMD,         /*####%BGFR*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@Lic*/

	return _result;                                                          /*##BX7TDLc*/
}

#if defined(CONFIG_BT_CENTRAL) || defined(__GENERATOR)

UNUSED
static const size_t bt_conn_le_create_param_buf_size = 20;                       /*##Bv4miOQ*/

void bt_conn_le_create_param_enc(CborEncoder *_encoder, const struct bt_conn_le_create_param *_data)/*####%Bvx+*/
{                                                                                                   /*#####@S50*/

	SERIALIZE(STRUCT(struct bt_conn_le_create_param));

	ser_encode_uint(_encoder, _data->options);                                                  /*#######%A*/
	ser_encode_uint(_encoder, _data->interval);                                                 /*#######5Q*/
	ser_encode_uint(_encoder, _data->window);                                                   /*#######kh*/
	ser_encode_uint(_encoder, _data->interval_coded);                                           /*########n*/
	ser_encode_uint(_encoder, _data->window_coded);                                             /*########c*/
	ser_encode_uint(_encoder, _data->timeout);                                                  /*########@*/

}                                                                                                   /*##B9ELNqo*/

struct bt_conn_le_create_rpc_res                                                 /*####%Bu1j*/
{                                                                                /*#####@PwU*/

	int _result;                                                             /*####%CbWP*/
	struct bt_conn ** conn;                                                  /*#####@mxQ*/

};                                                                               /*##B985gv0*/

static void bt_conn_le_create_rpc_rsp(CborValue *_value, void *_handler_data)    /*####%BhuD*/
{                                                                                /*#####@pbM*/

	struct bt_conn_le_create_rpc_res *_res =                                 /*####%AXl/*/
		(struct bt_conn_le_create_rpc_res *)_handler_data;               /*#####@D3E*/

	_res->_result = ser_decode_int(_value);                                  /*####%DbA5*/
	*(_res->conn) = decode_bt_conn(_value);                                  /*#####@uvI*/

	bt_conn_ref_local(*(_res->conn));

}                                                                                /*##B9ELNqo*/

int bt_conn_le_create(const bt_addr_le_t *peer,
		      const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param,
		      struct bt_conn **conn)
{
	SERIALIZE(OUT(conn));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Ab*/
	struct bt_conn_le_create_rpc_res _result;                                /*######HAH*/
	size_t _buffer_size_max = 35;                                            /*######@h0*/

	_buffer_size_max += peer ? sizeof(bt_addr_le_t) : 0;                     /*##CKH30f0*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_buffer(&_ctx.encoder, peer, sizeof(bt_addr_le_t));            /*######%A6*/
	bt_conn_le_create_param_enc(&_ctx.encoder, create_param);                /*######4dq*/
	bt_le_conn_param_enc(&_ctx.encoder, conn_param);                         /*######@8I*/

	_result.conn = conn;                                                     /*##C48A3sw*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_CREATE_RPC_CMD,          /*####%BMN8*/
		&_ctx, bt_conn_le_create_rpc_rsp, &_result);                     /*#####@S2s*/

	return _result._result;                                                  /*##BW0ge3U*/
}

#if defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR)

int bt_conn_le_create_auto(const struct bt_conn_le_create_param *create_param,
			   const struct bt_le_conn_param *conn_param)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AU*/
	int _result;                                                             /*######RM+*/
	size_t _buffer_size_max = 32;                                            /*######@4U*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	bt_conn_le_create_param_enc(&_ctx.encoder, create_param);                /*####%AwjT*/
	bt_le_conn_param_enc(&_ctx.encoder, conn_param);                         /*#####@DaY*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_LE_CREATE_AUTO_RPC_CMD,     /*####%BFBU*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@ev4*/

	return _result;                                                          /*##BX7TDLc*/
}

int bt_conn_create_auto_stop(void)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AX*/
	int _result;                                                             /*######56+*/
	size_t _buffer_size_max = 0;                                             /*######@io*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CREATE_AUTO_STOP_RPC_CMD,   /*####%BJ7W*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@4r0*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR) */

#if !defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR)

int bt_le_set_auto_conn(const bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	SERIALIZE(NULLABLE(param));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	int _result;                                                             /*######RDP*/
	size_t _buffer_size_max = 3;                                             /*######@sI*/

	_buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;                     /*####%CH+R*/
	_buffer_size_max += (param == NULL) ? 1 : 12;                            /*#####@QAI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_buffer(&_ctx.encoder, addr, sizeof(bt_addr_le_t));            /*#######%A*/
	if (param == NULL) {                                                     /*#######8h*/
		ser_encode_null(&_ctx.encoder);                                  /*#######PK*/
	} else {                                                                 /*########N*/
		bt_le_conn_param_enc(&_ctx.encoder, param);                      /*########g*/
	}                                                                        /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_SET_AUTO_CONN_RPC_CMD,        /*####%BHc8*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@M0A*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* !defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR) */

#endif /* defined(CONFIG_BT_CENTRAL) || defined(__GENERATOR) */

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

int bt_conn_set_security(struct bt_conn *conn, bt_security_t sec)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AZ*/
	int _result;                                                             /*######I55*/
	size_t _buffer_size_max = 8;                                             /*######@3E*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A+6f*/
	ser_encode_uint(&_ctx.encoder, (uint32_t)sec);                           /*#####@M/4*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_SET_SECURITY_RPC_CMD,       /*####%BCbM*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@nYc*/

	return _result;                                                          /*##BX7TDLc*/
}


struct bt_conn_get_security_rpc_res                                              /*####%BhPh*/
{                                                                                /*#####@0JQ*/

	bt_security_t _result;                                                   /*##CeMjkRQ*/

};                                                                               /*##B985gv0*/

static void bt_conn_get_security_rpc_rsp(CborValue *_value, void *_handler_data) /*####%Brtq*/
{                                                                                /*#####@jz0*/

	struct bt_conn_get_security_rpc_res *_res =                              /*####%AY+c*/
		(struct bt_conn_get_security_rpc_res *)_handler_data;            /*#####@eIw*/

	_res->_result = (bt_security_t)ser_decode_uint(_value);                  /*##DS0uuQY*/

}                                                                                /*##B9ELNqo*/

bt_security_t bt_conn_get_security(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AZ*/
	struct bt_conn_get_security_rpc_res _result;                             /*######2SK*/
	size_t _buffer_size_max = 3;                                             /*######@Vc*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_GET_SECURITY_RPC_CMD,       /*####%BKVe*/
		&_ctx, bt_conn_get_security_rpc_rsp, &_result);                  /*#####@Sxs*/

	return _result._result;                                                  /*##BW0ge3U*/
}

uint8_t bt_conn_enc_key_size(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AW*/
	uint8_t _result;                                                         /*######r71*/
	size_t _buffer_size_max = 3;                                             /*######@fQ*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_ENC_KEY_SIZE_RPC_CMD,       /*####%BHXr*/
		&_ctx, ser_rsp_simple_u8, &_result);                             /*#####@Jgw*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

static struct bt_conn_cb *first_bt_conn_cb = NULL;

static void bt_conn_cb_connected_call(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->connected) {
			cb->connected(conn, err);
		}
	}
}

static void bt_conn_cb_connected_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%Brdq*/
{                                                                                        /*#####@atQ*/

	struct bt_conn * conn;                                                           /*####%AVTh*/
	uint8_t err;                                                                     /*#####@hhk*/

	conn = decode_bt_conn(_value);                                                   /*####%CvMn*/
	err = ser_decode_uint(_value);                                                   /*#####@kx8*/

	if (!ser_decoding_done_and_check(_value)) {                                      /*######%FE*/
		goto decoding_error;                                                     /*######QTM*/
	}                                                                                /*######@1Y*/

	bt_conn_cb_connected_call(conn, err);                                            /*##DpvJ7lo*/

	ser_rsp_send_void();                                                             /*##BEYGLxw*/

	return;                                                                          /*######%B8*/
decoding_error:                                                                          /*#######Zt*/
	report_decoding_error(BT_CONN_CB_CONNECTED_CALL_RPC_CMD, _handler_data);         /*#######aV*/
}                                                                                        /*#######@M*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_connected_call, BT_CONN_CB_CONNECTED_CALL_RPC_CMD,/*####%BpVL*/
	bt_conn_cb_connected_call_rpc_handler, NULL);                                             /*#####@lwQ*/

static void bt_conn_cb_disconnected_call(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->disconnected) {
			cb->disconnected(conn, reason);
		}
	}
}

static void bt_conn_cb_disconnected_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%BrKq*/
{                                                                                           /*#####@ZkM*/

	struct bt_conn * conn;                                                              /*####%Acme*/
	uint8_t reason;                                                                     /*#####@zHg*/

	conn = decode_bt_conn(_value);                                                      /*####%CoIh*/
	reason = ser_decode_uint(_value);                                                   /*#####@ocQ*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	bt_conn_cb_disconnected_call(conn, reason);                                         /*##Duy6FBA*/

	ser_rsp_send_void();                                                                /*##BEYGLxw*/

	return;                                                                             /*######%Bx*/
decoding_error:                                                                             /*#######xQ*/
	report_decoding_error(BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD, _handler_data);         /*#######fb*/
}                                                                                           /*#######@Y*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_disconnected_call, BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD,/*####%Blgz*/
	bt_conn_cb_disconnected_call_rpc_handler, NULL);                                                /*#####@jtw*/

static bool bt_conn_cb_le_param_req_call(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_param_req) {
			if (!cb->le_param_req(conn, param)) {
				return false;
			}
		}
	}

	return true;
}

static void bt_conn_cb_le_param_req_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bj/w*/
{                                                                                           /*#####@NuI*/

	struct bt_conn * conn;                                                              /*######%Ae*/
	struct bt_le_conn_param param;                                                      /*######N+p*/
	bool _result;                                                                       /*######@pY*/

	conn = decode_bt_conn(_value);                                                      /*####%Cun5*/
	bt_le_conn_param_dec(_value, &param);                                               /*#####@kEE*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	_result = bt_conn_cb_le_param_req_call(conn, &param);                               /*##DmZtDO8*/

	ser_rsp_send_bool(_result);                                                         /*##BPuGKyE*/

	return;                                                                             /*######%B+*/
decoding_error:                                                                             /*#######PW*/
	report_decoding_error(BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD, _handler_data);         /*#######Xq*/
}                                                                                           /*#######@Q*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_param_req_call, BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD,/*####%BsN8*/
	bt_conn_cb_le_param_req_call_rpc_handler, NULL);                                                /*#####@hCM*/

static void bt_conn_cb_le_param_updated_call(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_param_updated) {
			cb->le_param_updated(conn, interval, latency, timeout);
		}
	}
}

static void bt_conn_cb_le_param_updated_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%BkB/*/
{                                                                                               /*#####@j78*/

	struct bt_conn * conn;                                                                  /*######%AW*/
	uint16_t interval;                                                                      /*#######JY*/
	uint16_t latency;                                                                       /*#######O6*/
	uint16_t timeout;                                                                       /*#######@E*/

	conn = decode_bt_conn(_value);                                                          /*######%Ci*/
	interval = ser_decode_uint(_value);                                                     /*#######AF*/
	latency = ser_decode_uint(_value);                                                      /*#######/a*/
	timeout = ser_decode_uint(_value);                                                      /*#######@s*/

	if (!ser_decoding_done_and_check(_value)) {                                             /*######%FE*/
		goto decoding_error;                                                            /*######QTM*/
	}                                                                                       /*######@1Y*/

	bt_conn_cb_le_param_updated_call(conn, interval, latency, timeout);                     /*##DiXeers*/

	ser_rsp_send_void();                                                                    /*##BEYGLxw*/

	return;                                                                                 /*######%By*/
decoding_error:                                                                                 /*#######mx*/
	report_decoding_error(BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD, _handler_data);         /*#######3R*/
}                                                                                               /*#######@g*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_param_updated_call, BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD,/*####%BmVP*/
	bt_conn_cb_le_param_updated_call_rpc_handler, NULL);                                                    /*#####@w4s*/

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

static void bt_conn_cb_identity_resolved_call(struct bt_conn *conn, const bt_addr_le_t *rpa, const bt_addr_le_t *identity)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, identity);
		}
	}
}

static void bt_conn_cb_identity_resolved_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bu+G*/
{                                                                                                /*#####@B9g*/

	struct bt_conn * conn;                                                                   /*#######%A*/
	bt_addr_le_t _rpa_data;                                                                  /*#######YA*/
	const bt_addr_le_t * rpa;                                                                /*#######FZ*/
	bt_addr_le_t _identity_data;                                                             /*#######rQ*/
	const bt_addr_le_t * identity;                                                           /*########@*/

	conn = decode_bt_conn(_value);                                                           /*######%Cv*/
	rpa = ser_decode_buffer(_value, &_rpa_data, sizeof(bt_addr_le_t));                       /*######8At*/
	identity = ser_decode_buffer(_value, &_identity_data, sizeof(bt_addr_le_t));             /*######@tg*/

	if (!ser_decoding_done_and_check(_value)) {                                              /*######%FE*/
		goto decoding_error;                                                             /*######QTM*/
	}                                                                                        /*######@1Y*/

	bt_conn_cb_identity_resolved_call(conn, rpa, identity);                                  /*##DkGcqF0*/

	ser_rsp_send_void();                                                                     /*##BEYGLxw*/

	return;                                                                                  /*######%Bw*/
decoding_error:                                                                                  /*#######CX*/
	report_decoding_error(BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD, _handler_data);         /*#######23*/
}                                                                                                /*#######@0*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_identity_resolved_call, BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD,/*####%Bl6V*/
	bt_conn_cb_identity_resolved_call_rpc_handler, NULL);                                                     /*#####@2+8*/

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

static void bt_conn_cb_security_changed_call(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->security_changed(conn, level, err);
		}
	}
}

static void bt_conn_cb_security_changed_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%BjTn*/
{                                                                                               /*#####@DBA*/

	struct bt_conn * conn;                                                                  /*######%Af*/
	bt_security_t level;                                                                    /*######uqN*/
	enum bt_security_err err;                                                               /*######@OI*/

	conn = decode_bt_conn(_value);                                                          /*######%Cj*/
	level = (bt_security_t)ser_decode_uint(_value);                                         /*######MNf*/
	err = (enum bt_security_err)ser_decode_uint(_value);                                    /*######@Fc*/

	if (!ser_decoding_done_and_check(_value)) {                                             /*######%FE*/
		goto decoding_error;                                                            /*######QTM*/
	}                                                                                       /*######@1Y*/

	bt_conn_cb_security_changed_call(conn, level, err);                                     /*##DtZ4KXo*/

	ser_rsp_send_void();                                                                    /*##BEYGLxw*/

	return;                                                                                 /*######%B4*/
decoding_error:                                                                                 /*#######Qr*/
	report_decoding_error(BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD, _handler_data);         /*#######dP*/
}                                                                                               /*#######@s*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_security_changed_call, BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD,/*####%BtY8*/
	bt_conn_cb_security_changed_call_rpc_handler, NULL);                                                    /*#####@Q4w*/

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

#if defined(CONFIG_BT_REMOTE_INFO) || defined(__GENERATOR)

static void bt_conn_cb_remote_info_available_call(struct bt_conn *conn, struct bt_conn_remote_info *remote_info)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->remote_info_available) {
			cb->remote_info_available(conn, remote_info);
		}
	}
}

static void bt_conn_cb_remote_info_available_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%BovW*/
{                                                                                                    /*#####@zjU*/

	struct bt_conn * conn;                                                                       /*##AWn5t/c*/

	struct bt_conn_remote_info remote_info;

	conn = decode_bt_conn(_value);                                                               /*##CgkztUA*/

	bt_conn_remote_info_dec(_value, conn, &remote_info);

	if (!ser_decoding_done_and_check(_value)) {                                                  /*######%FE*/
		goto decoding_error;                                                                 /*######QTM*/
	}                                                                                            /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	bt_conn_cb_remote_info_available_call(conn, &remote_info);

	ser_rsp_send_void();                                                                         /*##BEYGLxw*/

	return;                                                                                      /*######%B6*/
decoding_error:                                                                                      /*#######I/*/
	report_decoding_error(BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD, _handler_data);         /*#######UY*/
}                                                                                                    /*#######@g*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_remote_info_available_call, BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD,/*####%Bkxy*/
	bt_conn_cb_remote_info_available_call_rpc_handler, NULL);                                                         /*#####@+X4*/

#endif /* defined(CONFIG_BT_REMOTE_INFO) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

static void bt_conn_cb_le_phy_updated_call(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_phy_updated) {
			cb->le_phy_updated(conn, param);
		}
	}
}

static void bt_conn_cb_le_phy_updated_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%BsRh*/
{                                                                                             /*#####@ymI*/

	struct bt_conn * conn;                                                                /*####%AfEO*/
	struct bt_conn_le_phy_info param;                                                     /*#####@Yck*/

	conn = decode_bt_conn(_value);                                                        /*####%CnCb*/
	bt_conn_le_phy_info_dec(_value, &param);                                              /*#####@YtA*/

	if (!ser_decoding_done_and_check(_value)) {                                           /*######%FE*/
		goto decoding_error;                                                          /*######QTM*/
	}                                                                                     /*######@1Y*/

	bt_conn_cb_le_phy_updated_call(conn, &param);                                         /*##Dqn+VQw*/

	ser_rsp_send_void();                                                                  /*##BEYGLxw*/

	return;                                                                               /*######%Bw*/
decoding_error:                                                                               /*#######ou*/
	report_decoding_error(BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD, _handler_data);         /*#######2+*/
}                                                                                             /*#######@4*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_phy_updated_call, BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD,/*####%Bmm8*/
	bt_conn_cb_le_phy_updated_call_rpc_handler, NULL);                                                  /*#####@Wz8*/

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

static void bt_conn_cb_le_data_len_updated_call(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
	struct bt_conn_cb *cb;

	for (cb = first_bt_conn_cb; cb; cb = cb->_next) {
		if (cb->le_data_len_updated) {
			cb->le_data_len_updated(conn, info);
		}
	}
}

static void bt_conn_cb_le_data_len_updated_call_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bs8y*/
{                                                                                                  /*#####@AOY*/

	struct bt_conn * conn;                                                                     /*####%AesM*/
	struct bt_conn_le_data_len_info info;                                                      /*#####@ewQ*/

	conn = decode_bt_conn(_value);                                                             /*####%Cq9W*/
	bt_conn_le_data_len_info_dec(_value, &info);                                               /*#####@87w*/

	if (!ser_decoding_done_and_check(_value)) {                                                /*######%FE*/
		goto decoding_error;                                                               /*######QTM*/
	}                                                                                          /*######@1Y*/

	bt_conn_cb_le_data_len_updated_call(conn, &info);                                          /*##DoQ82TU*/

	ser_rsp_send_void();                                                                       /*##BEYGLxw*/

	return;                                                                                    /*######%B+*/
decoding_error:                                                                                    /*#######VG*/
	report_decoding_error(BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD, _handler_data);         /*#######Tx*/
}                                                                                                  /*#######@I*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_le_data_len_updated_call, BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD,/*####%Br4G*/
	bt_conn_cb_le_data_len_updated_call_rpc_handler, NULL);                                                       /*#####@7Us*/

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

static void bt_conn_cb_register_on_remote()
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                              /*####%ATMv*/
	size_t _buffer_size_max = 0;                                               /*#####@1d4*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                /*##AvrU03s*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_REGISTER_ON_REMOTE_RPC_CMD,/*####%BGWx*/
		&_ctx, ser_rsp_simple_void, NULL);                                 /*#####@wpU*/
}

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
	bool register_on_remote;

	LOCK_CONN_INFO();
	register_on_remote = (first_bt_conn_cb == NULL);
	cb->_next = first_bt_conn_cb;
	first_bt_conn_cb = cb;
	UNLOCK_CONN_INFO();

	if (register_on_remote) {
		bt_conn_cb_register_on_remote();
	}
}

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

void bt_set_bondable(bool enable) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%AZz+*/
	size_t _buffer_size_max = 1;                                             /*#####@G+Y*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_bool(&_ctx.encoder, enable);                                  /*##AyLT/1M*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_BONDABLE_RPC_CMD,            /*####%BAOE*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@LZE*/
}

void bt_set_oob_data_flag(bool enable)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%AZz+*/
	size_t _buffer_size_max = 1;                                             /*#####@G+Y*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_bool(&_ctx.encoder, enable);                                  /*##AyLT/1M*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_SET_OOB_DATA_FLAG_RPC_CMD,       /*####%BBhC*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@myc*/
}

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) || defined(__GENERATOR)

int bt_le_oob_set_legacy_tk(struct bt_conn *conn, const uint8_t *tk)
{
	SERIALIZE(SIZE(tk, 16));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*#######%A*/
	size_t _tk_size;                                                         /*#######SI*/
	int _result;                                                             /*#######uc*/
	size_t _scratchpad_size = 0;                                             /*#######P8*/
	size_t _buffer_size_max = 13;                                            /*########@*/

	_tk_size = sizeof(uint8_t) * 16;                                         /*####%CAer*/
	_buffer_size_max += _tk_size;                                            /*#####@pxg*/

	_scratchpad_size += SCRATCHPAD_ALIGN(_tk_size);                          /*##EF89I8U*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*####%AoDN*/
	ser_encode_uint(&_ctx.encoder, _scratchpad_size);                        /*#####@BNc*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A5j9*/
	ser_encode_buffer(&_ctx.encoder, tk, _tk_size);                          /*#####@iK8*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_LEGACY_TK_RPC_CMD,    /*####%BIJK*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@byI*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) || defined(__GENERATOR) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) || defined(__GENERATOR)

size_t bt_le_oob_sc_data_buf_size(const struct bt_le_oob_sc_data *_data)         /*####%BhDG*/
{                                                                                /*#####@jx4*/

	size_t _buffer_size_max = 10;                                            /*##ATzALbk*/

	_buffer_size_max += 16 * sizeof(uint8_t);                                /*####%CK2q*/
	_buffer_size_max += 16 * sizeof(uint8_t);                                /*#####@c/o*/

	return _buffer_size_max;                                                 /*##BWmN6G8*/

}                                                                                /*##B9ELNqo*/

void bt_le_oob_sc_data_enc(CborEncoder *_encoder, const struct bt_le_oob_sc_data *_data)/*####%Bjhl*/
{                                                                                       /*#####@eHQ*/

	SERIALIZE(STRUCT(struct bt_le_oob_sc_data));

	ser_encode_buffer(_encoder, _data->r, 16 * sizeof(uint8_t));                    /*####%A14m*/
	ser_encode_buffer(_encoder, _data->c, 16 * sizeof(uint8_t));                    /*#####@e/E*/

}                                                                                       /*##B9ELNqo*/

void bt_le_oob_sc_data_dec(CborValue *_value, struct bt_le_oob_sc_data *_data)   /*####%BkKT*/
{                                                                                /*#####@jhE*/

	ser_decode_buffer(_value, _data->r, 16 * sizeof(uint8_t));               /*####%Cmrc*/
	ser_decode_buffer(_value, _data->c, 16 * sizeof(uint8_t));               /*#####@I/0*/

}                                                                                /*##B9ELNqo*/

int bt_le_oob_set_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data *oobd_local,
			  const struct bt_le_oob_sc_data *oobd_remote)
{
	SERIALIZE(NULLABLE(oobd_local));
	SERIALIZE(NULLABLE(oobd_remote));

	struct nrf_rpc_cbor_ctx _ctx;                                                                /*######%AR*/
	int _result;                                                                                 /*######RDP*/
	size_t _buffer_size_max = 3;                                                                 /*######@sI*/

	_buffer_size_max += (oobd_local == NULL) ? 1 : 0 + bt_le_oob_sc_data_buf_size(oobd_local);;  /*####%CAmf*/
	_buffer_size_max += (oobd_remote == NULL) ? 1 : 0 + bt_le_oob_sc_data_buf_size(oobd_remote);;/*#####@iWA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                                  /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                                         /*########%*/
	if (oobd_local == NULL) {                                                                    /*########A*/
		ser_encode_null(&_ctx.encoder);                                                      /*########3*/
	} else {                                                                                     /*########D*/
		bt_le_oob_sc_data_enc(&_ctx.encoder, oobd_local);                                    /*########e*/
	}                                                                                            /*########O*/
	if (oobd_remote == NULL) {                                                                   /*########J*/
		ser_encode_null(&_ctx.encoder);                                                      /*########M*/
	} else {                                                                                     /*#########*/
		bt_le_oob_sc_data_enc(&_ctx.encoder, oobd_remote);                                   /*#########*/
	}                                                                                            /*########@*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_SET_SC_DATA_RPC_CMD,                          /*####%BBT8*/
		&_ctx, ser_rsp_simple_i32, &_result);                                                /*#####@x4Y*/

	return _result;                                                                              /*##BX7TDLc*/
}


struct bt_le_oob_get_sc_data_rpc_res                                             /*####%Bmci*/
{                                                                                /*#####@gIw*/

	int _result;                                                             /*##CWc+iOc*/

	struct bt_conn *conn;
	const struct bt_le_oob_sc_data **oobd_local;
	const struct bt_le_oob_sc_data **oobd_remote;

};                                                                               /*##B985gv0*/

static void bt_le_oob_get_sc_data_rpc_rsp(CborValue *_value, void *_handler_data)/*####%BhN9*/
{                                                                                /*#####@BTM*/

	struct bt_le_oob_get_sc_data_rpc_res *_res =                             /*####%AT4P*/
		(struct bt_le_oob_get_sc_data_rpc_res *)_handler_data;           /*#####@3x4*/

	_res->_result = ser_decode_int(_value);                                  /*##Dbnc/yo*/

	if (ser_decode_is_null(_value))
	{
		*_res->oobd_local = NULL;
		ser_decode_skip(_value);
	} else {
		*_res->oobd_local = &_res->conn->oobd_local;
		bt_le_oob_sc_data_dec(_value, &_res->conn->oobd_local);
	}

	if (ser_decode_is_null(_value))
	{
		*_res->oobd_remote = NULL;
		ser_decode_skip(_value);
	} else {
		*_res->oobd_remote = &_res->conn->oobd_remote;
		bt_le_oob_sc_data_dec(_value, &_res->conn->oobd_remote);
	}

}                                                                                /*##B9ELNqo*/

int bt_le_oob_get_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data **oobd_local,
			  const struct bt_le_oob_sc_data **oobd_remote)
{
	SERIALIZE(CUSTOM_RESPONSE);
	SERIALIZE(DEL(oobd_local));
	SERIALIZE(DEL(oobd_remote));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	struct bt_le_oob_get_sc_data_rpc_res _result;                            /*######KAK*/
	size_t _buffer_size_max = 3;                                             /*######@Xc*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	_result.conn = conn;
	_result.oobd_local = oobd_local;
	_result.oobd_remote = oobd_remote;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_LE_OOB_GET_SC_DATA_RPC_CMD,      /*####%BI/+*/
		&_ctx, bt_le_oob_get_sc_data_rpc_rsp, &_result);                 /*#####@Fxg*/

	return _result._result;                                                  /*##BW0ge3U*/
}

#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) || defined(__GENERATOR) */

#if defined(CONFIG_BT_FIXED_PASSKEY) || defined(__GENERATOR)

int bt_passkey_set(unsigned int passkey)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%Aa*/
	int _result;                                                             /*######Qso*/
	size_t _buffer_size_max = 5;                                             /*######@uA*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, passkey);                                 /*##A8e/BdU*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_PASSKEY_SET_RPC_CMD,             /*####%BCj6*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@qTs*/

	return _result;                                                          /*##BX7TDLc*/
}

#endif /* defined(CONFIG_BT_FIXED_PASSKEY) || defined(__GENERATOR) */

static const struct bt_conn_auth_cb *auth_cb = NULL;

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT) || defined(__GENERATOR)

void bt_conn_pairing_feat_dec(CborValue *_value, struct bt_conn_pairing_feat *_data)/*####%Bno2*/
{                                                                                   /*#####@D28*/

	_data->io_capability = ser_decode_uint(_value);                             /*#######%C*/
	_data->oob_data_flag = ser_decode_uint(_value);                             /*#######lN*/
	_data->auth_req = ser_decode_uint(_value);                                  /*#######Ou*/
	_data->max_enc_key_size = ser_decode_uint(_value);                          /*########Z*/
	_data->init_key_dist = ser_decode_uint(_value);                             /*########Q*/
	_data->resp_key_dist = ser_decode_uint(_value);                             /*########@*/

}                                                                                   /*##B9ELNqo*/

static void bt_rpc_auth_cb_pairing_accept_rpc_handler(CborValue *_value, void *_handler_data)/*####%BpSx*/
{                                                                                            /*#####@e/o*/

	struct nrf_rpc_cbor_ctx _ctx;                                                        /*#######%A*/
	enum bt_security_err _result;                                                        /*#######WF*/
	struct bt_conn * conn;                                                               /*#######1z*/
	struct bt_conn_pairing_feat feat;                                                    /*#######Z8*/
	size_t _buffer_size_max = 5;                                                         /*########@*/

	conn = decode_bt_conn(_value);                                                       /*####%Cu2i*/
	bt_conn_pairing_feat_dec(_value, &feat);                                             /*#####@pAw*/

	if (!ser_decoding_done_and_check(_value)) {                                          /*######%FE*/
		goto decoding_error;                                                         /*######QTM*/
	}                                                                                    /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->pairing_accept) {
		_result = auth_cb->pairing_accept(conn, &feat);
	} else {
		_result = BT_SECURITY_ERR_INVALID_PARAM;
	}

	{                                                                                    /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                  /*#####@zxs*/

		ser_encode_uint(&_ctx.encoder, (uint32_t)_result);                           /*##DHYvGBI*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                              /*####%BIlG*/
	}                                                                                    /*#####@TnU*/

	return;                                                                              /*######%FS*/
decoding_error:                                                                              /*######mtz*/
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD, _handler_data);         /*######@bI*/

}                                                                                            /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_accept, BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD,/*####%BhNc*/
	bt_rpc_auth_cb_pairing_accept_rpc_handler, NULL);                                                 /*#####@VbM*/

#endif /* defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT) || defined(__GENERATOR) */

static void bt_rpc_auth_cb_passkey_display_rpc_handler(CborValue *_value, void *_handler_data)/*####%BmkZ*/
{                                                                                             /*#####@axk*/

	struct bt_conn * conn;                                                                /*####%Afds*/
	unsigned int passkey;                                                                 /*#####@O5s*/

	conn = decode_bt_conn(_value);                                                        /*####%CuFE*/
	passkey = ser_decode_uint(_value);                                                    /*#####@Qoc*/

	if (!ser_decoding_done_and_check(_value)) {                                           /*######%FE*/
		goto decoding_error;                                                          /*######QTM*/
	}                                                                                     /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->passkey_display) {
		auth_cb->passkey_display(conn, passkey);
	}

	ser_rsp_send_void();                                                                  /*##BEYGLxw*/

	return;                                                                               /*######%Fd*/
decoding_error:                                                                               /*######Lkn*/
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD, _handler_data);         /*######@j0*/

}                                                                                             /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_display, BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD,/*####%Bu2C*/
	bt_rpc_auth_cb_passkey_display_rpc_handler, NULL);                                                  /*#####@doI*/

static void bt_rpc_auth_cb_passkey_entry_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bhxr*/
{                                                                                           /*#####@frI*/

	struct bt_conn * conn;                                                              /*##AWn5t/c*/

	conn = decode_bt_conn(_value);                                                      /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->passkey_entry) {
		auth_cb->passkey_entry(conn);
	}

	ser_rsp_send_void();                                                                /*##BEYGLxw*/

	return;                                                                             /*######%FX*/
decoding_error:                                                                             /*######yIc*/
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD, _handler_data);         /*######@dE*/

}                                                                                           /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_entry, BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD,/*####%Bjp3*/
	bt_rpc_auth_cb_passkey_entry_rpc_handler, NULL);                                                /*#####@wGg*/

static void bt_rpc_auth_cb_passkey_confirm_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bt4f*/
{                                                                                             /*#####@d9g*/

	struct bt_conn * conn;                                                                /*####%Afds*/
	unsigned int passkey;                                                                 /*#####@O5s*/

	conn = decode_bt_conn(_value);                                                        /*####%CuFE*/
	passkey = ser_decode_uint(_value);                                                    /*#####@Qoc*/

	if (!ser_decoding_done_and_check(_value)) {                                           /*######%FE*/
		goto decoding_error;                                                          /*######QTM*/
	}                                                                                     /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->passkey_confirm) {
		auth_cb->passkey_confirm(conn, passkey);
	}

	ser_rsp_send_void();                                                                  /*##BEYGLxw*/

	return;                                                                               /*######%Fc*/
decoding_error:                                                                               /*######zqQ*/
	report_decoding_error(BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD, _handler_data);         /*######@sk*/

}                                                                                             /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_passkey_confirm, BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD,/*####%BgPc*/
	bt_rpc_auth_cb_passkey_confirm_rpc_handler, NULL);                                                  /*#####@54Q*/

static void bt_rpc_auth_cb_oob_data_request_rpc_handler(CborValue *_value, void *_handler_data)/*####%BsO8*/
{                                                                                              /*#####@zMQ*/

	struct bt_conn * conn;                                                                 /*######%Ab*/
	struct bt_conn_oob_info _info_data;                                                    /*######xVE*/
	struct bt_conn_oob_info * info;                                                        /*######@Lo*/

	conn = decode_bt_conn(_value);                                                         /*####%Ch9B*/
	info = ser_decode_buffer(_value, &_info_data, sizeof(struct bt_conn_oob_info));        /*#####@9dA*/

	if (!ser_decoding_done_and_check(_value)) {                                            /*######%FE*/
		goto decoding_error;                                                           /*######QTM*/
	}                                                                                      /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->oob_data_request) {
		auth_cb->oob_data_request(conn, info);
	}

	ser_rsp_send_void();                                                                   /*##BEYGLxw*/

	return;                                                                                /*######%Fe*/
decoding_error:                                                                                /*######Lz4*/
	report_decoding_error(BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD, _handler_data);         /*######@6w*/

}                                                                                              /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_oob_data_request, BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD,/*####%BvFi*/
	bt_rpc_auth_cb_oob_data_request_rpc_handler, NULL);                                                   /*#####@83o*/

static void bt_rpc_auth_cb_cancel_rpc_handler(CborValue *_value, void *_handler_data)/*####%BrXQ*/
{                                                                                    /*#####@Rps*/

	struct bt_conn * conn;                                                       /*##AWn5t/c*/

	conn = decode_bt_conn(_value);                                               /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->cancel) {
		auth_cb->cancel(conn);
	}

	ser_rsp_send_void();                                                         /*##BEYGLxw*/

	return;                                                                      /*######%FQ*/
decoding_error:                                                                      /*######RcM*/
	report_decoding_error(BT_RPC_AUTH_CB_CANCEL_RPC_CMD, _handler_data);         /*######@6M*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_cancel, BT_RPC_AUTH_CB_CANCEL_RPC_CMD,/*####%BmDK*/
	bt_rpc_auth_cb_cancel_rpc_handler, NULL);                                         /*#####@Iuw*/

static void bt_rpc_auth_cb_pairing_confirm_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bjm/*/
{                                                                                             /*#####@bG0*/

	struct bt_conn * conn;                                                                /*##AWn5t/c*/

	conn = decode_bt_conn(_value);                                                        /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                           /*######%FE*/
		goto decoding_error;                                                          /*######QTM*/
	}                                                                                     /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->pairing_confirm) {
		auth_cb->pairing_confirm(conn);
	}

	ser_rsp_send_void();                                                                  /*##BEYGLxw*/

	return;                                                                               /*######%FU*/
decoding_error:                                                                               /*######y80*/
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD, _handler_data);         /*######@7c*/

}                                                                                             /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_confirm, BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD,/*####%Bmrg*/
	bt_rpc_auth_cb_pairing_confirm_rpc_handler, NULL);                                                  /*#####@lvM*/

static void bt_rpc_auth_cb_pairing_complete_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bq5b*/
{                                                                                              /*#####@eyU*/

	struct bt_conn * conn;                                                                 /*####%AfUj*/
	bool bonded;                                                                           /*#####@bS0*/

	conn = decode_bt_conn(_value);                                                         /*####%Cges*/
	bonded = ser_decode_bool(_value);                                                      /*#####@CEo*/

	if (!ser_decoding_done_and_check(_value)) {                                            /*######%FE*/
		goto decoding_error;                                                           /*######QTM*/
	}                                                                                      /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->pairing_complete) {
		auth_cb->pairing_complete(conn, bonded);
	}

	ser_rsp_send_void();                                                                   /*##BEYGLxw*/

	return;                                                                                /*######%FR*/
decoding_error:                                                                                /*######q5b*/
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_COMPLETE_RPC_CMD, _handler_data);         /*######@Ro*/

}                                                                                              /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_complete, BT_RPC_AUTH_CB_PAIRING_COMPLETE_RPC_CMD,/*####%BmVx*/
	bt_rpc_auth_cb_pairing_complete_rpc_handler, NULL);                                                   /*#####@Roo*/

static void bt_rpc_auth_cb_pairing_failed_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bghd*/
{                                                                                            /*#####@t/Y*/

	struct bt_conn * conn;                                                               /*####%Aa+4*/
	enum bt_security_err reason;                                                         /*#####@qhQ*/

	conn = decode_bt_conn(_value);                                                       /*####%Cjju*/
	reason = (enum bt_security_err)ser_decode_uint(_value);                              /*#####@DUs*/

	if (!ser_decoding_done_and_check(_value)) {                                          /*######%FE*/
		goto decoding_error;                                                         /*######QTM*/
	}                                                                                    /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	if (auth_cb->pairing_failed) {
		auth_cb->pairing_failed(conn, reason);
	}

	ser_rsp_send_void();                                                                 /*##BEYGLxw*/

	return;                                                                              /*######%Ff*/
decoding_error:                                                                              /*######LLv*/
	report_decoding_error(BT_RPC_AUTH_CB_PAIRING_FAILED_RPC_CMD, _handler_data);         /*######@rY*/

}                                                                                            /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rpc_auth_cb_pairing_failed, BT_RPC_AUTH_CB_PAIRING_FAILED_RPC_CMD,/*####%BnJW*/
	bt_rpc_auth_cb_pairing_failed_rpc_handler, NULL);                                                 /*#####@Src*/

int bt_conn_auth_cb_register_on_remote(uint16_t flags)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                   /*######%AR*/
	int _result;                                                                    /*######RDP*/
	size_t _buffer_size_max = 3;                                                    /*######@sI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                     /*##AvrU03s*/

	ser_encode_uint(&_ctx.encoder, flags);                                          /*##A7dK8O0*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD,/*####%BGFD*/
		&_ctx, ser_rsp_simple_i32, &_result);                                   /*#####@6IM*/

	return _result;                                                                 /*##BX7TDLc*/
}

int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	int res;
	uint16_t flags = 0;

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	flags |= cb->pairing_accept ? FLAG_PAIRING_ACCEPT_PRESENT : 0;
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
	flags |= cb->passkey_display ? FLAG_PASSKEY_DISPLAY_PRESENT : 0;
	flags |= cb->passkey_entry ? FLAG_PASSKEY_ENTRY_PRESENT : 0;
	flags |= cb->passkey_confirm ? FLAG_PASSKEY_CONFIRM_PRESENT : 0;
	flags |= cb->oob_data_request ? FLAG_OOB_DATA_REQUEST_PRESENT : 0;
	flags |= cb->cancel ? FLAG_CANCEL_PRESENT : 0;
	flags |= cb->pairing_confirm ? FLAG_PAIRING_CONFIRM_PRESENT : 0;
	flags |= cb->pairing_complete ? FLAG_PAIRING_COMPLETE_PRESENT : 0;
	flags |= cb->pairing_failed ? FLAG_PAIRING_FAILED_PRESENT : 0;

	res = bt_conn_auth_cb_register_on_remote(flags);

	if (res == 0) {
		auth_cb = cb;
	}

	return res;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AZ*/
	int _result;                                                             /*######I55*/
	size_t _buffer_size_max = 8;                                             /*######@3E*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A/BJ*/
	ser_encode_uint(&_ctx.encoder, passkey);                                 /*#####@l9A*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD, /*####%BAJu*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@l28*/

	return _result;                                                          /*##BX7TDLc*/
}


int bt_conn_auth_cancel(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AR*/
	int _result;                                                             /*######RDP*/
	size_t _buffer_size_max = 3;                                             /*######@sI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_CANCEL_RPC_CMD,        /*####%BBXK*/
		&_ctx, ser_rsp_simple_i32, &_result);                            /*#####@hmc*/

	return _result;                                                          /*##BX7TDLc*/
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                             /*######%AR*/
	int _result;                                                              /*######RDP*/
	size_t _buffer_size_max = 3;                                              /*######@sI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                      /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD,/*####%BLYR*/
		&_ctx, ser_rsp_simple_i32, &_result);                             /*#####@2gs*/

	return _result;                                                           /*##BX7TDLc*/
}

int bt_conn_auth_pairing_confirm(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                             /*######%AR*/
	int _result;                                                              /*######RDP*/
	size_t _buffer_size_max = 3;                                              /*######@sI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                      /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD,/*####%BC5L*/
		&_ctx, ser_rsp_simple_i32, &_result);                             /*#####@y20*/

	return _result;                                                           /*##BX7TDLc*/
}

#else /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

bt_security_t bt_conn_get_security(struct bt_conn *conn)
{
	return BT_SECURITY_L1;
}

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */
