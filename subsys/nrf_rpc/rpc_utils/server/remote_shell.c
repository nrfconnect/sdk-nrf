/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/shell/shell_dummy.h>

static int shell_exec(const char *line)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	shell_backend_dummy_clear_output(sh);
	return shell_execute_cmd(sh, line);
}

const char *shell_get_output(size_t *len)
{
	return shell_backend_dummy_get_output(shell_backend_dummy_get_ptr(), len);
}

static void remote_shell_cmd(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	char *cmd_buffer = NULL;
	const void *ptr;
	size_t len = 0;
	const char *output = NULL;

	ptr = nrf_rpc_decode_str_ptr_and_len(ctx, &len);

	if (ptr) {
		cmd_buffer = k_malloc(len + 1);
		if (cmd_buffer) {
			memcpy(cmd_buffer, ptr, len);
			cmd_buffer[len] = '\0';
		}
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group,
			    RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD, NRF_RPC_PACKET_TYPE_CMD);
		goto exit;
	}

	if (!ptr || !cmd_buffer) {
		goto exit;
	}

	shell_exec(cmd_buffer);
	output = shell_get_output(&len);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 3 + len);

	nrf_rpc_encode_str(&rsp_ctx, output, len);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);

exit:
	k_free(cmd_buffer);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, remote_shell_cmd, RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD,
			 remote_shell_cmd, NULL);
