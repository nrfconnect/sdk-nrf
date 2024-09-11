/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/link.h>

static void ot_rpc_cmd_set_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint_decode(ctx->zs, &poll_period, sizeof(poll_period));
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otLinkSetPollPeriod(openthread_get_default_instance(), poll_period);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	poll_period = otLinkGetPollPeriod(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &poll_period, sizeof(poll_period));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_max_frame_retries_direct(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkSetMaxFrameRetriesDirect(openthread_get_default_instance(), max_retries);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_max_frame_retries_indirect(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkSetMaxFrameRetriesIndirect(openthread_get_default_instance(), max_retries);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_link_enabled(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	bool enabled = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_ENABLED);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otLinkSetEnabled(openthread_get_default_instance(), enabled);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_get_factory_assigned_eui64(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otExtAddress ext_addr;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkGetFactoryAssignedIeeeEui64(openthread_get_default_instance(), &ext_addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(ext_addr.m8) + 1);
	nrf_rpc_encode_buffer(&rsp_ctx, &ext_addr, sizeof(ext_addr.m8));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_poll_period, OT_RPC_CMD_LINK_SET_POLL_PERIOD,
			 ot_rpc_cmd_set_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_poll_period, OT_RPC_CMD_LINK_GET_POLL_PERIOD,
			 ot_rpc_cmd_get_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_direct,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT,
			 ot_rpc_cmd_set_max_frame_retries_direct, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_indirect,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT,
			 ot_rpc_cmd_set_max_frame_retries_indirect, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_link_enabled, OT_RPC_CMD_LINK_SET_ENABLED,
			 ot_rpc_cmd_set_link_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_factory_assigned_eui64,
			 OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64,
			 ot_rpc_cmd_get_factory_assigned_eui64, NULL);
