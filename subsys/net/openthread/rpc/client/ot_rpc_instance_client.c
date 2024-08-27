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
