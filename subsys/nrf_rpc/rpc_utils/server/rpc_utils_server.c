/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <ncs_commit.h>
#include <rpc_utils_ids.h>

#if defined(CONFIG_NRF_RPC_REMOTE_SHELL)
#include <zephyr/shell/shell_dummy.h>
#endif

NRF_RPC_GROUP_DECLARE(rpc_utils_group);

#if defined(CONFIG_NRF_RPC_CRASH_GEN)

static enum rpc_utils_cmd_server crash_command;

static void crash_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(crash_work, crash_work_handler);

#endif /* CONFIG_NRF_RPC_CRASH_GEN */

#if defined(CONFIG_NRF_RPC_DEV_INFO)

static void get_server_version(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, RPC_UTIL_DEV_INFO_GET_VERSION,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	cbor_buffer_size += 2 + sizeof(NCS_COMMIT_STRING) - 1;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_str(&rsp_ctx, NCS_COMMIT_STRING, sizeof(NCS_COMMIT_STRING) - 1);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, get_server_version, RPC_UTIL_DEV_INFO_GET_VERSION,
			 get_server_version, NULL);

#if defined(CONFIG_NRF_RPC_REMOTE_SHELL)
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
#endif /* CONFIG_NRF_RPC_REMOTE_SHELL */

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

#if defined(CONFIG_NRF_RPC_REMOTE_SHELL)
	shell_exec(cmd_buffer);
	output = shell_get_output(&len);
#else
	output = "Remote shell is disabled on the server. "
		 "Enable CONFIG_NRF_RPC_REMOTE_SHELL to use it.";
	len = strnlen(output, 255);
#endif /* CONFIG_NRF_RPC_REMOTE_SHELL */

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 3 + len);

	nrf_rpc_encode_str(&rsp_ctx, output, len);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);

exit:
	k_free(cmd_buffer);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, remote_shell_cmd, RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD,
			 remote_shell_cmd, NULL);

#endif /* CONFIG_NRF_RPC_DEV_INFO */

#if defined(CONFIG_NRF_RPC_CRASH_GEN)

static void do_stack_overflow(void)
{
	volatile uint8_t arr[(256 * 1024)] __attribute__((unused));
}

static void do_assert(void)
{
	__ASSERT_NO_MSG(false);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"

static void do_hardfault(void)
{
	volatile uint32_t value __attribute__((unused)) = 1 / 0;
}

#pragma GCC diagnostic pop

static void crash_work_handler(struct k_work *work)
{
	switch (crash_command) {
	case RPC_UTIL_CRASH_GEN_ASSERT:
		do_assert();
		break;
	case RPC_UTIL_CRASH_GEN_HARD_FAULT:
		do_hardfault();
		break;
	case RPC_UTIL_CRASH_GEN_STACK_OVERFLOW:
		do_stack_overflow();
		break;
	default:
		__ASSERT(false, "Invalid crash command");
	}
}

static void generate_assert(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	crash_command = RPC_UTIL_CRASH_GEN_ASSERT;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, generate_assert, RPC_UTIL_CRASH_GEN_ASSERT,
			 generate_assert, NULL);

static void generate_hard_fault(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	crash_command = RPC_UTIL_CRASH_GEN_HARD_FAULT;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, generate_hard_fault, RPC_UTIL_CRASH_GEN_HARD_FAULT,
			 generate_hard_fault, NULL);

static void generate_stack_overflow(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	crash_command = RPC_UTIL_CRASH_GEN_STACK_OVERFLOW;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, generate_stack_overflow,
			 RPC_UTIL_CRASH_GEN_STACK_OVERFLOW, generate_stack_overflow, NULL);

#endif /* CONFIG_NRF_RPC_CRASH_GEN */
