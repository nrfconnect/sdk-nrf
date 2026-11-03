/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT_RPC_CMD_PING client (UART / RPC link test).
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include "dect_rpc_client_net.h"

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <errno.h>
#include <zephyr/kernel.h>

static K_SEM_DEFINE(ping_rsp_sem, 0, 1);

static void ping_rsp_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	ARG_UNUSED(group);
	ARG_UNUSED(handler_data);

	if (nrf_rpc_decode_valid(ctx)) {
		(void)nrf_rpc_decode_skip(ctx);
	}
	k_sem_give(&ping_rsp_sem);
}

static uint32_t dect_rpc_ping_seq;

#define DECT_RPC_PING_PRE_MS   250
#define DECT_RPC_PING_ATTEMPTS 3
#define DECT_RPC_PING_RETRY_MS 400

int dect_rpc_client_ping(k_timeout_t timeout)
{
	struct nrf_rpc_cbor_ctx req_ctx;
	int ret;
	int attempt;

	k_sem_take(&ping_rsp_sem, K_NO_WAIT);
	k_msleep(DECT_RPC_PING_PRE_MS);
	for (attempt = 0; attempt < DECT_RPC_PING_ATTEMPTS; attempt++) {
		if (attempt > 0) {
			k_msleep(DECT_RPC_PING_RETRY_MS);
		}
		NRF_RPC_CBOR_ALLOC(&dect_rpc_group, req_ctx, 8);
		nrf_rpc_encode_uint(&req_ctx, dect_rpc_ping_seq++);
		ret = nrf_rpc_cbor_cmd(&dect_rpc_group, DECT_RPC_CMD_PING, &req_ctx,
				       ping_rsp_handler, NULL);
		if (ret == 0) {
			return k_sem_take(&ping_rsp_sem, timeout) == 0 ? 0 : -ETIMEDOUT;
		}
	}
	return -EIO;
}
