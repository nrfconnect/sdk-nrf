/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zephyr/sys/util.h"
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <ncs_commit.h>
#include <dev_info_rpc_ids.h>

NRF_RPC_GROUP_DECLARE(dev_info_group);

#define MAX_ARGV_SIZE 255

static char *allocate_buffer_and_decode_str(struct nrf_rpc_cbor_ctx *ctx,
					    enum def_info_rpc_cmd_server cmd)
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

	if (!nrf_rpc_decoding_done_and_check(&dev_info_group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &dev_info_group, cmd,
			    NRF_RPC_PACKET_TYPE_RSP);
		k_free(output);
		return NULL;
	}

	return output;
}

char *nrf_rpc_get_ncs_commit_sha(void)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&dev_info_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&dev_info_group, DEV_INFO_RPC_GET_VERSION, &ctx);

	return allocate_buffer_and_decode_str(&ctx, DEV_INFO_RPC_GET_VERSION);
}

static size_t get_cmd_len(size_t argc, char *argv[])
{
	size_t len = 0;

	for (size_t i = 0; i < argc; i++) {
		if (argv[i] == NULL) {
			return 0;
		}

		len += strnlen(argv[i], MAX_ARGV_SIZE) + 1;
	}

	return len;
}

static size_t create_cmd_line(char *buffer, size_t cmd_buffer_size, size_t argc, char *argv[])
{
	size_t len = 0;

	for (size_t i = 0; i < argc; i++) {
		if (argv[i] == NULL) {
			return 0;
		}

		size_t arg_len = strnlen(argv[i], MAX_ARGV_SIZE);

		if (len + arg_len + 1 > cmd_buffer_size) {
			return 0;
		}
		memcpy(buffer + len, argv[i], arg_len);
		len += arg_len;
		buffer[len++] = ' ';
	}

	buffer[len - 1] = '\0';

	return len;
}

char *nrf_rpc_invoke_remote_shell_cmd(size_t argc, char *argv[])
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cmd_buffer_size = get_cmd_len(argc, argv);
	char *cmd_buffer = k_malloc(cmd_buffer_size);

	if (!cmd_buffer) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &dev_info_group,
			    DEV_INFO_RPC_INVOKE_SHELL_CMD, NRF_RPC_PACKET_TYPE_CMD);
		return NULL;
	}

	size_t cmd_len = create_cmd_line(cmd_buffer, cmd_buffer_size, argc, argv);

	if (!cmd_len) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &dev_info_group,
			    DEV_INFO_RPC_INVOKE_SHELL_CMD, NRF_RPC_PACKET_TYPE_CMD);
		k_free(cmd_buffer);
		return NULL;
	}

	NRF_RPC_CBOR_ALLOC(&dev_info_group, ctx, 3 + cmd_len);

	nrf_rpc_encode_str(&ctx, cmd_buffer, cmd_len);

	nrf_rpc_cbor_cmd_rsp_no_err(&dev_info_group, DEV_INFO_RPC_INVOKE_SHELL_CMD, &ctx);

	return allocate_buffer_and_decode_str(&ctx, DEV_INFO_RPC_INVOKE_SHELL_CMD);
}
