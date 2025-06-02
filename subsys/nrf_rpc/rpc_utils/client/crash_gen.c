/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/crash_gen.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

void nrf_rpc_crash_gen_assert(uint32_t delay_ms)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group, RPC_UTIL_CRASH_GEN_ASSERT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void nrf_rpc_crash_gen_hard_fault(uint32_t delay_ms)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group, RPC_UTIL_CRASH_GEN_HARD_FAULT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void nrf_rpc_crash_gen_stack_overflow(uint32_t delay_ms)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group, RPC_UTIL_CRASH_GEN_STACK_OVERFLOW, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}
