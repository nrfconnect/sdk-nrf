/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/conn.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);


#ifndef __GENERATOR
#define UNUSED __attribute__((unused)) /* TODO: Improve generator to avoid this workaround */
#else
#define UNUSED ;
#endif

#define SIZE_OF_FIELD(structure, field) (sizeof(((structure*)NULL)->field))

static void report_decoding_error(uint8_t cmd_evt_id, void* data) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}


UNUSED
void encode_bt_conn(CborEncoder *encoder,
		    const struct bt_conn *conn)
{
	if (CONFIG_BT_MAX_CONN > 1) {
		ser_encode_uint(encoder, (uint8_t)bt_conn_index((struct bt_conn *)conn));
	}
}

struct bt_conn *decode_bt_conn(CborValue *value)
{
	/* Making bt_conn_lookup_index() public will be better approach. */
	extern struct bt_conn *bt_conn_lookup_index(uint8_t index);

	struct bt_conn *conn;
	uint8_t index;

	if (CONFIG_BT_MAX_CONN > 1) {
		index = ser_decode_uint(value);
	} else {
		index = 0;
	}
	
	conn = bt_conn_lookup_index(index);

	if (conn == NULL) {
		LOG_ERR("Cannot find connection of specified index");
		ser_decoder_invalid(value, CborErrorIO);
	} else {
		/* It is safe to unref, because remote side must be holding
		 * at least one reference.
		 */
		bt_conn_unref(conn);
	}

	return conn;
}


static void bt_conn_remote_update_ref(struct bt_conn *conn, int8_t value)
{
	if (value < 0) {
		bt_conn_unref(conn);
	} else {
		bt_conn_ref(conn);
	}
}

static void bt_conn_remote_update_ref_rpc_handler(CborValue *_value, void *_handler_data)/*####%BhfQ*/
{                                                                                        /*#####@n9c*/

	struct bt_conn * conn;                                                           /*####%AT80*/
	int8_t value;                                                                    /*#####@czU*/

	conn = decode_bt_conn(_value);                                                   /*####%Cqo/*/
	value = ser_decode_int(_value);                                                  /*#####@SRE*/

	if (!ser_decoding_done_and_check(_value)) {                                      /*######%FE*/
		goto decoding_error;                                                     /*######QTM*/
	}                                                                                /*######@1Y*/

	bt_conn_remote_update_ref(conn, value);                                          /*##DkX8zrk*/

	ser_rsp_send_void();                                                             /*##BEYGLxw*/

	return;                                                                          /*######%Fb*/
decoding_error:                                                                          /*######UYw*/
	report_decoding_error(BT_CONN_REMOTE_UPDATE_REF_RPC_CMD, _handler_data);         /*######@jo*/

}                                                                                        /*##B9ELNqo*/


NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_remote_update_ref, BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,/*####%BpL0*/
	bt_conn_remote_update_ref_rpc_handler, NULL);                                             /*#####@VFU*/

static inline void bt_conn_foreach_cb_callback(struct bt_conn *conn, void *data,
					       uint32_t callback_slot)
{
	SERIALIZE(CALLBACK(bt_conn_foreach_cb));

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%ARzi*/
	size_t _buffer_size_max = 11;                                            /*#####@x8o*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*######%A6*/
	ser_encode_uint(&_ctx.encoder, (uintptr_t)data);                         /*######W+6*/
	ser_encode_callback_slot(&_ctx.encoder, callback_slot);                  /*######@Ww*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD,/*####%BKc8*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@HvE*/
}

CBKPROXY_HANDLER(bt_conn_foreach_cb_encoder, bt_conn_foreach_cb_callback,
	(struct bt_conn *conn, void *data), (conn, data));

static void bt_conn_foreach_rpc_handler(CborValue *_value, void *_handler_data)            /*####%Bixe*/
{                                                                                          /*#####@7pE*/

	int type;                                                                          /*######%AV*/
	bt_conn_foreach_cb func;                                                           /*######jq0*/
	void * data;                                                                       /*######@8U*/

	type = ser_decode_int(_value);                                                     /*######%Ch*/
	func = (bt_conn_foreach_cb)ser_decode_callback(_value, bt_conn_foreach_cb_encoder);/*######0/s*/
	data = (void *)ser_decode_uint(_value);                                            /*######@VI*/

	if (!ser_decoding_done_and_check(_value)) {                                        /*######%FE*/
		goto decoding_error;                                                       /*######QTM*/
	}                                                                                  /*######@1Y*/

	bt_conn_foreach(type, func, data);                                                 /*##Dkyjw2I*/

	ser_rsp_send_void();                                                               /*##BEYGLxw*/

	return;                                                                            /*######%Fd*/
decoding_error:                                                                            /*######JVK*/
	report_decoding_error(BT_CONN_FOREACH_RPC_CMD, _handler_data);                     /*######@qw*/

}                                                                                          /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_foreach, BT_CONN_FOREACH_RPC_CMD,   /*####%Bois*/
	bt_conn_foreach_rpc_handler, NULL);                                      /*#####@RjU*/


static void bt_conn_lookup_addr_le_rpc_handler(CborValue *_value, void *_handler_data)/*####%BsMb*/
{                                                                                     /*#####@ISo*/

	struct nrf_rpc_cbor_ctx _ctx;                                                 /*#######%A*/
	struct bt_conn *_result;                                                      /*#######eB*/
	uint8_t id;                                                                   /*#######P4*/
	bt_addr_le_t _peer_data;                                                      /*########N*/
	const bt_addr_le_t * peer;                                                    /*########I*/
	size_t _buffer_size_max = 3;                                                  /*########@*/

	id = ser_decode_uint(_value);                                                 /*####%CrMZ*/
	peer = ser_decode_buffer(_value, &_peer_data, sizeof(bt_addr_le_t));          /*#####@6MU*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	_result = bt_conn_lookup_addr_le(id, peer);                                   /*##Dp2yzPE*/

	{                                                                             /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                           /*#####@zxs*/

		encode_bt_conn(&_ctx.encoder, _result);                               /*##DIUWBhg*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                       /*####%BIlG*/
	}                                                                             /*#####@TnU*/

	return;                                                                       /*######%FU*/
decoding_error:                                                                       /*######Uys*/
	report_decoding_error(BT_CONN_LOOKUP_ADDR_LE_RPC_CMD, _handler_data);         /*######@WY*/

}                                                                                     /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_lookup_addr_le, BT_CONN_LOOKUP_ADDR_LE_RPC_CMD,/*####%BhIg*/
	bt_conn_lookup_addr_le_rpc_handler, NULL);                                          /*#####@Mg8*/

static void bt_conn_get_dst_out_rpc_handler(CborValue *_value, void *_handler_data)/*####%BvTa*/
{                                                                                  /*#####@6No*/

	struct nrf_rpc_cbor_ctx _ctx;                                              /*#######%A*/
	bool _result;                                                              /*#######W/*/
	const struct bt_conn * conn;                                               /*#######14*/
	bt_addr_le_t _dst_data;                                                    /*########l*/
	bt_addr_le_t * dst = &_dst_data;                                           /*########s*/
	size_t _buffer_size_max = 4;                                               /*########@*/

	conn = decode_bt_conn(_value);                                             /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);

	dst = (bt_addr_le_t *)bt_conn_get_dst(conn);
	_result = (dst != NULL);

	_buffer_size_max += sizeof(bt_addr_le_t);                                  /*##CN3eLBo*/

	{                                                                          /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                        /*#####@zxs*/

		ser_encode_bool(&_ctx.encoder, _result);                           /*####%DP/0*/
		ser_encode_buffer(&_ctx.encoder, dst, sizeof(bt_addr_le_t));       /*#####@DVQ*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                    /*####%BIlG*/
	}                                                                          /*#####@TnU*/

	return;                                                                    /*######%FV*/
