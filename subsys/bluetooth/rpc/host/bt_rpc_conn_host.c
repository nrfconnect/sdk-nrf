
#include "zephyr.h"

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/conn.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"


#define SIZE_OF_FIELD(structure, field) (sizeof(((structure*)NULL)->field))

static void report_decoding_error(uint8_t cmd_evt_id, void* DATA) {
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}



struct bt_conn *decode_bt_conn(CborValue *value)
{
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
		// TODO: Log error
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

	return;                                                                          /*######%B8*/
decoding_error:                                                                          /*#######bJ*/
	report_decoding_error(BT_CONN_REMOTE_UPDATE_REF_RPC_CMD, _handler_data);         /*#######x0*/
}                                                                                        /*#######@E*/


NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_remote_update_ref, BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,/*####%BpL0*/
	bt_conn_remote_update_ref_rpc_handler, NULL);                                             /*#####@VFU*/


void bt_conn_le_phy_info_enc(CborEncoder *_encoder, const struct bt_conn_le_phy_info *_data)/*####%Bm4q*/
{                                                                                           /*#####@bAE*/

	SERIALIZE(STRUCT(struct bt_conn_le_phy_info));

	ser_encode_uint(_encoder, _data->tx_phy);                                           /*####%A6K0*/
	ser_encode_uint(_encoder, _data->rx_phy);                                           /*#####@V5A*/

}                                                                                           /*##B9ELNqo*/

void bt_conn_le_data_len_info_enc(CborEncoder *_encoder, const struct bt_conn_le_data_len_info *_data)/*####%Bhsk*/
{                                                                                                     /*#####@4Uo*/

	SERIALIZE(STRUCT(struct bt_conn_le_data_len_info));

	ser_encode_uint(_encoder, _data->tx_max_len);                                                 /*######%A8*/
	ser_encode_uint(_encoder, _data->tx_max_time);                                                /*#######U+*/
	ser_encode_uint(_encoder, _data->rx_max_len);                                                 /*#######ug*/
	ser_encode_uint(_encoder, _data->rx_max_time);                                                /*#######@I*/

}                                                                                                     /*##B9ELNqo*/

static const size_t bt_conn_info_buf_size = 
	1 + SIZE_OF_FIELD(struct bt_conn_info, type) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, role) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, id) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.interval) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.latency) +
	1 + SIZE_OF_FIELD(struct bt_conn_info, le.timeout) +
	4 * (1 + sizeof(bt_addr_le_t)) + 
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	bt_conn_le_phy_info_buf_size +
#else
	1 +
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
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
#if defined(CONFIG_BT_USER_PHY_UPDATE)
		bt_conn_le_phy_info_enc(encoder, info->le.phy);
#else
		ser_encode_null(encoder);
#endif
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		bt_conn_le_data_len_info_enc(encoder, info->le.data_len)
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

	return;                                                                  /*######%B2*/
decoding_error:                                                                  /*#######/F*/
	report_decoding_error(BT_CONN_GET_INFO_RPC_CMD, _handler_data);          /*#######Ho*/
}                                                                                /*#######@E*/

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

	return;                                                                        /*######%B3*/
decoding_error:                                                                        /*#######YW*/
	report_decoding_error(BT_CONN_GET_REMOTE_INFO_RPC_CMD, _handler_data);         /*#######7u*/
}                                                                                      /*#######@M*/

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_conn_get_remote_info, BT_CONN_GET_REMOTE_INFO_RPC_CMD,/*####%BoP0*/
	bt_conn_get_remote_info_rpc_handler, NULL);                                           /*#####@ZY8*/

SERIALIZE(STRUCT_BUFFER_CONST(bt_conn_le_phy_info, 4));                          /*##Brou7OA*/

SERIALIZE(STRUCT_BUFFER_CONST(bt_conn_le_data_len_info, 12));                    /*##Bol6Rn4*/

static const bt_conn_le_phy_info_buf_init = 4;

static const bt_conn_le_data_len_info_buf_init = 12;
