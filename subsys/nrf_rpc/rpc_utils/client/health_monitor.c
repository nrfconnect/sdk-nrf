/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

#include <health_monitor.h>

#include "health_monitor_encoding.h"


static health_monitor_event_callback  event_callback;
static void			     *callback_context;

static void decode_event(struct nrf_rpc_cbor_ctx *ctx, struct health_event_data *event)
{
	static struct counters_data counters;

	switch (event->event_type) {
	case HEALTH_EVENT_STATS:
		XCODE_COUNTERS(counters, ctx);
		event->counters = &counters;
		break;
	case HEALTH_EVENT_NO_BUFFERS:
		event->no_buffers.total_count = nrf_rpc_decode_uint(ctx);
		event->no_buffers.max_used = nrf_rpc_decode_uint(ctx);
		event->no_buffers.current_free = nrf_rpc_decode_uint(ctx);
		break;
	default:
		/* unknown or no args event. */
		break;
	}
}

void rpc_util_health_moniotor_data(const struct nrf_rpc_group *group,
				   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct health_event_data event;

	event.event_type = nrf_rpc_decode_uint(ctx);
	decode_event(ctx, &event);

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (event_callback) {
		event_callback(&event, callback_context);
	}
	nrf_rpc_rsp_send_void(group);
}

void health_monitor_set_raport_interval(uint32_t seconds)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, sizeof(seconds)+1);
	nrf_rpc_encode_uint(&ctx, seconds);
	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group,
				RPC_UTIL_HEALTH_MONITOR_RAPORT_INTERVAL,
				&ctx,
				nrf_rpc_rsp_decode_void,
				NULL);
}

void health_monitor_set_event_callback(health_monitor_event_callback callback, void *context)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, callback != NULL);

	event_callback = callback;
	callback_context = context;

	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group,
				RPC_UTIL_HEALTH_MONITOR_CALLBACK,
				&ctx,
				nrf_rpc_rsp_decode_void,
				NULL);
}


NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group,
			 rpc_util_health_moniotor_data,
			 RPC_UTIL_HEALTH_MONITOR_DATA,
			 rpc_util_health_moniotor_data,
			 NULL);
