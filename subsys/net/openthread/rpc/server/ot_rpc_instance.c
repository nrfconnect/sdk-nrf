/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc_cbor.h>

#include <openthread/instance.h>
#include <openthread/platform/misc.h>
#include <openthread/thread.h>

#include <zephyr/fs/nvs.h>
#include <zephyr/fs/zms.h>
#include <zephyr/net/openthread.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>

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
	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	(void)otInstanceInitSingle();
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_init_single, OT_RPC_CMD_INSTANCE_INIT_SINGLE,
			 ot_rpc_cmd_instance_init_single, NULL);

static void ot_rpc_cmd_instance_get_id(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t instance_id;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	instance_id = otInstanceGetId(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, instance_id);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_get_id, OT_RPC_CMD_INSTANCE_GET_ID,
			 ot_rpc_cmd_instance_get_id, NULL);

static void ot_rpc_cmd_instance_is_initialized(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool initialized;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	initialized = otInstanceIsInitialized(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_bool(group, initialized);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_is_initialized,
			 OT_RPC_CMD_INSTANCE_IS_INITIALIZED, ot_rpc_cmd_instance_is_initialized,
			 NULL);

static void ot_rpc_cmd_instance_finalize(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	otInstanceFinalize(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_finalize, OT_RPC_CMD_INSTANCE_FINALIZE,
			 ot_rpc_cmd_instance_finalize, NULL);

static void ot_rpc_cmd_instance_erase_persistent_info(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	otError error;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	error = otInstanceErasePersistentInfo(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_erase_persistent_info,
			 OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO,
			 ot_rpc_cmd_instance_erase_persistent_info, NULL);

static void ot_rpc_cmd_instance_factory_reset(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct otInstance *inst;

	nrf_rpc_cbor_decoding_done(group, ctx);

	/*
	 * The factory reset procedure involves rebooting the device, so send the response before it
	 * begins, until there is still a chance.
	 */
	nrf_rpc_rsp_send_void(group);

	inst = openthread_get_default_instance();

	if (IS_ENABLED(CONFIG_OPENTHREAD_RPC_ERASE_SETTINGS)) {
		int rc;
		void *storage;

		/*
		 * Lock the system scheduler to assure that no setting is written after the storage
		 * has been cleared and before the device is reset.
		 */
		k_sched_lock();
		rc = settings_storage_get(&storage);
		__ASSERT_NO_MSG(rc == 0);

		if (IS_ENABLED(CONFIG_SETTINGS_NVS)) {
			nvs_clear(storage);
		} else {
			zms_clear(storage);
		}

		otPlatReset(inst);
		k_sched_unlock();
	} else {
		ot_rpc_mutex_lock();
		otInstanceFactoryReset(inst);
		ot_rpc_mutex_unlock();
	}
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_instance_factory_reset,
			 OT_RPC_CMD_INSTANCE_FACTORY_RESET, ot_rpc_cmd_instance_factory_reset,
			 NULL);

static void ot_state_changed_callback(otChangedFlags aFlags, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_rpc_callback_t *cb = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 15);
	nrf_rpc_encode_uint(&ctx, cb->callback);
	nrf_rpc_encode_uint(&ctx, cb->context);
	nrf_rpc_encode_uint(&ctx, aFlags);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_STATE_CHANGED, &ctx, ot_rpc_decode_void,
				NULL);
	ot_rpc_mutex_lock();
}

static void ot_rpc_cmd_set_state_changed_callback(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t callback;
	uint32_t context;
	ot_rpc_callback_t *cb;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	callback = nrf_rpc_decode_uint(ctx);
	context = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK);
		return;
	}

	cb = ot_rpc_callback_put(callback, context);

	if (!cb) {
		error = OT_ERROR_NO_BUFS;
		goto out;
	}

	ot_rpc_mutex_lock();
	error = otSetStateChangedCallback(openthread_get_default_instance(),
					  ot_state_changed_callback, cb);
	ot_rpc_mutex_unlock();

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, error);
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
	ot_rpc_callback_t *cb;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	callback = nrf_rpc_decode_uint(ctx);
	context = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK);
		return;
	}

	cb = ot_rpc_callback_del(callback, context);

	if (!cb) {
		goto out;
	}

	ot_rpc_mutex_lock();
	otRemoveStateChangeCallback(openthread_get_default_instance(), ot_state_changed_callback,
				    cb);
	ot_rpc_mutex_unlock();

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_remove_state_changed_callback,
			 OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_remove_state_changed_callback, NULL);
