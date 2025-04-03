/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>
#include <zephyr/sys/__assert.h>

#include <crash_gen_rpc_ids.h>

NRF_RPC_GROUP_DECLARE(crash_gen_group);

static void crash_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(crash_work, crash_work_handler);

static enum crash_gen_rpc_cmd_server last_command;

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
	switch (last_command) {
	case CRASH_GEN_RPC_ASSERT:
		do_assert();
		break;
	case CRASH_GEN_RPC_HARD_FAULT:
		do_hardfault();
		break;
	case CRASH_GEN_RPC_STACK_OVERFLOW:
		do_stack_overflow();
		break;
	}
}

static void generate_assert(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	last_command = CRASH_GEN_RPC_ASSERT;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(crash_gen_group, generate_assert, CRASH_GEN_RPC_ASSERT, generate_assert,
			 NULL);

static void generate_hard_fault(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	last_command = CRASH_GEN_RPC_HARD_FAULT;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(crash_gen_group, generate_hard_fault, CRASH_GEN_RPC_HARD_FAULT,
			 generate_hard_fault, NULL);

static void generate_stack_overflow(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	uint32_t timeout_ms = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);
	nrf_rpc_rsp_send_void(group);

	last_command = CRASH_GEN_RPC_STACK_OVERFLOW;
	k_work_reschedule(&crash_work, K_MSEC(timeout_ms));
}

NRF_RPC_CBOR_CMD_DECODER(crash_gen_group, generate_stack_overflow, CRASH_GEN_RPC_STACK_OVERFLOW,
			 generate_stack_overflow, NULL);
