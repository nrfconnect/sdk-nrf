/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <debug/rram_wear_test.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/rram_wear_test.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/kernel.h>

static void rram_wear_test_get_partition_count(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx,
						void *handler_data)
{
	ARG_UNUSED(handler_data);

	nrf_rpc_cbor_decoding_done(group, ctx);

	nrf_rpc_rsp_send_uint(group, rram_wear_test_partition_count());
}

static void rram_wear_test_get_partition(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx,
					  void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp;
	struct rram_wear_test_partition part;
	size_t index;
	int ret;

	ARG_UNUSED(handler_data);

	index = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);

	ret = rram_wear_test_partition_get(index, &part);
	if (ret != 0) {
		NRF_RPC_CBOR_ALLOC(group, rsp, 5);
		nrf_rpc_encode_int(&rsp, ret);
		nrf_rpc_cbor_rsp_no_err(group, &rsp);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp, 5 + 2 * 9 + strlen(part.name) + 5);
	nrf_rpc_encode_int(&rsp, 0);
	nrf_rpc_encode_uint64(&rsp, part.addr_start);
	nrf_rpc_encode_uint64(&rsp, part.addr_end);
	nrf_rpc_encode_str(&rsp, part.name, strlen(part.name));
	nrf_rpc_cbor_rsp_no_err(group, &rsp);
}

static void rram_wear_test_cmd(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	size_t index;
	uint64_t addr_start = 0;
	uint64_t addr_end = 0;
	bool force = false;
	int ret;

	ARG_UNUSED(handler_data);

	index = nrf_rpc_decode_uint(ctx);
	addr_start = nrf_rpc_decode_uint64(ctx);
	addr_end = nrf_rpc_decode_uint64(ctx);
	force = nrf_rpc_decode_bool(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);

	ret = rram_wear_test(index, addr_start, addr_end, force);
	nrf_rpc_rsp_send_int(group, ret);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, rram_wear_test_cmd, RPC_UTIL_RRAM_WEAR_TEST,
			 rram_wear_test_cmd, NULL);

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, rram_wear_test_get_partition_count,
			 RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION_COUNT,
			 rram_wear_test_get_partition_count, NULL);

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, rram_wear_test_get_partition,
			 RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION,
			 rram_wear_test_get_partition, NULL);
