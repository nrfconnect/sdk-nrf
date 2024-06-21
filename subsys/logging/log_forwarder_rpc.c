/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements Remote Procedure Calls (RPC) for logging subsystem.
 *
 */

#include "log_rpc_internal.h"

#include <nrf_rpc_cbor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rpc);

static void log_rpc_msg_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	uint8_t level;
	struct zcbor_string message;
	bool decoded_ok;

	decoded_ok = zcbor_uint_decode(ctx->zs, &level, sizeof(level));
	decoded_ok = decoded_ok && zcbor_bstr_decode(ctx->zs, &message);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto out;
	}

	switch (level) {
	case LOG_LEVEL_ERR:
		LOG_ERR("%.*s", message.len, message.value);
		break;
	case LOG_LEVEL_WRN:
		LOG_WRN("%.*s", message.len, message.value);
		break;
	case LOG_LEVEL_INF:
		LOG_INF("%.*s", message.len, message.value);
		break;
	case LOG_LEVEL_DBG:
		LOG_DBG("%.*s", message.len, message.value);
		break;
	default:
		break;
	}

out:
	return;
}

NRF_RPC_CBOR_EVT_DECODER(log_rpc_group, log_rpc_msg_handler, LOG_RPC_EVT_MSG, log_rpc_msg_handler,
			 NULL);
