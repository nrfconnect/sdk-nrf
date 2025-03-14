/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/netdiag.h>

static void ot_rpc_cmd_set_vendor_name(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_NAME);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorName(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_set_vendor_model(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_MODEL);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorModel(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_set_vendor_sw_version(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorSwVersion(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_get_vendor_name(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorName(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_vendor_model(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorModel(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_vendor_sw_version(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorSwVersion(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_name, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
			 ot_rpc_cmd_set_vendor_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_model, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
			 ot_rpc_cmd_set_vendor_model, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_sw_version,
			 OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
			 ot_rpc_cmd_set_vendor_sw_version, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_name, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
			 ot_rpc_cmd_get_vendor_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_model, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
			 ot_rpc_cmd_get_vendor_model, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_sw_version,
			 OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
			 ot_rpc_cmd_get_vendor_sw_version, NULL);