decoding_error:                                                                    /*######XKY*/
	report_decoding_error(BT_CONN_GET_DST_OUT_RPC_CMD, _handler_data);         /*######@Kk*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_dst_out, BT_CONN_GET_DST_OUT_RPC_CMD,/*####%Bjcb*/
	bt_conn_get_dst_out_rpc_handler, NULL);                                       /*#####@7Qc*/

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

UNUSED
static const size_t bt_conn_le_phy_info_buf_size = 4;                            /*##BnxUetY*/


void bt_conn_le_phy_info_enc(CborEncoder *_encoder, const struct bt_conn_le_phy_info *_data)/*####%Bm4q*/
{                                                                                           /*#####@bAE*/

	SERIALIZE(STRUCT(struct bt_conn_le_phy_info));

	ser_encode_uint(_encoder, _data->tx_phy);                                           /*####%A6K0*/
	ser_encode_uint(_encoder, _data->rx_phy);                                           /*#####@V5A*/

}                                                                                           /*##B9ELNqo*/

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

UNUSED
static const size_t bt_conn_le_data_len_info_buf_size = 12;                      /*##BjXn4CA*/


void bt_conn_le_data_len_info_enc(CborEncoder *_encoder, const struct bt_conn_le_data_len_info *_data)/*####%Bhsk*/
{                                                                                                     /*#####@4Uo*/

	SERIALIZE(STRUCT(struct bt_conn_le_data_len_info));

	ser_encode_uint(_encoder, _data->tx_max_len);                                                 /*######%A8*/
	ser_encode_uint(_encoder, _data->tx_max_time);                                                /*#######U+*/
	ser_encode_uint(_encoder, _data->rx_max_len);                                                 /*#######ug*/
	ser_encode_uint(_encoder, _data->rx_max_time);                                                /*#######@I*/

}                                                                                                     /*##B9ELNqo*/

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

static const size_t bt_conn_info_buf_size = 
	1 + SIZE_OF_FIELD(struct bt_conn_info, type) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, role) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, id) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.interval) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.latency) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.timeout) +
	4 * (1 + sizeof(bt_addr_le_t)) + 
#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)
	bt_conn_le_phy_info_buf_size +
#else
	1 +
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)
	bt_conn_le_data_len_info_buf_size;
#else
	1;
#endif


void bt_conn_info_enc(CborEncoder* encoder, struct bt_conn_info *info)
{
	ser_encode_uint(encoder, info->type);
	ser_encode_uint(encoder, info->role);
	ser_encode_uint(encoder, info->id);

	if (info->type == BT_CONN_TYPE_LE) {
		ser_encode_uint(encoder, info->le.interval);
		ser_encode_uint(encoder, info->le.latency);
		ser_encode_uint(encoder, info->le.timeout);
		ser_encode_buffer(encoder, info->le.src, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.dst, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.local, sizeof(bt_addr_le_t));
		ser_encode_buffer(encoder, info->le.remote, sizeof(bt_addr_le_t));
#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)
		bt_conn_le_phy_info_enc(encoder, info->le.phy);
#else
		ser_encode_null(encoder);
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)
		bt_conn_le_data_len_info_enc(encoder, info->le.data_len);
#else
		ser_encode_null(encoder);
#endif
	} else {
		/* non-LE connection types are not supported. */
		ser_encoder_invalid(encoder);
	}
}


static void bt_conn_get_info_rpc_handler(CborValue *_value, void *_handler_data) /*####%BssD*/
{                                                                                /*#####@zpE*/

	struct nrf_rpc_cbor_ctx _ctx;                                            /*######%AY*/
	int _result;                                                             /*#######mt*/
	const struct bt_conn * conn;                                             /*#######MJ*/
	size_t _buffer_size_max = 5;                                             /*#######@M*/

	struct bt_conn_info info;

	conn = decode_bt_conn(_value);                                           /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	_result = bt_conn_get_info(conn, &info);

	_buffer_size_max += bt_conn_info_buf_size;

	{                                                                        /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                      /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                          /*##DIBqwtg*/

		bt_conn_info_enc(&_ctx.encoder, &info);

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                  /*####%BIlG*/
	}                                                                        /*#####@TnU*/

	return;                                                                  /*######%Ff*/
decoding_error:                                                                  /*######mLS*/
	report_decoding_error(BT_CONN_GET_INFO_RPC_CMD, _handler_data);          /*######@24*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_info, BT_CONN_GET_INFO_RPC_CMD, /*####%BruT*/
	bt_conn_get_info_rpc_handler, NULL);                                     /*#####@j6w*/


static const size_t bt_conn_remote_info_buf_size = 
	1 + SIZE_OF_FIELD(struct bt_conn_remote_info, type) +
	1 + SIZE_OF_FIELD(struct bt_conn_remote_info, version) +
	1 + SIZE_OF_FIELD(struct bt_conn_remote_info, manufacturer) +
	1 + SIZE_OF_FIELD(struct bt_conn_remote_info, subversion) +
	1 + 8 * sizeof(uint8_t);

void bt_conn_remote_info_enc(CborEncoder* encoder, struct bt_conn_remote_info *remote_info)
{
	ser_encode_uint(encoder, remote_info->type);
	ser_encode_uint(encoder, remote_info->version);
	ser_encode_uint(encoder, remote_info->manufacturer);
	ser_encode_uint(encoder, remote_info->subversion);

	if (remote_info->type == BT_CONN_TYPE_LE) {
		ser_encode_buffer(encoder, remote_info->le.features, 8 * sizeof(uint8_t));
	} else {
		/* non-LE connection types are not supported. */
		ser_encoder_invalid(encoder);
	}
}

static void bt_conn_get_remote_info_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bkh4*/
{                                                                                      /*#####@acA*/

	struct nrf_rpc_cbor_ctx _ctx;                                                  /*######%AT*/
	int _result;                                                                   /*#######3L*/
	struct bt_conn * conn;                                                         /*#######dZ*/
	size_t _buffer_size_max = 5;                                                   /*#######@g*/

	struct bt_conn_remote_info remote_info;

	conn = decode_bt_conn(_value);                                                 /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                    /*######%FE*/
		goto decoding_error;                                                   /*######QTM*/
	}                                                                              /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	_result = bt_conn_get_remote_info(conn, &remote_info);

	_buffer_size_max += bt_conn_remote_info_buf_size;

	{                                                                              /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                            /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                                /*##DIBqwtg*/

		bt_conn_remote_info_enc(&_ctx.encoder, &remote_info);

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                        /*####%BIlG*/
	}                                                                              /*#####@TnU*/

	return;                                                                        /*######%FV*/
decoding_error:                                                                        /*######MFs*/
	report_decoding_error(BT_CONN_GET_REMOTE_INFO_RPC_CMD, _handler_data);         /*######@iE*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_remote_info, BT_CONN_GET_REMOTE_INFO_RPC_CMD,/*####%BoP0*/
	bt_conn_get_remote_info_rpc_handler, NULL);                                           /*#####@ZY8*/


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

