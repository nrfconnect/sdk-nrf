/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <crash_gen_rpc_ids.h>

NRF_RPC_GROUP_DECLARE(crash_gen_group);

void nrf_rpc_crash_gen_assert(uint32_t delay_ms) {
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&crash_gen_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&crash_gen_group, CRASH_GEN_RPC_ASSERT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void nrf_rpc_crash_gen_hard_fault(uint32_t delay_ms) {
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&crash_gen_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&crash_gen_group, CRASH_GEN_RPC_HARD_FAULT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void nrf_rpc_crash_gen_stack_overflow(uint32_t delay_ms) {
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&crash_gen_group, ctx, 5);

	nrf_rpc_encode_uint(&ctx, delay_ms);

	nrf_rpc_cbor_cmd_no_err(&crash_gen_group, CRASH_GEN_RPC_STACK_OVERFLOW, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}
