/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "openthread/error.h"
#include "zcbor_encode.h"
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <nrf_rpc/nrf_rpc_serialize.h>

#include <openthread/netdata.h>
#include <openthread/ip6.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/thread.h>

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

static void ot_rpc_cmd_ip6_get_unicast_addrs(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	addr = otIp6GetUnicastAddresses(openthread_get_default_instance());
	addr_count = 0;

	for (const otNetifAddress *p = addr; p != NULL; p = p->mNext) {
		++addr_count;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, addr_count * 25);

	for (; addr; addr = addr->mNext) {
		uint16_t flags =
			((uint16_t)addr->mPreferred << OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET) |
			((uint16_t)addr->mValid << OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
			((uint16_t)addr->mScopeOverrideValid
			 << OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET) |
			((uint16_t)addr->mScopeOverride << OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
			((uint16_t)addr->mRloc << OT_RPC_NETIF_ADDRESS_RLOC_OFFSET) |
			((uint16_t)addr->mMeshLocal << OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET);

		zcbor_list_start_encode(rsp_ctx.zs, 4);
		zcbor_bstr_encode_ptr(rsp_ctx.zs, (const char *)addr->mAddress.mFields.m8,
				      sizeof(addr->mAddress.mFields.m8));
		zcbor_uint32_put(rsp_ctx.zs, addr->mPrefixLength);
		zcbor_uint32_put(rsp_ctx.zs, addr->mAddressOrigin);
		zcbor_uint32_put(rsp_ctx.zs, flags);
		zcbor_list_end_encode(rsp_ctx.zs, 4);
	}

	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_get_multicast_addrs(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifMulticastAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	addr = otIp6GetMulticastAddresses(openthread_get_default_instance());
	addr_count = 0;

	for (const otNetifMulticastAddress *p = addr; p != NULL; p = p->mNext) {
		++addr_count;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, addr_count * 17);

	for (; addr; addr = addr->mNext) {
		zcbor_bstr_encode_ptr(rsp_ctx.zs, (const char *)addr->mAddress.mFields.m8,
				      OT_IP6_ADDRESS_SIZE);
	}

	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

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

static void ot_rpc_cmd_ip6_set_enabled(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bool_decode(ctx->zs, &enabled);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6SetEnabled(openthread_get_default_instance(), enabled);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_is_enabled(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	enabled = otIp6IsEnabled(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	zcbor_bool_encode(rsp_ctx.zs, &enabled);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_subscribe_multicast_address(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct zcbor_string addr_bstr;
	otIp6Address addr;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bstr_decode(ctx->zs, &addr_bstr);
	decoded_ok = decoded_ok && (addr_bstr.len == OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	memcpy(addr.mFields.m8, addr_bstr.value, addr_bstr.len);
	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6SubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_unsubscribe_multicast_address(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct zcbor_string addr_bstr;
	otIp6Address addr;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bstr_decode(ctx->zs, &addr_bstr);
	decoded_ok = decoded_ok && (addr_bstr.len == OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	memcpy(addr.mFields.m8, addr_bstr.value, addr_bstr.len);
	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6UnsubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_thread_set_enabled(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bool_decode(ctx->zs, &enabled);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetEnabled(openthread_get_default_instance(), enabled);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_thread_get_device_role(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otDeviceRole role;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	role = otThreadGetDeviceRole(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	zcbor_uint_encode(rsp_ctx.zs, &role, sizeof(role));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_link_mode(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t mode_mask;
	otLinkModeConfig mode;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint_decode(ctx->zs, &mode_mask, sizeof(mode_mask));
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	mode.mRxOnWhenIdle = (mode_mask & BIT(OT_RPC_LINK_MODE_RX_ON_WHEN_IDLE_OFFSET)) != 0;
	mode.mDeviceType = (mode_mask & BIT(OT_RPC_LINK_MODE_DEVICE_TYPE_OFFSET)) != 0;
	mode.mNetworkData = (mode_mask & BIT(OT_RPC_LINK_MODE_NETWORK_DATA_OFFSET)) != 0;

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetLinkMode(openthread_get_default_instance(), mode);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_link_mode(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otLinkModeConfig mode;
	uint8_t mode_mask = 0;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	mode = otThreadGetLinkMode(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (mode.mRxOnWhenIdle) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_RX_ON_WHEN_IDLE_OFFSET);
	}

	if (mode.mDeviceType) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_DEVICE_TYPE_OFFSET);
	}

	if (mode.mNetworkData) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_NETWORK_DATA_OFFSET);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	zcbor_uint_encode(rsp_ctx.zs, &mode_mask, sizeof(mode_mask));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint_decode(ctx->zs, &poll_period, sizeof(poll_period));
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otLinkSetPollPeriod(openthread_get_default_instance(), poll_period);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	poll_period = otLinkGetPollPeriod(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &poll_period, sizeof(poll_period));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_max_frame_retries_direct(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkSetMaxFrameRetriesDirect(openthread_get_default_instance(), max_retries);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_max_frame_retries_indirect(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkSetMaxFrameRetriesIndirect(openthread_get_default_instance(), max_retries);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_link_enabled(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	bool enabled = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_SET_ENABLED);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otLinkSetEnabled(openthread_get_default_instance(), enabled);
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_get_factory_assigned_eui64(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otExtAddress ext_addr;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otLinkGetFactoryAssignedIeeeEui64(openthread_get_default_instance(), &ext_addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(ext_addr.m8) + 1);
	nrf_rpc_encode_buffer(&rsp_ctx, &ext_addr, sizeof(ext_addr.m8));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_netdata_get(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	bool stable;
	bool decoded_ok;
	bool buff_allocated = false;
	uint8_t length = 0;

	decoded_ok = zcbor_bool_decode(ctx->zs, &stable);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoded_ok = zcbor_uint_decode(ctx->zs, &length, sizeof(length));
	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + length + 3);
	buff_allocated = true;

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());

	error = otNetDataGet(openthread_get_default_instance(), stable, rsp_ctx.zs[0].payload_mut,
			     &length);
	rsp_ctx.zs[0].payload_mut += length;

	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_bstr_end_encode(rsp_ctx.zs, NULL)) {
		goto exit;
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}

	if (!buff_allocated) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	}

	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_next_service(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	otNetworkDataIterator iterator;
	bool decoded_ok = false;
	otServiceConfig service_config;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(service_config) + 7);

	decoded_ok = zcbor_uint_decode(ctx->zs, &iterator, sizeof(iterator));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otNetDataGetNextService(openthread_get_default_instance(), &iterator,
					&service_config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_uint_encode(rsp_ctx.zs, &iterator, sizeof(iterator))) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	if (error == OT_ERROR_NONE) {
		ot_rpc_encode_service_config(&rsp_ctx, &service_config);
	} else {
		ot_rpc_encode_service_config(&rsp_ctx, NULL);
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_netdata_get_next_on_mesh_prefix(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	otNetworkDataIterator iterator;
	bool decoded_ok = false;
	otBorderRouterConfig config;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(config) + 14);

	decoded_ok = zcbor_uint_decode(ctx->zs, &iterator, sizeof(iterator));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otNetDataGetNextOnMeshPrefix(openthread_get_default_instance(), &iterator, &config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_uint_encode(rsp_ctx.zs, &iterator, sizeof(iterator))) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	if (error == OT_ERROR_NONE) {
		ot_rpc_encode_border_router_config(&rsp_ctx, &config);
	} else {
		ot_rpc_encode_border_router_config(&rsp_ctx, NULL);
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_get_unicast_addrs,
			 OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, ot_rpc_cmd_ip6_get_unicast_addrs,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_get_multicast_addrs,
			 OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES, ot_rpc_cmd_ip6_get_multicast_addrs,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_state_changed_callback,
			 OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_set_state_changed_callback, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_remove_state_changed_callback,
			 OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_remove_state_changed_callback, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_set_enabled, OT_RPC_CMD_IP6_SET_ENABLED,
			 ot_rpc_cmd_ip6_set_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_is_enabled, OT_RPC_CMD_IP6_IS_ENABLED,
			 ot_rpc_cmd_ip6_is_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_subscribe_multicast_address,
			 OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, ot_rpc_cmd_ip6_subscribe_multicast_address,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_unsubscribe_multicast_address,
			 OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR,
			 ot_rpc_cmd_ip6_unsubscribe_multicast_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_thread_set_enabled, OT_RPC_CMD_THREAD_SET_ENABLED,
			 ot_rpc_cmd_thread_set_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_thread_get_device_role,
			 OT_RPC_CMD_THREAD_GET_DEVICE_ROLE, ot_rpc_cmd_thread_get_device_role,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_link_mode, OT_RPC_CMD_THREAD_SET_LINK_MODE,
			 ot_rpc_cmd_set_link_mode, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_link_mode, OT_RPC_CMD_THREAD_GET_LINK_MODE,
			 ot_rpc_cmd_get_link_mode, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_poll_period, OT_RPC_CMD_LINK_SET_POLL_PERIOD,
			 ot_rpc_cmd_set_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_poll_period, OT_RPC_CMD_LINK_GET_POLL_PERIOD,
			 ot_rpc_cmd_get_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_direct,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT,
			 ot_rpc_cmd_set_max_frame_retries_direct, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_indirect,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT,
			 ot_rpc_cmd_set_max_frame_retries_indirect, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_link_enabled, OT_RPC_CMD_LINK_SET_ENABLED,
			 ot_rpc_cmd_set_link_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_factory_assigned_eui64,
			 OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64,
			 ot_rpc_cmd_get_factory_assigned_eui64, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get, OT_RPC_CMD_NETDATA_GET,
			 ot_rpc_cmd_netdata_get, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get_next_on_mesh_prefix,
			 OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX,
			 ot_rpc_cmd_netdata_get_next_on_mesh_prefix, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_service, OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE,
			 ot_rpc_cmd_get_next_service, NULL);