static void bt_conn_le_param_update_rpc_handler(CborValue *_value, void *_handler_data)/*####%BrcG*/
{                                                                                      /*#####@Iqc*/

	struct bt_conn * conn;                                                         /*######%Ac*/
	struct bt_le_conn_param param;                                                 /*######/0C*/
	int _result;                                                                   /*######@kE*/

	conn = decode_bt_conn(_value);                                                 /*####%Cun5*/
	bt_le_conn_param_dec(_value, &param);                                          /*#####@kEE*/

	if (!ser_decoding_done_and_check(_value)) {                                    /*######%FE*/
		goto decoding_error;                                                   /*######QTM*/
	}                                                                              /*######@1Y*/

	_result = bt_conn_le_param_update(conn, &param);                               /*##Dn9F2Lg*/

	ser_rsp_send_int(_result);                                                     /*##BPC96+4*/

	return;                                                                        /*######%FS*/
decoding_error:                                                                        /*######4Mq*/
	report_decoding_error(BT_CONN_LE_PARAM_UPDATE_RPC_CMD, _handler_data);         /*######@o8*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_param_update, BT_CONN_LE_PARAM_UPDATE_RPC_CMD,/*####%BnpE*/
	bt_conn_le_param_update_rpc_handler, NULL);                                           /*#####@lCw*/

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

void bt_conn_le_data_len_param_dec(CborValue *_value, struct bt_conn_le_data_len_param *_data)/*####%BsQM*/
{                                                                                             /*#####@jgQ*/

	_data->tx_max_len = ser_decode_uint(_value);                                          /*####%Ch5i*/
	_data->tx_max_time = ser_decode_uint(_value);                                         /*#####@mwA*/

}                                                                                             /*##B9ELNqo*/

static void bt_conn_le_data_len_update_rpc_handler(CborValue *_value, void *_handler_data)/*####%Brqk*/
{                                                                                         /*#####@0RU*/

	struct bt_conn * conn;                                                            /*######%Ae*/
	struct bt_conn_le_data_len_param param;                                           /*######+IU*/
	int _result;                                                                      /*######@1Y*/

	conn = decode_bt_conn(_value);                                                    /*####%Cvvo*/
	bt_conn_le_data_len_param_dec(_value, &param);                                    /*#####@4ds*/

	if (!ser_decoding_done_and_check(_value)) {                                       /*######%FE*/
		goto decoding_error;                                                      /*######QTM*/
	}                                                                                 /*######@1Y*/

	_result = bt_conn_le_data_len_update(conn, &param);                               /*##Dkni44o*/

	ser_rsp_send_int(_result);                                                        /*##BPC96+4*/

	return;                                                                           /*######%FX*/
decoding_error:                                                                           /*######281*/
	report_decoding_error(BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD, _handler_data);         /*######@x8*/

}                                                                                         /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_data_len_update, BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD,/*####%BiAv*/
	bt_conn_le_data_len_update_rpc_handler, NULL);                                              /*#####@J5g*/

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

void bt_conn_le_phy_param_dec(CborValue *_value, struct bt_conn_le_phy_param *_data)/*####%BseT*/
{                                                                                   /*#####@CYc*/

	_data->options = ser_decode_uint(_value);                                   /*######%Cl*/
	_data->pref_tx_phy = ser_decode_uint(_value);                               /*######PBB*/
	_data->pref_rx_phy = ser_decode_uint(_value);                               /*######@Y4*/

}                                                                                   /*##B9ELNqo*/

static void bt_conn_le_phy_update_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bsl8*/
{                                                                                    /*#####@xiY*/

	struct bt_conn * conn;                                                       /*######%Aa*/
	struct bt_conn_le_phy_param param;                                           /*######Cj6*/
	int _result;                                                                 /*######@1Y*/

	conn = decode_bt_conn(_value);                                               /*####%Ctyw*/
	bt_conn_le_phy_param_dec(_value, &param);                                    /*#####@dng*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	_result = bt_conn_le_phy_update(conn, &param);                               /*##DqiUE1k*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

	return;                                                                      /*######%FR*/
decoding_error:                                                                      /*######Q7+*/
	report_decoding_error(BT_CONN_LE_PHY_UPDATE_RPC_CMD, _handler_data);         /*######@50*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_phy_update, BT_CONN_LE_PHY_UPDATE_RPC_CMD,/*####%BhbB*/
	bt_conn_le_phy_update_rpc_handler, NULL);                                         /*#####@ByA*/

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

static void bt_conn_disconnect_rpc_handler(CborValue *_value, void *_handler_data)/*####%BjsE*/
{                                                                                 /*#####@Des*/

	struct bt_conn * conn;                                                    /*######%AU*/
	uint8_t reason;                                                           /*######ANd*/
	int _result;                                                              /*######@fI*/

	conn = decode_bt_conn(_value);                                            /*####%CoIh*/
	reason = ser_decode_uint(_value);                                         /*#####@ocQ*/

	if (!ser_decoding_done_and_check(_value)) {                               /*######%FE*/
		goto decoding_error;                                              /*######QTM*/
	}                                                                         /*######@1Y*/

	_result = bt_conn_disconnect(conn, reason);                               /*##Do2VejQ*/

	ser_rsp_send_int(_result);                                                /*##BPC96+4*/

	return;                                                                   /*######%FU*/
decoding_error:                                                                   /*######ptV*/
	report_decoding_error(BT_CONN_DISCONNECT_RPC_CMD, _handler_data);         /*######@Do*/

}                                                                                 /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_disconnect, BT_CONN_DISCONNECT_RPC_CMD,/*####%BgGh*/
	bt_conn_disconnect_rpc_handler, NULL);                                      /*#####@euA*/

#if defined(CONFIG_BT_CENTRAL) || defined(__GENERATOR)

void bt_conn_le_create_param_dec(CborValue *_value, struct bt_conn_le_create_param *_data)/*####%BmH3*/
{                                                                                         /*#####@pn0*/

	_data->options = ser_decode_uint(_value);                                         /*#######%C*/
	_data->interval = ser_decode_uint(_value);                                        /*#######qJ*/
	_data->window = ser_decode_uint(_value);                                          /*#######HP*/
	_data->interval_coded = ser_decode_uint(_value);                                  /*########V*/
	_data->window_coded = ser_decode_uint(_value);                                    /*########c*/
	_data->timeout = ser_decode_uint(_value);                                         /*########@*/

}                                                                                         /*##B9ELNqo*/

static void bt_conn_le_create_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bs/x*/
{                                                                                /*#####@bkg*/

	struct nrf_rpc_cbor_ctx _ctx;                                            /*########%*/
	int _result;                                                             /*########A*/
	bt_addr_le_t _peer_data;                                                 /*########R*/
	const bt_addr_le_t * peer;                                               /*########e*/
	struct bt_conn_le_create_param create_param;                             /*########C*/
	struct bt_le_conn_param conn_param;                                      /*########G*/
	struct bt_conn *_conn_data;                                              /*########w*/
	struct bt_conn ** conn = &_conn_data;                                    /*########c*/
	size_t _buffer_size_max = 8;                                             /*########@*/

	peer = ser_decode_buffer(_value, &_peer_data, sizeof(bt_addr_le_t));     /*######%Cp*/
	bt_conn_le_create_param_dec(_value, &create_param);                      /*######W7Q*/
	bt_le_conn_param_dec(_value, &conn_param);                               /*######@Gw*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_conn_le_create(peer, &create_param, &conn_param, conn);     /*##Dk7LlwE*/

	{                                                                        /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                      /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                          /*####%DIvK*/
		encode_bt_conn(&_ctx.encoder, *conn);                            /*#####@bmE*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                  /*####%BIlG*/
	}                                                                        /*#####@TnU*/

	return;                                                                  /*######%Fd*/
decoding_error:                                                                  /*######Nio*/
	report_decoding_error(BT_CONN_LE_CREATE_RPC_CMD, _handler_data);         /*######@FI*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_create, BT_CONN_LE_CREATE_RPC_CMD,/*####%BqOb*/
	bt_conn_le_create_rpc_handler, NULL);                                     /*#####@KWY*/

