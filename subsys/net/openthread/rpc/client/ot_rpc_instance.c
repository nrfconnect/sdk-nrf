/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/instance.h>

static otError decode_ot_error(struct nrf_rpc_cbor_ctx *ctx)
{
	otError error;

	if (!zcbor_uint_decode(ctx->zs, &error, sizeof(error))) {
		error = OT_ERROR_PARSE;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, ctx);

	return error;
}

otInstance *otInstanceInitSingle(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	uintptr_t instance_rep;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_INSTANCE_INIT_SINGLE, &ctx);
	nrf_rpc_rsp_decode_uint(&ot_group, &ctx, &instance_rep, sizeof(instance_rep));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return (otInstance *)instance_rep;
}

uint32_t otInstanceGetId(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t id;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aInstance);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_GET_ID, &ctx, nrf_rpc_rsp_decode_u32,
				&id);

	return id;
}

bool otInstanceIsInitialized(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool initialized;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aInstance);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_IS_INITIALIZED, &ctx,
				nrf_rpc_rsp_decode_bool, &initialized);

	return initialized;
}

void otInstanceFinalize(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aInstance);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_FINALIZE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otInstanceErasePersistentInfo(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aInstance);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				  void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	zcbor_uint32_put(ctx.zs, (uint32_t)aCallback);
	zcbor_uint32_put(ctx.zs, (uint32_t)aContext);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK, &ctx);

	return decode_ot_error(&ctx);
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				 void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	zcbor_uint32_put(ctx.zs, (uint32_t)aCallback);
	zcbor_uint32_put(ctx.zs, (uint32_t)aContext);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK, &ctx,
				ot_rpc_decode_void, NULL);
}

static void ot_rpc_cmd_state_changed(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint32_t callback;
	uint32_t context;
	otChangedFlags flags;
	bool decoded_ok;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback) &&
		     zcbor_uint32_decode(ctx->zs, &context) && zcbor_uint32_decode(ctx->zs, &flags);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto out;
	}

	/* TODO: implement callback & address pseudonymization. */
	((otStateChangedCallback)callback)(flags, (void *)context);

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_state_changed, OT_RPC_CMD_STATE_CHANGED,
			 ot_rpc_cmd_state_changed, NULL);
