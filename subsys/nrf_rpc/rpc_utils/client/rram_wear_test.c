/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include "decode_helpers.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/rram_wear_test.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

int nrf_rpc_rram_wear_test(size_t index, uint64_t addr_start, uint64_t addr_end,
				    bool force)
{
	struct nrf_rpc_cbor_ctx ctx;
	int32_t result;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 4 + sizeof(uint32_t) + 2 * sizeof(uint64_t));

	nrf_rpc_encode_uint(&ctx, index);
	nrf_rpc_encode_uint64(&ctx, addr_start);
	nrf_rpc_encode_uint64(&ctx, addr_end);
	nrf_rpc_encode_bool(&ctx, force);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_RRAM_WEAR_TEST, &ctx);

	result = nrf_rpc_decode_int(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_RRAM_WEAR_TEST, NRF_RPC_PACKET_TYPE_RSP);
		return -EBADMSG;
	}

	return (int)result;
}

int nrf_rpc_rram_wear_test_get_partition_count(size_t *count)
{
	struct nrf_rpc_cbor_ctx ctx;

	if (count == NULL) {
		return -EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group,
				    RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION_COUNT, &ctx);

	*count = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION_COUNT, NRF_RPC_PACKET_TYPE_RSP);
		return -EBADMSG;
	}

	return 0;
}

int nrf_rpc_rram_wear_test_get_partition(size_t index, struct rram_wear_test_partition *part)
{
	struct nrf_rpc_cbor_ctx ctx;
	int ret;

	if (part == NULL) {
		return -EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 5);
	nrf_rpc_encode_uint(&ctx, index);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION, &ctx);

	ret = nrf_rpc_decode_int(&ctx);
	if (ret != 0) {
		nrf_rpc_cbor_decoding_done(&rpc_utils_group, &ctx);
		return ret;
	}

	part->addr_start = nrf_rpc_decode_uint64(&ctx);
	part->addr_end = nrf_rpc_decode_uint64(&ctx);

	size_t len;
	const char *name_ptr = nrf_rpc_decode_str_ptr_and_len(&ctx, &len);
	char *name_copy = NULL;

	if (name_ptr != NULL) {
		name_copy = k_malloc(len + 1);
		if (name_copy == NULL) {
			nrf_rpc_cbor_decoding_done(&rpc_utils_group, &ctx);
			return -ENOMEM;
		}
		memcpy(name_copy, name_ptr, len);
		name_copy[len] = '\0';
	}

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		if (name_copy != NULL) {
			k_free(name_copy);
		}
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_RRAM_WEAR_TEST_GET_PARTITION, NRF_RPC_PACKET_TYPE_RSP);
		return -EBADMSG;
	}

	part->name = name_copy;

	return 0;
}

void nrf_rpc_rram_wear_test_partition_free(struct rram_wear_test_partition *part)
{
	if (part != NULL && part->name != NULL) {
		k_free((void *)(part->name));
		part->name = NULL;
	}
}