#if defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR)

static void bt_conn_le_create_auto_rpc_handler(CborValue *_value, void *_handler_data)/*####%BqAB*/
{                                                                                     /*#####@yng*/

	struct bt_conn_le_create_param create_param;                                  /*######%AR*/
	struct bt_le_conn_param conn_param;                                           /*######IuI*/
	int _result;                                                                  /*######@4Y*/

	bt_conn_le_create_param_dec(_value, &create_param);                           /*####%CpZB*/
	bt_le_conn_param_dec(_value, &conn_param);                                    /*#####@KFY*/

	if (!ser_decoding_done_and_check(_value)) {                                   /*######%FE*/
		goto decoding_error;                                                  /*######QTM*/
	}                                                                             /*######@1Y*/

	_result = bt_conn_le_create_auto(&create_param, &conn_param);                 /*##DjEcFjU*/

	ser_rsp_send_int(_result);                                                    /*##BPC96+4*/

	return;                                                                       /*######%FR*/
decoding_error:                                                                       /*######j2t*/
	report_decoding_error(BT_CONN_LE_CREATE_AUTO_RPC_CMD, _handler_data);         /*######@VU*/

}                                                                                     /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_le_create_auto, BT_CONN_LE_CREATE_AUTO_RPC_CMD,/*####%BgrF*/
	bt_conn_le_create_auto_rpc_handler, NULL);                                          /*#####@RNA*/

static void bt_conn_create_auto_stop_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bq0O*/
{                                                                                       /*#####@qPA*/

	int _result;                                                                    /*##AWc+iOc*/

	nrf_rpc_cbor_decoding_done(_value);                                             /*##FGkSPWY*/

	_result = bt_conn_create_auto_stop();                                           /*##Dnkc4D8*/

	ser_rsp_send_int(_result);                                                      /*##BPC96+4*/

}                                                                                       /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_create_auto_stop, BT_CONN_CREATE_AUTO_STOP_RPC_CMD,/*####%BvIi*/
	bt_conn_create_auto_stop_rpc_handler, NULL);                                            /*#####@7GA*/

#endif /* defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR) */

#if !defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR)

static void bt_le_set_auto_conn_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bqlk*/
{                                                                                  /*#####@bmQ*/

	bt_addr_le_t _addr_data;                                                   /*#######%A*/
	const bt_addr_le_t * addr;                                                 /*#######SX*/
	struct bt_le_conn_param _param_data;                                       /*#######S6*/
	struct bt_le_conn_param *param;                                            /*#######V4*/
	int _result;                                                               /*########@*/

	addr = ser_decode_buffer(_value, &_addr_data, sizeof(bt_addr_le_t));       /*#######%C*/
	if (ser_decode_is_null(_value)) {                                          /*########p*/
		param = NULL;                                                      /*########H*/
		ser_decode_skip(_value);                                           /*########l*/
	} else {                                                                   /*########M*/
		param = &_param_data;                                              /*########U*/
		bt_le_conn_param_dec(_value, param);                               /*########8*/
	}                                                                          /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_le_set_auto_conn(addr, param);                                /*##DlPwmug*/

	ser_rsp_send_int(_result);                                                 /*##BPC96+4*/

	return;                                                                    /*######%Fa*/
decoding_error:                                                                    /*######E2M*/
	report_decoding_error(BT_LE_SET_AUTO_CONN_RPC_CMD, _handler_data);         /*######@TQ*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_set_auto_conn, BT_LE_SET_AUTO_CONN_RPC_CMD,/*####%Bh4e*/
	bt_le_set_auto_conn_rpc_handler, NULL);                                       /*#####@GRg*/

#endif /* !defined(CONFIG_BT_WHITELIST) || defined(__GENERATOR) */

#endif /* defined(CONFIG_BT_CENTRAL) */

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

static void bt_conn_set_security_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bqj3*/
{                                                                                   /*#####@bwE*/

	struct bt_conn * conn;                                                      /*######%AQ*/
	bt_security_t sec;                                                          /*######+j6*/
	int _result;                                                                /*######@Ew*/

	conn = decode_bt_conn(_value);                                              /*####%ClQh*/
	sec = (bt_security_t)ser_decode_uint(_value);                               /*#####@Q4o*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	_result = bt_conn_set_security(conn, sec);                                  /*##Dkwq/74*/

	ser_rsp_send_int(_result);                                                  /*##BPC96+4*/

	return;                                                                     /*######%Fc*/
decoding_error:                                                                     /*######xvd*/
	report_decoding_error(BT_CONN_SET_SECURITY_RPC_CMD, _handler_data);         /*######@dI*/

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_set_security, BT_CONN_SET_SECURITY_RPC_CMD,/*####%BvSE*/
	bt_conn_set_security_rpc_handler, NULL);                                        /*#####@UNk*/

static void bt_conn_get_security_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bl9l*/
{                                                                                   /*#####@kys*/

	struct nrf_rpc_cbor_ctx _ctx;                                               /*######%AV*/
	bt_security_t _result;                                                      /*#######jU*/
	struct bt_conn * conn;                                                      /*#######Ps*/
	size_t _buffer_size_max = 5;                                                /*#######@Y*/

	conn = decode_bt_conn(_value);                                              /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	_result = bt_conn_get_security(conn);                                       /*##DmzRejc*/

	{                                                                           /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                         /*#####@zxs*/

		ser_encode_uint(&_ctx.encoder, (uint32_t)_result);                  /*##DHYvGBI*/

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                     /*####%BIlG*/
	}                                                                           /*#####@TnU*/

	return;                                                                     /*######%FR*/
decoding_error:                                                                     /*######kIW*/
	report_decoding_error(BT_CONN_GET_SECURITY_RPC_CMD, _handler_data);         /*######@a4*/

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_security, BT_CONN_GET_SECURITY_RPC_CMD,/*####%BhdT*/
	bt_conn_get_security_rpc_handler, NULL);                                        /*#####@rZw*/

static void bt_conn_enc_key_size_rpc_handler(CborValue *_value, void *_handler_data)/*####%BuVo*/
{                                                                                   /*#####@pUk*/

	struct bt_conn * conn;                                                      /*####%AW3r*/
	uint8_t _result;                                                            /*#####@KHs*/

	conn = decode_bt_conn(_value);                                              /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	_result = bt_conn_enc_key_size(conn);                                       /*##Dhu9g8M*/

	ser_rsp_send_uint(_result);                                                 /*##BJsBF7s*/

	return;                                                                     /*######%FT*/
decoding_error:                                                                     /*######h2o*/
	report_decoding_error(BT_CONN_ENC_KEY_SIZE_RPC_CMD, _handler_data);         /*######@Dk*/

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_enc_key_size, BT_CONN_ENC_KEY_SIZE_RPC_CMD,/*####%Bnf/*/
	bt_conn_enc_key_size_rpc_handler, NULL);                                        /*#####@k/c*/

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

