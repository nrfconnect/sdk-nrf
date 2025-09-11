/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ncs_commit.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>
#include <health_monitor.h>

#include <nrf_rpc_cbor.h>

static void rpc_util_health_moniotor_event(struct health_event_data *event, void *context);

static bool encode_event_data(struct nrf_rpc_cbor_ctx *ctx, struct health_event_data *event)
{
	switch (event->event_type) {
	case HEALTH_EVENT_STATS:
		nrf_rpc_encode_uint(ctx, event->counters->state_changes);
		nrf_rpc_encode_uint(ctx, event->counters->child_added);
		nrf_rpc_encode_uint(ctx, event->counters->child_removed);
		nrf_rpc_encode_uint(ctx, event->counters->partition_id_changes);
		nrf_rpc_encode_uint(ctx, event->counters->key_sequence_changes);
		nrf_rpc_encode_uint(ctx, event->counters->network_data_changes);
		nrf_rpc_encode_uint(ctx, event->counters->active_dataset_changes);
		nrf_rpc_encode_uint(ctx, event->counters->pending_dataset_changes);
		break;
	default:
		/* unknown or no args event. */
		break;
	}
	return true;
}

void rpc_util_health_moniotor_data(struct health_event_data *event)
{
	struct nrf_rpc_cbor_ctx ctx;

	__ASSERT(event, "Invalid event");

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 500);
	nrf_rpc_encode_uint(&ctx, event->event_type);
	if (encode_event_data(&ctx, event)) {
		nrf_rpc_cbor_cmd_no_err(&rpc_utils_group,
					RPC_UTIL_HEALTH_MONITOR_DATA,
					&ctx,
					nrf_rpc_rsp_decode_void,
					NULL);
	} else {
		NRF_RPC_CBOR_DISCARD(&rpc_utils_group, ctx);
	}
}

void rpc_util_health_moniotor_interval(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t interval;

	interval = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);

	health_monitor_set_raport_interval(interval);

	nrf_rpc_rsp_send_void(group);

}

void rpc_util_health_moniotor_callback(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	health_monitor_set_event_callback(rpc_util_health_moniotor_event, NULL);

	nrf_rpc_rsp_send_void(group);

}

static void rpc_util_health_moniotor_event(struct health_event_data *event, void *context)
{
	rpc_util_health_moniotor_data(event);
}


NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group,
			 rpc_util_health_moniotor_interval,
			 RPC_UTIL_HEALTH_MONITOR_RAPORT_INTERVAL,
			 rpc_util_health_moniotor_interval,
			 NULL);
NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group,
			 rpc_util_health_moniotor_callback,
			 RPC_UTIL_HEALTH_MONITOR_CALLBACK,
			 rpc_util_health_moniotor_callback,
			 NULL);
