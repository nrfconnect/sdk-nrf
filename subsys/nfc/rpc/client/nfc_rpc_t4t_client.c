/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nfc_rpc_ids.h>
#include <nfc_rpc_common.h>

#include <nfc_t4t_lib.h>

static void nfc_rpc_t4t_cb_rpc_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nfc_t4t_event_t event;
	const uint8_t *data;
	uint8_t *container = NULL;
	size_t data_length;
	uint32_t flags;
	void *context;
	nfc_t4t_callback_t callback;

	context = (void *)nrf_rpc_decode_uint(ctx);
	event = nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &data_length);
	flags = nrf_rpc_decode_uint(ctx);
	callback = (nfc_t4t_callback_t)nrf_rpc_decode_callback_call(ctx);

	if (data && data_length) {
		container = k_malloc(data_length);
	}

	if (container) {
		memcpy(container, data, data_length);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_CB);
		k_free(container);
		return;
	}

	if (callback) {
		callback(context, event, data ? container : NULL, data ? data_length : 0, flags);
	}
	k_free(container);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_cb, NFC_RPC_CMD_T4T_CB, nfc_rpc_t4t_cb_rpc_handler,
			 NULL);

int nfc_t4t_setup(nfc_t4t_callback_t callback, void *context)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	int error;

	cbor_buffer_size += sizeof(uintptr_t) + 1; /* context */
	cbor_buffer_size += sizeof(uint32_t) + 1;  /* callback */

	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_callback(&ctx, callback);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);

	nrf_rpc_cbor_cmd_no_err(&nfc_group, NFC_RPC_CMD_T4T_SETUP, &ctx, nfc_rpc_decode_error,
				&error);

	return error;
}

static int send_payload(enum nfc_rpc_cmd_server cmd, const uint8_t *data, size_t data_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size;
	int error;

	cbor_buffer_size = data_length + 3;
	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_buffer(&ctx, data, data_length);
	nrf_rpc_cbor_cmd_no_err(&nfc_group, cmd, &ctx, nfc_rpc_decode_error, &error);

	return error;
}

int nfc_t4t_ndef_rwpayload_set(uint8_t *emulation_buffer, size_t buffer_length)
{
	return send_payload(NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET, emulation_buffer, buffer_length);
}

int nfc_t4t_ndef_staticpayload_set(const uint8_t *emulation_buffer, size_t buffer_length)
{
	return send_payload(NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET, emulation_buffer,
			    buffer_length);
}

int nfc_t4t_response_pdu_send(const uint8_t *pdu, size_t pdu_length)
{
	return send_payload(NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND, pdu, pdu_length);
}

int nfc_t4t_parameter_set(nfc_t4t_param_id_t id, void *data, size_t data_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 1; /* nfc_t4t_param_id_t */
	int error;

	if (data == NULL) {
		return -NRF_EINVAL;
	}

	cbor_buffer_size += data_length + 2;

	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&ctx, id);
	nrf_rpc_encode_buffer(&ctx, data, data_length);

	nrf_rpc_cbor_cmd_no_err(&nfc_group, NFC_RPC_CMD_T4T_PARAMETER_SET, &ctx,
				nfc_rpc_decode_error, &error);

	return error;
}

int nfc_t4t_parameter_get(nfc_t4t_param_id_t id, void *data, size_t *max_data_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct nfc_rpc_param_getter getter = {data, max_data_length};
	size_t cbor_buffer_size = 2 + sizeof(nfc_t4t_param_id_t) + sizeof(size_t);

	if (data == NULL || max_data_length == NULL) {
		return -NRF_EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&ctx, id);
	nrf_rpc_encode_uint(&ctx, *max_data_length);

	nrf_rpc_cbor_cmd_no_err(&nfc_group, NFC_RPC_CMD_T4T_PARAMETER_GET, &ctx,
				nfc_rpc_decode_parameter, &getter);

	return getter.data ? 0 : -NRF_EINVAL;
}

static int send_cmd(enum nfc_rpc_cmd_server cmd)
{
	struct nrf_rpc_cbor_ctx ctx;
	int error;

	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&nfc_group, cmd, &ctx, nfc_rpc_decode_error, &error);

	return error;
}

int nfc_t4t_emulation_start(void)
{
	return send_cmd(NFC_RPC_CMD_T4T_EMULATION_START);
}

int nfc_t4t_emulation_stop(void)
{
	return send_cmd(NFC_RPC_CMD_T4T_EMULATION_STOP);
}

int nfc_t4t_done(void)
{
	return send_cmd(NFC_RPC_CMD_T4T_DONE);
}
