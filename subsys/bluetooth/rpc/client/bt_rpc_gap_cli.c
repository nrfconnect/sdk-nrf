
/* Client side of bluetooth API over nRF RPC.
 */

#include "nrf_rpc_cbor.h"

#include "bluetooth/bluetooth.h"

#include "bt_rpc_common.h"
#include "serialize.h"


void report_decoding_error(uint8_t cmd_evt_id, void* DATA) {
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

static void bt_ready_cb_t_encoder_rpc_handler(CborValue *_value, void *_handler_data)/*####%BmU8*/
{                                                                                    /*#####@JaA*/

	bt_ready_cb_t callback_slot;                                                 /*####%AVZM*/
	int err;                                                                     /*#####@ksU*/

	callback_slot = (bt_ready_cb_t)ser_decode_callback_slot(_value);             /*####%ClOC*/
	err = ser_decode_int(_value);                                                /*#####@KRE*/

	if (!ser_decoding_done_and_check(_value)) {                                  /*######%AE*/
		goto decoding_error;                                                 /*######QTM*/
	}                                                                            /*######@1Y*/

	callback_slot(err);                                                          /*##DnrVhJE*/

	return;                                                                      /*######%Bz*/
decoding_error:                                                                      /*#######0D*/
	report_decoding_error(BT_READY_CB_T_ENCODER_RPC_EVT, _handler_data);         /*#######ax*/
}                                                                                    /*#######@c*/


NRF_RPC_CBOR_EVT_DECODER(bt_rpc_grp, bt_ready_cb_t_encoder, BT_READY_CB_T_ENCODER_RPC_EVT,/*####%BvSP*/
	bt_ready_cb_t_encoder_rpc_handler, NULL);                                         /*#####@hs8*/
