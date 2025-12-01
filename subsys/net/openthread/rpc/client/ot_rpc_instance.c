/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_macros.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/instance.h>

/*
 * The actual otInstance object resides on OT RPC server and is only accessed by OT RPC client using
 * remote OpenThread API calls. Nevertheless, otInstanceInitSingle() on OT RPC client shall return a
 * valid non-null address, so the variable below is defined to represent otInstance object.
 */
static char ot_instance;

otInstance *otInstanceInitSingle(void)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_INIT_SINGLE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	return (otInstance *)&ot_instance;
}

uint32_t otInstanceGetId(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t id;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_GET_ID, &ctx, nrf_rpc_rsp_decode_u32,
				&id);

	return id;
}

bool otInstanceIsInitialized(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool initialized;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_IS_INITIALIZED, &ctx,
				nrf_rpc_rsp_decode_bool, &initialized);

	return initialized;
}

void otInstanceFinalize(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_FINALIZE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otInstanceErasePersistentInfo(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

void otInstanceFactoryReset(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_INSTANCE_FACTORY_RESET, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				  void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	nrf_rpc_encode_uint(&ctx, (uint32_t)aCallback);
	nrf_rpc_encode_uint(&ctx, (uint32_t)aContext);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				 void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	nrf_rpc_encode_uint(&ctx, (uint32_t)aCallback);
	nrf_rpc_encode_uint(&ctx, (uint32_t)aContext);

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

	callback = nrf_rpc_decode_uint(ctx);
	context = nrf_rpc_decode_uint(ctx);
	flags = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_STATE_CHANGED);
		return;
	}

	ot_rpc_mutex_lock();
	/* TODO: implement callback & address pseudonymization. */
	((otStateChangedCallback)callback)(flags, (void *)context);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_state_changed, OT_RPC_CMD_STATE_CHANGED,
			 ot_rpc_cmd_state_changed, NULL);
