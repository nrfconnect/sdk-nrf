/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/instance.h>

#include <zephyr/net/openthread.h>

/* TODO: move to common */
typedef struct ot_rpc_callback {
	uint32_t callback;
	uint32_t context;
} ot_rpc_callback_t;

static ot_rpc_callback_t ot_rpc_callbacks[10];

static struct ot_rpc_callback *ot_rpc_callback_put(uint32_t callback, uint32_t context)
{
	ot_rpc_callback_t *free = NULL;
	ot_rpc_callback_t *cb = ot_rpc_callbacks;

	while (cb < ot_rpc_callbacks + ARRAY_SIZE(ot_rpc_callbacks)) {
		if (cb->callback == 0) {
			free = cb;
		} else if (cb->callback == callback && cb->context == context) {
			return cb;
		}

		++cb;
	}

	if (free != NULL) {
		free->callback = callback;
		free->context = context;
	}

	return free;
}

static ot_rpc_callback_t *ot_rpc_callback_del(uint32_t callback, uint32_t context)
{
	ot_rpc_callback_t *cb = ot_rpc_callbacks;

	while (cb < ot_rpc_callbacks + ARRAY_SIZE(ot_rpc_callbacks)) {
		if (cb->callback == callback && cb->context == context) {
			cb->callback = 0;
			cb->context = 0;
			return cb;
		}

		++cb;
	}

	return NULL;
}

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

static void ot_state_changed_callback(otChangedFlags aFlags, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_rpc_callback_t *cb = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 15);
	zcbor_uint32_put(ctx.zs, cb->callback);
	zcbor_uint32_put(ctx.zs, cb->context);
	zcbor_uint32_put(ctx.zs, aFlags);

	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_STATE_CHANGED, &ctx, ot_rpc_decode_void,
				NULL);
	openthread_api_mutex_lock(openthread_get_default_context());
}

static void ot_rpc_cmd_set_state_changed_callback(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t callback;
	uint32_t context;
	bool decoded_ok;
	ot_rpc_callback_t *cb;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback);
	decoded_ok = decoded_ok && zcbor_uint32_decode(ctx->zs, &context);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	cb = ot_rpc_callback_put(callback, context);

	if (!cb) {
		error = OT_ERROR_NO_BUFS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSetStateChangedCallback(openthread_get_default_instance(),
					  ot_state_changed_callback, cb);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint32_put(rsp_ctx.zs, (uint32_t)error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_state_changed_callback,
			 OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_set_state_changed_callback, NULL);

static void ot_rpc_cmd_remove_state_changed_callback(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t callback;
	uint32_t context;
	bool decoded_ok;
	ot_rpc_callback_t *cb;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback);
	decoded_ok = decoded_ok && zcbor_uint32_decode(ctx->zs, &context);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto out;
	}

	cb = ot_rpc_callback_del(callback, context);

	if (!cb) {
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otRemoveStateChangeCallback(openthread_get_default_instance(), ot_state_changed_callback,
				    cb);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_remove_state_changed_callback,
			 OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_remove_state_changed_callback, NULL);
