/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <zephyr/kernel.h>

static inline char *allocate_buffer_and_decode_str(struct nrf_rpc_cbor_ctx *ctx,
					    enum rpc_utils_cmd_server cmd)
{
	const void *ptr;
	size_t len;
	char *output = NULL;

	ptr = nrf_rpc_decode_str_ptr_and_len(ctx, &len);

	if (ptr) {
		output = k_malloc(len + 1);
		if (output) {
			memcpy(output, ptr, len);
			output[len] = '\0';
		}
	}

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group, cmd,
			    NRF_RPC_PACKET_TYPE_RSP);
		k_free(output);
		return NULL;
	}

	return output;
}
