/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

static enum rpc_utils_cmd_server crash_command;
static void crash_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(crash_work, crash_work_handler);

static void do_stack_overflow(void)
{
	volatile uint8_t arr[CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE];

	/* Write to 'arr' to prevent the compiler from optimizing it away. */
	arr[0] = 1;
}

static void do_assert(void)
{
	__ASSERT_NO_MSG(false);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"

static void do_hardfault(void)
{
	volatile uint32_t value;

	value = 1 / 0;
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
