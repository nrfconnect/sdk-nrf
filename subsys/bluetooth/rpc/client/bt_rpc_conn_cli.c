
#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/conn.h"

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

SERIALIZE(GROUP(bt_rpc_grp));
// TODO: Create CUSTOM_STRUCT() annotation that can contain template for any block, including conn_unref after execution and decoding error.
SERIALIZE(RAW_STRUCT(bt_addr_le_t));
SERIALIZE(FILTERED_STRUCT(struct bt_conn, 3, encode_bt_conn, decode_bt_conn));

#define LOCK_CONN_INFO() k_mutex_lock(&bt_rpc_conn_mutex, K_FOREVER)
#define UNLOCK_CONN_INFO() k_mutex_unlock(&bt_rpc_conn_mutex)

#ifndef NRF_RPC_GENERATOR
#define UNUSED __attribute__((unused))
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
};

static struct bt_conn connections[CONFIG_BT_MAX_CONN];


static inline uint8_t get_conn_index(const struct bt_conn *conn)
{
	return (uint8_t)(conn - connections);
}


static inline void encode_bt_conn(CborEncoder *encoder,
				  const struct bt_conn *conn)
{
	if (CONFIG_BT_MAX_CONN > 1) {
		ser_encode_uint(encoder, get_conn_index(conn));
	}
}

static struct bt_conn *decode_bt_conn(CborValue *value)
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


struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_inc(&conn->ref);

	if (old == 0) {
		bt_conn_remote_update_ref(conn, +1);
	}

	return conn;
}

UNUSED
static void bt_conn_ref_set(struct bt_conn *conn, uint32_t value)
{
	atomic_set(&conn->ref, (atomic_val_t)value);
}



void bt_conn_unref(struct bt_conn *conn)
{
	atomic_val_t old = atomic_dec(&conn->ref);

	if (old == 1) {
		bt_conn_remote_update_ref(conn, -1);
	}
}


uint8_t bt_conn_index(struct bt_conn *conn)
{
	return get_conn_index(conn);
}


void bt_conn_le_phy_info_dec(CborValue *_value, struct bt_conn_le_phy_info *_data)/*####%Boer*/
{                                                                                 /*#####@VUM*/

	_data->tx_phy = ser_decode_uint(_value);                                  /*####%CkF8*/
	_data->rx_phy = ser_decode_uint(_value);                                  /*#####@sAY*/

}                                                                                 /*##B9ELNqo*/

void bt_conn_le_data_len_info_dec(CborValue *_value, struct bt_conn_le_data_len_info *_data)/*####%BjZZ*/
{                                                                                           /*#####@5uc*/

	_data->tx_max_len = ser_decode_uint(_value);                                        /*######%Cu*/
	_data->tx_max_time = ser_decode_uint(_value);                                       /*#######xr*/
	_data->rx_max_len = ser_decode_uint(_value);                                        /*#######yN*/
	_data->rx_max_time = ser_decode_uint(_value);                                       /*#######@Y*/

}                                                                                           /*##B9ELNqo*/



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

	if (remote_info->type == BT_CONN_TYPE_LE) {
		LOCK_CONN_INFO();
		remote_info->le.features = ser_decode_buffer(value, &conn->features, sizeof(bt_addr_le_t));
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

	bt_conn_ref(*(_res->conn));

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