static void bt_conn_cb_connected_call(struct bt_conn *conn, uint8_t err) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%Aefv*/
	size_t _buffer_size_max = 5;                                             /*#####@HWE*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*####%A98g*/
	ser_encode_uint(&_ctx.encoder, err);                                     /*#####@sYo*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_CONNECTED_CALL_RPC_CMD,  /*####%BJXq*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@Rxk*/
}

static void bt_conn_cb_disconnected_call(struct bt_conn *conn, uint8_t reason) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                             /*####%Aefv*/
	size_t _buffer_size_max = 5;                                              /*#####@HWE*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                      /*####%A+Lp*/
	ser_encode_uint(&_ctx.encoder, reason);                                   /*#####@OkE*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD,/*####%BFVb*/
		&_ctx, ser_rsp_simple_void, NULL);                                /*#####@DlY*/
}

static bool bt_conn_cb_le_param_req_call(struct bt_conn *conn, struct bt_le_conn_param *param) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                             /*######%AW*/
	bool _result;                                                             /*######94t*/
	size_t _buffer_size_max = 15;                                             /*######@/g*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                      /*####%AyYA*/
	bt_le_conn_param_enc(&_ctx.encoder, param);                               /*#####@Baw*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD,/*####%BB91*/
		&_ctx, ser_rsp_simple_bool, &_result);                            /*#####@JVs*/

	return _result;                                                           /*##BX7TDLc*/
}

static void bt_conn_cb_le_param_updated_call(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                 /*####%AYVb*/
	size_t _buffer_size_max = 12;                                                 /*#####@PYw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                   /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                          /*######%A8*/
	ser_encode_uint(&_ctx.encoder, interval);                                     /*#######aK*/
	ser_encode_uint(&_ctx.encoder, latency);                                      /*#######3v*/
	ser_encode_uint(&_ctx.encoder, timeout);                                      /*#######@w*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD,/*####%BLKv*/
		&_ctx, ser_rsp_simple_void, NULL);                                    /*#####@Odg*/
}

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

static void bt_conn_cb_identity_resolved_call(struct bt_conn *conn, const bt_addr_le_t *rpa, const bt_addr_le_t *identity) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                  /*####%AYVL*/
	size_t _buffer_size_max = 9;                                                   /*#####@xaA*/

	_buffer_size_max += rpa ? sizeof(bt_addr_le_t) : 0;                            /*####%CIHB*/
	_buffer_size_max += identity ? sizeof(bt_addr_le_t) : 0;                       /*#####@Jbw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                    /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                           /*######%Ax*/
	ser_encode_buffer(&_ctx.encoder, rpa, sizeof(bt_addr_le_t));                   /*######J7t*/
	ser_encode_buffer(&_ctx.encoder, identity, sizeof(bt_addr_le_t));              /*######@Rk*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD,/*####%BFcK*/
		&_ctx, ser_rsp_simple_void, NULL);                                     /*#####@zeA*/
}

static void bt_conn_cb_security_changed_call(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                 /*####%AYVk*/
	size_t _buffer_size_max = 13;                                                 /*#####@yks*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                   /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                          /*######%A3*/
	ser_encode_uint(&_ctx.encoder, (uint32_t)level);                              /*######9gj*/
	ser_encode_uint(&_ctx.encoder, (uint32_t)err);                                /*######@74*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD,/*####%BHJ7*/
		&_ctx, ser_rsp_simple_void, NULL);                                    /*#####@Se8*/
}

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */

#if defined(CONFIG_BT_REMOTE_INFO) || defined(__GENERATOR)

static void bt_conn_cb_remote_info_available_call(struct bt_conn *conn, struct bt_conn_remote_info *remote_info) {
	SERIALIZE(DEL(remote_info));

	struct nrf_rpc_cbor_ctx _ctx;                                                      /*####%ARJl*/
	size_t _buffer_size_max = 3;                                                       /*#####@MNU*/

	_buffer_size_max += bt_conn_remote_info_buf_size;

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                        /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                               /*##A0Ocmvc*/

	bt_conn_remote_info_enc(&_ctx.encoder, remote_info);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD,/*####%BP7E*/
		&_ctx, ser_rsp_simple_void, NULL);                                         /*#####@G54*/
}

#endif /* defined(CONFIG_BT_REMOTE_INFO) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR)

static void bt_conn_cb_le_phy_updated_call(struct bt_conn *conn, struct bt_conn_le_phy_info *param) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                               /*####%AT18*/
	size_t _buffer_size_max = 7;                                                /*#####@eJc*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                 /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                        /*####%A8tb*/
	bt_conn_le_phy_info_enc(&_ctx.encoder, param);                              /*#####@XQA*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD,/*####%BOp3*/
		&_ctx, ser_rsp_simple_void, NULL);                                  /*#####@TvM*/
}

#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) || defined(__GENERATOR) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR)

static void bt_conn_cb_le_data_len_updated_call(struct bt_conn *conn, struct bt_conn_le_data_len_info *info) {
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                    /*####%AbDw*/
	size_t _buffer_size_max = 15;                                                    /*#####@wIo*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                      /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                             /*####%A42B*/
	bt_conn_le_data_len_info_enc(&_ctx.encoder, info);                               /*#####@ieQ*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD,/*####%BIaV*/
		&_ctx, ser_rsp_simple_void, NULL);                                       /*#####@+lA*/
}

#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) || defined(__GENERATOR) */

static struct bt_conn_cb bt_conn_cb_register_data = {

	.connected = bt_conn_cb_connected_call,
	.disconnected = bt_conn_cb_disconnected_call,
	.le_param_req = bt_conn_cb_le_param_req_call,
	.le_param_updated = bt_conn_cb_le_param_updated_call,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = bt_conn_cb_identity_resolved_call,
	.security_changed = bt_conn_cb_security_changed_call,
#endif /* defined(CONFIG_BT_SMP) */
#if defined(CONFIG_BT_REMOTE_INFO)
	.remote_info_available = bt_conn_cb_remote_info_available_call,
#endif /* defined(CONFIG_BT_REMOTE_INFO) */
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = bt_conn_cb_le_phy_updated_call,
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = bt_conn_cb_le_data_len_updated_call,
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */
};

static void bt_conn_cb_register_on_remote_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bndp*/
{                                                                                            /*#####@dbA*/

	nrf_rpc_cbor_decoding_done(_value);                                                  /*##FGkSPWY*/

	SERIALIZE(CUSTOM_EXECUTE);
	bt_conn_cb_register(&bt_conn_cb_register_data);

	ser_rsp_send_void();                                                                 /*##BEYGLxw*/

}                                                                                            /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_cb_register_on_remote, BT_CONN_CB_REGISTER_ON_REMOTE_RPC_CMD,/*####%BviA*/
	bt_conn_cb_register_on_remote_rpc_handler, NULL);                                                 /*#####@Xeg*/

#if defined(CONFIG_BT_SMP) || defined(__GENERATOR)

