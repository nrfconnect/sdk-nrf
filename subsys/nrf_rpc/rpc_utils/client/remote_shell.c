/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "decode_helpers.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/remote_shell.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

#define MAX_ARGV_SIZE 255

static size_t get_cmd_len(size_t argc, char *argv[])
{
	size_t len = 0;

	for (size_t i = 0; i < argc; i++) {
		len += strnlen(argv[i], MAX_ARGV_SIZE) + 1;
	}

	return len;
}

static size_t create_cmd_line(char *buffer, size_t cmd_buffer_size, size_t argc, char *argv[])
{
	size_t len = 0;

	for (size_t i = 0; i < argc; i++) {
		size_t arg_len = strnlen(argv[i], MAX_ARGV_SIZE);

		if (len + arg_len + 1 > cmd_buffer_size) {
			return 0;
		}
		memcpy(buffer + len, argv[i], arg_len);
		len += arg_len;
		buffer[len++] = ' ';
	}

	if (len > 0) {
		buffer[len - 1] = '\0';
	}

	return len;
}

char *nrf_rpc_invoke_remote_shell_cmd(size_t argc, char *argv[])
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cmd_buffer_size = get_cmd_len(argc, argv);
	char *cmd_buffer = k_malloc(cmd_buffer_size);

	if (!cmd_buffer) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &rpc_utils_group,
			    RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD, NRF_RPC_PACKET_TYPE_CMD);
		return NULL;
	}

	size_t cmd_len = create_cmd_line(cmd_buffer, cmd_buffer_size, argc, argv);

	if (!cmd_len) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &rpc_utils_group,
			    RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD, NRF_RPC_PACKET_TYPE_CMD);
		k_free(cmd_buffer);
		return NULL;
	}

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 3 + cmd_len);

	nrf_rpc_encode_str(&ctx, cmd_buffer, cmd_len);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD, &ctx);

	return allocate_buffer_and_decode_str(&ctx, RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD);
}
