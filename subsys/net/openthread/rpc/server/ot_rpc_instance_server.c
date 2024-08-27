/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/instance.h>

#include <zephyr/net/openthread.h>

static void ot_rpc_cmd_instance_init_single(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otInstance *instance;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	instance = otInstanceInitSingle();
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, (uintptr_t)instance);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_init_single, OT_RPC_CMD_INSTANCE_INIT_SINGLE,
			 ot_rpc_cmd_instance_init_single, NULL);

static void ot_rpc_cmd_instance_get_id(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otInstance *instance;
	uint32_t instance_id;

	instance = (otInstance *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	if (instance != openthread_get_default_instance()) {
		/* The instance is unknown to the OT RPC server. */
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	instance_id = otInstanceGetId(instance);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, instance_id);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_get_id, OT_RPC_CMD_INSTANCE_GET_ID,
			 ot_rpc_cmd_instance_get_id, NULL);

static void ot_rpc_cmd_instance_is_initialized(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otInstance *instance;
	bool initialized;

	instance = (otInstance *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	if (instance != openthread_get_default_instance()) {
		/* The instance is unknown to the OT RPC server. */
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	initialized = otInstanceIsInitialized(instance);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_bool(group, initialized);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_is_initialized,
			 OT_RPC_CMD_INSTANCE_IS_INITIALIZED, ot_rpc_cmd_instance_is_initialized,
			 NULL);

static void ot_rpc_cmd_instance_finalize(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otInstance *instance;

	instance = (otInstance *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	if (instance != openthread_get_default_instance()) {
		/* The instance is unknown to the OT RPC server. */
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otInstanceFinalize(instance);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_finalize, OT_RPC_CMD_INSTANCE_FINALIZE,
			 ot_rpc_cmd_instance_finalize, NULL);

static void ot_rpc_cmd_instance_erase_persistent_info(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	otInstance *instance;
	otError error;

	instance = (otInstance *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	if (instance != openthread_get_default_instance()) {
		/* The instance is unknown to the OT RPC server. */
		ot_rpc_report_decoding_error(OT_RPC_CMD_INSTANCE_GET_ID);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otInstanceErasePersistentInfo(instance);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_erase_persistent_info,
			 OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO,
			 ot_rpc_cmd_instance_erase_persistent_info, NULL);