static void bt_set_bondable_rpc_handler(CborValue *_value, void *_handler_data)  /*####%Bh+c*/
{                                                                                /*#####@F/g*/

	bool enable;                                                             /*##Acix4M4*/

	enable = ser_decode_bool(_value);                                        /*##CszoDnE*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	bt_set_bondable(enable);                                                 /*##Dkg63i4*/

	ser_rsp_send_void();                                                     /*##BEYGLxw*/

	return;                                                                  /*######%FU*/
decoding_error:                                                                  /*######12U*/
	report_decoding_error(BT_SET_BONDABLE_RPC_CMD, _handler_data);           /*######@U8*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_bondable, BT_SET_BONDABLE_RPC_CMD,   /*####%BhWw*/
	bt_set_bondable_rpc_handler, NULL);                                      /*#####@QqU*/


static void bt_set_oob_data_flag_rpc_handler(CborValue *_value, void *_handler_data)/*####%Blz3*/
{                                                                                   /*#####@ue4*/

	bool enable;                                                                /*##Acix4M4*/

	enable = ser_decode_bool(_value);                                           /*##CszoDnE*/

	if (!ser_decoding_done_and_check(_value)) {                                 /*######%FE*/
		goto decoding_error;                                                /*######QTM*/
	}                                                                           /*######@1Y*/

	bt_set_oob_data_flag(enable);                                               /*##DgijYC0*/

	ser_rsp_send_void();                                                        /*##BEYGLxw*/

	return;                                                                     /*######%FT*/
decoding_error:                                                                     /*######HxL*/
	report_decoding_error(BT_SET_OOB_DATA_FLAG_RPC_CMD, _handler_data);         /*######@Jg*/

}                                                                                   /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_set_oob_data_flag, BT_SET_OOB_DATA_FLAG_RPC_CMD,/*####%BvWO*/
	bt_set_oob_data_flag_rpc_handler, NULL);                                        /*#####@tW8*/

#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) || defined(__GENERATOR)

static void bt_le_oob_set_legacy_tk_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bmi1*/
{                                                                                      /*#####@L2A*/

	struct bt_conn * conn;                                                         /*######%AR*/
	const uint8_t * tk;                                                            /*#######Dn*/
	int _result;                                                                   /*#######aj*/
	struct ser_scratchpad _scratchpad;                                             /*#######@M*/

	SER_SCRATCHPAD_ALLOC(&_scratchpad, _value);                                    /*##EZKHjKY*/

	conn = decode_bt_conn(_value);                                                 /*####%Cpi5*/
	tk = ser_decode_buffer_sp(&_scratchpad);                                       /*#####@Piw*/

	if (!ser_decoding_done_and_check(_value)) {                                    /*######%FE*/
		goto decoding_error;                                                   /*######QTM*/
	}                                                                              /*######@1Y*/

	_result = bt_le_oob_set_legacy_tk(conn, tk);                                   /*##DmZv4nA*/

		SER_SCRATCHPAD_FREE(&_scratchpad);                                     /*##Eq1r7Tg*/

	ser_rsp_send_int(_result);                                                     /*##BPC96+4*/

	return;                                                                        /*######%Fb*/
decoding_error:                                                                        /*#######/p*/
	report_decoding_error(BT_LE_OOB_SET_LEGACY_TK_RPC_CMD, _handler_data);         /*#######6f*/
	SER_SCRATCHPAD_FREE(&_scratchpad);                                             /*#######@s*/

}                                                                                      /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_legacy_tk, BT_LE_OOB_SET_LEGACY_TK_RPC_CMD,/*####%BrP3*/
	bt_le_oob_set_legacy_tk_rpc_handler, NULL);                                           /*#####@U4k*/

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

static void bt_le_oob_set_sc_data_rpc_handler(CborValue *_value, void *_handler_data)/*####%BhKJ*/
{                                                                                    /*#####@ouU*/

	struct bt_conn * conn;                                                       /*#######%A*/
	struct bt_le_oob_sc_data _oobd_local_data;                                   /*#######RY*/
	struct bt_le_oob_sc_data *oobd_local;                                        /*#######aB*/
	struct bt_le_oob_sc_data _oobd_remote_data;                                  /*########Q*/
	struct bt_le_oob_sc_data *oobd_remote;                                       /*########A*/
	int _result;                                                                 /*########@*/

	conn = decode_bt_conn(_value);                                               /*########%*/
	if (ser_decode_is_null(_value)) {                                            /*########C*/
		oobd_local = NULL;                                                   /*########j*/
		ser_decode_skip(_value);                                             /*########P*/
	} else {                                                                     /*########9*/
		oobd_local = &_oobd_local_data;                                      /*########V*/
		bt_le_oob_sc_data_dec(_value, oobd_local);                           /*########9*/
	}                                                                            /*########o*/
	if (ser_decode_is_null(_value)) {                                            /*#########*/
		oobd_remote = NULL;                                                  /*#########*/
		ser_decode_skip(_value);                                             /*#########*/
	} else {                                                                     /*#########*/
		oobd_remote = &_oobd_remote_data;                                    /*#########*/
		bt_le_oob_sc_data_dec(_value, oobd_remote);                          /*#########*/
	}                                                                            /*########@*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	_result = bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);              /*##Dp8GSTY*/

	ser_rsp_send_int(_result);                                                   /*##BPC96+4*/

	return;                                                                      /*######%Fc*/
decoding_error:                                                                      /*######Mj6*/
	report_decoding_error(BT_LE_OOB_SET_SC_DATA_RPC_CMD, _handler_data);         /*######@DI*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_set_sc_data, BT_LE_OOB_SET_SC_DATA_RPC_CMD,/*####%BqMl*/
	bt_le_oob_set_sc_data_rpc_handler, NULL);                                         /*#####@mrg*/

static void bt_le_oob_get_sc_data_rpc_handler(CborValue *_value, void *_handler_data)/*####%BtKx*/
{                                                                                    /*#####@LeE*/

	struct nrf_rpc_cbor_ctx _ctx;                                                /*######%AT*/
	int _result;                                                                 /*#######3L*/
	struct bt_conn * conn;                                                       /*#######dZ*/
	size_t _buffer_size_max = 5;                                                 /*#######@g*/

	const struct bt_le_oob_sc_data *oobd_local;
	const struct bt_le_oob_sc_data *oobd_remote;

	conn = decode_bt_conn(_value);                                               /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%FE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	SERIALIZE(CUSTOM_EXECUTE);
	_result = bt_le_oob_get_sc_data(conn, &oobd_local, &oobd_remote);

	_buffer_size_max += bt_le_oob_sc_data_buf_size(oobd_local);
	_buffer_size_max += bt_le_oob_sc_data_buf_size(oobd_remote);

	{                                                                            /*####%AnG1*/
		NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                          /*#####@zxs*/

		ser_encode_int(&_ctx.encoder, _result);                              /*##DIBqwtg*/

		if (!oobd_local) {
			ser_encode_null(&_ctx.encoder);
		} else {
			bt_le_oob_sc_data_enc(&_ctx.encoder, oobd_local);
		}

		if (!oobd_remote) {
			ser_encode_null(&_ctx.encoder);
		} else {
			bt_le_oob_sc_data_enc(&_ctx.encoder, oobd_remote);
		}

		nrf_rpc_cbor_rsp_no_err(&_ctx);                                      /*####%BIlG*/
	}                                                                            /*#####@TnU*/

	return;                                                                      /*######%FQ*/
decoding_error:                                                                      /*######FDp*/
	report_decoding_error(BT_LE_OOB_GET_SC_DATA_RPC_CMD, _handler_data);         /*######@uM*/

}                                                                                    /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_le_oob_get_sc_data, BT_LE_OOB_GET_SC_DATA_RPC_CMD,/*####%Bms7*/
	bt_le_oob_get_sc_data_rpc_handler, NULL);                                         /*#####@Uzk*/

