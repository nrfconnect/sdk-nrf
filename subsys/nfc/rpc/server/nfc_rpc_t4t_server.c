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

static uint8_t ndef_buffer[CONFIG_NDEF_FILE_SIZE];

static void nfc_t4t_cb(void *context, nfc_t4t_event_t event, const uint8_t *data,
		       size_t data_length, uint32_t flags, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = data ? data_length + 2 : 1;

	cbor_buffer_size += 25; /* for event, flags, context and callback slot */
	NRF_RPC_CBOR_ALLOC(&nfc_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, event);
	nrf_rpc_encode_buffer(&ctx, data, data_length);
	nrf_rpc_encode_uint(&ctx, flags);
	nrf_rpc_encode_uint(&ctx, callback_slot);
	nrf_rpc_cbor_cmd_no_err(&nfc_group, NFC_RPC_CMD_T4T_CB, &ctx, nrf_rpc_rsp_decode_void,
				NULL);
}

NRF_RPC_CBKPROXY_HANDLER(nfc_t4t_cb_encoder, nfc_t4t_cb,
			 (void *context, nfc_t4t_event_t event, const uint8_t *data,
			  size_t data_length, uint32_t flags),
			 (context, event, data, data_length, flags));

static void nfc_rpc_t4t_setup_rpc_handler(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nfc_t4t_callback_t cb;
	void *cb_ctx;
	int error;

	cb = (nfc_t4t_callback_t)nrf_rpc_decode_callbackd(ctx, nfc_t4t_cb_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_SETUP);
		return;
	}

	error = nfc_t4t_setup(cb, cb_ctx);

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_setup, NFC_RPC_CMD_T4T_SETUP,
			 nfc_rpc_t4t_setup_rpc_handler, NULL);

static void nfc_rpc_t4t_ndef_rwpayload_set_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	const uint8_t *emulation_buffer;
	size_t buffer_length;
	int error;

	emulation_buffer = nrf_rpc_decode_buffer_ptr_and_size(ctx, &buffer_length);

	if (emulation_buffer && buffer_length) {
		memcpy(ndef_buffer, emulation_buffer, buffer_length);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET);
		return;
	}

	error = nfc_t4t_ndef_rwpayload_set(emulation_buffer ? ndef_buffer : NULL, buffer_length);
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_ndef_rwpayload_set,
			 NFC_RPC_CMD_T4T_NDEF_RWPAYLOAD_SET,
			 nfc_rpc_t4t_ndef_rwpayload_set_rpc_handler, NULL);

static void nfc_rpc_t4t_ndef_staticpayload_set_rpc_handler(const struct nrf_rpc_group *group,
							   struct nrf_rpc_cbor_ctx *ctx,
							   void *handler_data)
{
	const uint8_t *emulation_buffer;
	size_t buffer_length;
	int error;

	emulation_buffer = nrf_rpc_decode_buffer_ptr_and_size(ctx, &buffer_length);

	if (emulation_buffer && buffer_length) {
		memcpy(ndef_buffer, emulation_buffer, buffer_length);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET);
		return;
	}

	error = nfc_t4t_ndef_staticpayload_set(emulation_buffer ? ndef_buffer : NULL,
					       buffer_length);
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_ndef_staticpayload_set,
			 NFC_RPC_CMD_T4T_NDEF_STATICPAYLOAD_SET,
			 nfc_rpc_t4t_ndef_staticpayload_set_rpc_handler, NULL);

static void nfc_rpc_t4t_response_pdu_send_rpc_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	const uint8_t *pdu;
	void *container = NULL;
	size_t pdu_length;
	int error;

	pdu = nrf_rpc_decode_buffer_ptr_and_size(ctx, &pdu_length);

	if (pdu && pdu_length) {
		container = k_malloc(pdu_length);
	}

	if (container) {
		memcpy(container, pdu, pdu_length);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND);
		k_free(container);
		return;
	}

	if (pdu == NULL) {
		error = nfc_t4t_response_pdu_send(NULL, pdu_length);
	} else if (container) {
		error = nfc_t4t_response_pdu_send(container, pdu_length);
		k_free(container);
	} else {
		error = -NRF_EINVAL;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_response_pdu_send,
			 NFC_RPC_CMD_T4T_RESPONSE_PDU_SEND,
			 nfc_rpc_t4t_response_pdu_send_rpc_handler, NULL);

static void nfc_rpc_t4t_parameter_set_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nfc_t4t_param_id_t id;
	void *data;
	void *container = NULL;
	size_t data_length;
	int error = -NRF_EINVAL;

	id = nrf_rpc_decode_uint(ctx);
	data = (void *)nrf_rpc_decode_buffer_ptr_and_size(ctx, &data_length);

	if (data && data_length) {
		container = k_malloc(data_length);
	}

	if (container) {
		memcpy(container, data, data_length);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_PARAMETER_SET);
		k_free(container);
		return;
	}

	if (container) {
		error = nfc_t4t_parameter_set(id, container, data_length);
		k_free(container);
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_parameter_set, NFC_RPC_CMD_T4T_PARAMETER_SET,
			 nfc_rpc_t4t_parameter_set_rpc_handler, NULL);

static void nfc_rpc_t4t_parameter_get_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx local_ctx;
	nfc_t4t_param_id_t id;
	size_t max_data_length;
	uint8_t *container = NULL;
	int error = -NRF_EINVAL;

	id = nrf_rpc_decode_uint(ctx);
	max_data_length = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_PARAMETER_GET);
		return;
	}

	if (max_data_length) {
		container = k_malloc(max_data_length);
	}

	if (container) {
		error = nfc_t4t_parameter_get(id, container, &max_data_length);
	}

	NRF_RPC_CBOR_ALLOC(&nfc_group, local_ctx, error ? 1 : max_data_length + 2);
	nrf_rpc_encode_buffer(&local_ctx, error ? NULL : container, error ? 0 : max_data_length);
	nrf_rpc_cbor_rsp_no_err(&nfc_group, &local_ctx);
	k_free(container);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_parameter_get, NFC_RPC_CMD_T4T_PARAMETER_GET,
			 nfc_rpc_t4t_parameter_get_rpc_handler, NULL);

static void nfc_rpc_t4t_emulation_start_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	int error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_EMULATION_START);
		return;
	}

	error = nfc_t4t_emulation_start();
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_emulation_start, NFC_RPC_CMD_T4T_EMULATION_START,
			 nfc_rpc_t4t_emulation_start_rpc_handler, NULL);

static void nfc_rpc_t4t_emulation_stop_rpc_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_EMULATION_STOP);
		return;
	}

	error = nfc_t4t_emulation_stop();
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_emulation_stop, NFC_RPC_CMD_T4T_EMULATION_STOP,
			 nfc_rpc_t4t_emulation_stop_rpc_handler, NULL);

static void nfc_rpc_t4t_done_rpc_handler(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	int error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nfc_rpc_report_decoding_error(NFC_RPC_CMD_T4T_DONE);
		return;
	}

	error = nfc_t4t_done();
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(nfc_group, nfc_rpc_t4t_done, NFC_RPC_CMD_T4T_DONE,
			 nfc_rpc_t4t_done_rpc_handler, NULL);