#endif /* defined(CONFIG_BT_SMP) && !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

#if defined(CONFIG_BT_FIXED_PASSKEY) || defined(__GENERATOR)

static void bt_passkey_set_rpc_handler(CborValue *_value, void *_handler_data)   /*####%BhQy*/
{                                                                                /*#####@/eo*/

	unsigned int passkey;                                                    /*####%ASJX*/
	int _result;                                                             /*#####@rCQ*/

	passkey = ser_decode_uint(_value);                                       /*##CvVxCrQ*/

	if (!ser_decoding_done_and_check(_value)) {                              /*######%FE*/
		goto decoding_error;                                             /*######QTM*/
	}                                                                        /*######@1Y*/

	_result = bt_passkey_set(passkey);                                       /*##DvwdhBU*/

	ser_rsp_send_int(_result);                                               /*##BPC96+4*/

	return;                                                                  /*######%Fb*/
decoding_error:                                                                  /*######nBu*/
	report_decoding_error(BT_PASSKEY_SET_RPC_CMD, _handler_data);            /*######@pk*/

}                                                                                /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_passkey_set, BT_PASSKEY_SET_RPC_CMD,     /*####%Bikf*/
	bt_passkey_set_rpc_handler, NULL);                                       /*#####@0sw*/

#endif /* defined(CONFIG_BT_FIXED_PASSKEY) || defined(__GENERATOR) */

static struct bt_conn_auth_cb remote_auth_cb;

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT) || defined(__GENERATOR)

static const size_t bt_conn_pairing_feat_buf_size = 12;                          /*##BgBlzt8*/

void bt_conn_pairing_feat_enc(CborEncoder *_encoder, const struct bt_conn_pairing_feat *_data)/*####%BoVW*/
{                                                                                             /*#####@Hzg*/

	SERIALIZE(STRUCT(struct bt_conn_pairing_feat));

	ser_encode_uint(_encoder, _data->io_capability);                                      /*#######%A*/
	ser_encode_uint(_encoder, _data->oob_data_flag);                                      /*#######wI*/
	ser_encode_uint(_encoder, _data->auth_req);                                           /*#######4R*/
	ser_encode_uint(_encoder, _data->max_enc_key_size);                                   /*########V*/
	ser_encode_uint(_encoder, _data->init_key_dist);                                      /*########0*/
	ser_encode_uint(_encoder, _data->resp_key_dist);                                      /*########@*/

}                                                                                             /*##B9ELNqo*/

struct bt_rpc_auth_cb_pairing_accept_rpc_res                                     /*####%BogB*/
{                                                                                /*#####@CDY*/

	enum bt_security_err _result;                                            /*##CRoC5JY*/

};                                                                               /*##B985gv0*/

static void bt_rpc_auth_cb_pairing_accept_rpc_rsp(CborValue *_value, void *_handler_data)/*####%BlLc*/
{                                                                                        /*#####@i9g*/

	struct bt_rpc_auth_cb_pairing_accept_rpc_res *_res =                             /*####%AT0F*/
		(struct bt_rpc_auth_cb_pairing_accept_rpc_res *)_handler_data;           /*#####@dcw*/

	_res->_result = (enum bt_security_err)ser_decode_uint(_value);                   /*##DZrShDE*/

}                                                                                        /*##B9ELNqo*/

enum bt_security_err bt_rpc_auth_cb_pairing_accept(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                              /*######%Af*/
	struct bt_rpc_auth_cb_pairing_accept_rpc_res _result;                      /*######EIV*/
	size_t _buffer_size_max = 15;                                              /*######@AE*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                       /*####%A4fs*/
	bt_conn_pairing_feat_enc(&_ctx.encoder, feat);                             /*#####@M3Y*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD,/*####%BGrI*/
		&_ctx, bt_rpc_auth_cb_pairing_accept_rpc_rsp, &_result);           /*#####@Jf0*/

	return _result._result;                                                    /*##BW0ge3U*/
}

#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT || defined(__GENERATOR) */

void bt_rpc_auth_cb_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                               /*####%AZLc*/
	size_t _buffer_size_max = 8;                                                /*#####@cfw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                 /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                        /*####%A/BJ*/
	ser_encode_uint(&_ctx.encoder, passkey);                                    /*#####@l9A*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD,/*####%BOHp*/
		&_ctx, ser_rsp_simple_void, NULL);                                  /*#####@q70*/
}

void bt_rpc_auth_cb_passkey_entry(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                             /*####%ARJl*/
	size_t _buffer_size_max = 3;                                              /*#####@MNU*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                               /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                      /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD,/*####%BD/j*/
		&_ctx, ser_rsp_simple_void, NULL);                                /*#####@j1Q*/
}

void bt_rpc_auth_cb_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                               /*####%AZLc*/
	size_t _buffer_size_max = 8;                                                /*#####@cfw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                 /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                        /*####%A/BJ*/
	ser_encode_uint(&_ctx.encoder, passkey);                                    /*#####@l9A*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD,/*####%BBrn*/
		&_ctx, ser_rsp_simple_void, NULL);                                  /*#####@i3s*/
}

void bt_rpc_auth_cb_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                /*####%AYi8*/
	size_t _buffer_size_max = 6;                                                 /*#####@1I4*/

	_buffer_size_max += info ? sizeof(struct bt_conn_oob_info) : 0;              /*##CMCJMWs*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                  /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                         /*####%A82b*/
	ser_encode_buffer(&_ctx.encoder, info, sizeof(struct bt_conn_oob_info));     /*#####@DcM*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD,/*####%BHFm*/
		&_ctx, ser_rsp_simple_void, NULL);                                   /*#####@ew8*/
}

void bt_rpc_auth_cb_cancel(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                            /*####%ARJl*/
	size_t _buffer_size_max = 3;                                             /*#####@MNU*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                              /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                     /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_CANCEL_RPC_CMD,      /*####%BPPO*/
		&_ctx, ser_rsp_simple_void, NULL);                               /*#####@ZYE*/
}

void bt_rpc_auth_cb_pairing_confirm(struct bt_conn *conn)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                               /*####%ARJl*/
	size_t _buffer_size_max = 3;                                                /*#####@MNU*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                 /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                        /*##A0Ocmvc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD,/*####%BAzO*/
		&_ctx, ser_rsp_simple_void, NULL);                                  /*#####@s+I*/
}

void bt_rpc_auth_cb_pairing_complete(struct bt_conn *conn, bool bonded)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                                /*####%AYNr*/
	size_t _buffer_size_max = 4;                                                 /*#####@YeI*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                  /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                         /*####%A/f8*/
	ser_encode_bool(&_ctx.encoder, bonded);                                      /*#####@QtE*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_COMPLETE_RPC_CMD,/*####%BN96*/
		&_ctx, ser_rsp_simple_void, NULL);                                   /*#####@0TI*/
}

void bt_rpc_auth_cb_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	SERIALIZE();

	struct nrf_rpc_cbor_ctx _ctx;                                              /*####%AZLc*/
	size_t _buffer_size_max = 8;                                               /*#####@cfw*/

	NRF_RPC_CBOR_ALLOC(_ctx, _buffer_size_max);                                /*##AvrU03s*/

	encode_bt_conn(&_ctx.encoder, conn);                                       /*####%A0wn*/
	ser_encode_uint(&_ctx.encoder, (uint32_t)reason);                          /*#####@8sc*/

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RPC_AUTH_CB_PAIRING_FAILED_RPC_CMD,/*####%BCVv*/
		&_ctx, ser_rsp_simple_void, NULL);                                 /*#####@9r8*/
}

static int bt_conn_auth_cb_register_on_remote(uint16_t flags)
{
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	remote_auth_cb.pairing_accept = (flags & FLAG_PAIRING_ACCEPT_PRESENT) ? bt_rpc_auth_cb_pairing_accept : NULL;
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
	remote_auth_cb.passkey_display = (flags & FLAG_PASSKEY_DISPLAY_PRESENT) ? bt_rpc_auth_cb_passkey_display : NULL;
	remote_auth_cb.passkey_entry = (flags & FLAG_PASSKEY_ENTRY_PRESENT) ? bt_rpc_auth_cb_passkey_entry : NULL;
	remote_auth_cb.passkey_confirm = (flags & FLAG_PASSKEY_CONFIRM_PRESENT) ? bt_rpc_auth_cb_passkey_confirm : NULL;
	remote_auth_cb.oob_data_request = (flags & FLAG_OOB_DATA_REQUEST_PRESENT) ? bt_rpc_auth_cb_oob_data_request : NULL;
	remote_auth_cb.cancel = (flags & FLAG_CANCEL_PRESENT) ? bt_rpc_auth_cb_cancel : NULL;
	remote_auth_cb.pairing_confirm = (flags & FLAG_PAIRING_CONFIRM_PRESENT) ? bt_rpc_auth_cb_pairing_confirm : NULL;
	remote_auth_cb.pairing_complete = (flags & FLAG_PAIRING_COMPLETE_PRESENT) ? bt_rpc_auth_cb_pairing_complete : NULL;
	remote_auth_cb.pairing_failed = (flags & FLAG_PAIRING_FAILED_PRESENT) ? bt_rpc_auth_cb_pairing_failed : NULL;

	return bt_conn_auth_cb_register(&remote_auth_cb);
}

static void bt_conn_auth_cb_register_on_remote_rpc_handler(CborValue *_value, void *_handler_data)/*####%BgwL*/
{                                                                                                 /*#####@xQU*/

	uint16_t flags;                                                                           /*####%Add7*/
	int _result;                                                                              /*#####@s0U*/

	flags = ser_decode_uint(_value);                                                          /*##ChLklnM*/

	if (!ser_decoding_done_and_check(_value)) {                                               /*######%FE*/
		goto decoding_error;                                                              /*######QTM*/
	}                                                                                         /*######@1Y*/

	_result = bt_conn_auth_cb_register_on_remote(flags);                                      /*##Dm+pd7Y*/

	ser_rsp_send_int(_result);                                                                /*##BPC96+4*/

	return;                                                                                   /*######%FW*/
decoding_error:                                                                                   /*######ose*/
	report_decoding_error(BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD, _handler_data);         /*######@q8*/

}                                                                                                 /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_cb_register_on_remote, BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD,/*####%Bv1D*/
	bt_conn_auth_cb_register_on_remote_rpc_handler, NULL);                                                      /*#####@ltE*/

static void bt_conn_auth_passkey_entry_rpc_handler(CborValue *_value, void *_handler_data)/*####%Btp1*/
{                                                                                         /*#####@zxI*/

	struct bt_conn * conn;                                                            /*######%AZ*/
	unsigned int passkey;                                                             /*######ZXN*/
	int _result;                                                                      /*######@bA*/

	conn = decode_bt_conn(_value);                                                    /*####%CuFE*/
	passkey = ser_decode_uint(_value);                                                /*#####@Qoc*/

	if (!ser_decoding_done_and_check(_value)) {                                       /*######%FE*/
		goto decoding_error;                                                      /*######QTM*/
	}                                                                                 /*######@1Y*/

	_result = bt_conn_auth_passkey_entry(conn, passkey);                              /*##Dv/YyEs*/

	ser_rsp_send_int(_result);                                                        /*##BPC96+4*/

	return;                                                                           /*######%FV*/
decoding_error:                                                                           /*######OrX*/
	report_decoding_error(BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD, _handler_data);         /*######@54*/

}                                                                                         /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_passkey_entry, BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD,/*####%BgTg*/
	bt_conn_auth_passkey_entry_rpc_handler, NULL);                                              /*#####@Tp0*/

static void bt_conn_auth_cancel_rpc_handler(CborValue *_value, void *_handler_data)/*####%BoOq*/
{                                                                                  /*#####@8ko*/

	struct bt_conn * conn;                                                     /*####%AXbT*/
	int _result;                                                               /*#####@thA*/

	conn = decode_bt_conn(_value);                                             /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                /*######%FE*/
		goto decoding_error;                                               /*######QTM*/
	}                                                                          /*######@1Y*/

	_result = bt_conn_auth_cancel(conn);                                       /*##DvC3bvM*/

	ser_rsp_send_int(_result);                                                 /*##BPC96+4*/

	return;                                                                    /*######%FR*/
decoding_error:                                                                    /*######Die*/
	report_decoding_error(BT_CONN_AUTH_CANCEL_RPC_CMD, _handler_data);         /*######@6o*/

}                                                                                  /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_cancel, BT_CONN_AUTH_CANCEL_RPC_CMD,/*####%Bszd*/
	bt_conn_auth_cancel_rpc_handler, NULL);                                       /*#####@WsM*/

static void bt_conn_auth_passkey_confirm_rpc_handler(CborValue *_value, void *_handler_data)/*####%Bi8W*/
{                                                                                           /*#####@7uo*/

	struct bt_conn * conn;                                                              /*####%AXbT*/
	int _result;                                                                        /*#####@thA*/

	conn = decode_bt_conn(_value);                                                      /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	_result = bt_conn_auth_passkey_confirm(conn);                                       /*##DjJbpDI*/

	ser_rsp_send_int(_result);                                                          /*##BPC96+4*/

	return;                                                                             /*######%FT*/
decoding_error:                                                                             /*######jKc*/
	report_decoding_error(BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD, _handler_data);         /*######@kU*/

}                                                                                           /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_passkey_confirm, BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD,/*####%BgAl*/
	bt_conn_auth_passkey_confirm_rpc_handler, NULL);                                                /*#####@ea4*/

static void bt_conn_auth_pairing_confirm_rpc_handler(CborValue *_value, void *_handler_data)/*####%BmTk*/
{                                                                                           /*#####@eD0*/

	struct bt_conn * conn;                                                              /*####%AXbT*/
	int _result;                                                                        /*#####@thA*/

	conn = decode_bt_conn(_value);                                                      /*##CgkztUA*/

	if (!ser_decoding_done_and_check(_value)) {                                         /*######%FE*/
		goto decoding_error;                                                        /*######QTM*/
	}                                                                                   /*######@1Y*/

	_result = bt_conn_auth_pairing_confirm(conn);                                       /*##DvN7P3s*/

	ser_rsp_send_int(_result);                                                          /*##BPC96+4*/

	return;                                                                             /*######%Fa*/
decoding_error:                                                                             /*######5Qw*/
	report_decoding_error(BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD, _handler_data);         /*######@SA*/

}                                                                                           /*##B9ELNqo*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_auth_pairing_confirm, BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD,/*####%BijI*/
	bt_conn_auth_pairing_confirm_rpc_handler, NULL);                                                /*#####@VU4*/

#endif /* defined(CONFIG_BT_SMP) || defined(__GENERATOR) */
