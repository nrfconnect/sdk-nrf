/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <zephyr/net/openthread.h>

#include <openthread/thread.h>

#define ACTIVE_SCAN_RESULTS_LENGTH(result) \
	sizeof((result)->mExtAddress.m8) + 1 + \
	sizeof((result)->mNetworkName.m8) + 1 + \
	sizeof((result)->mExtendedPanId.m8) + 1 + \
	sizeof((result)->mSteeringData.m8) + 1 + \
	sizeof((result)->mPanId) + 1 + \
	sizeof((result)->mJoinerUdpPort) + 1 + \
	sizeof((result)->mChannel) + 1 + \
	sizeof((result)->mRssi) + 1 + \
	sizeof((result)->mLqi) + 1 + \
	sizeof(uint8_t) + 1 + 3  /* mIsNative, mDiscover, mIsJoinable as booleans */

static void ot_thread_discover_cb(otActiveScanResult *result, void *context, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = result ? ACTIVE_SCAN_RESULTS_LENGTH(result) : 1;

	cbor_buffer_size += 10; /* for context and callback slot */
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	if (result) {
		nrf_rpc_encode_buffer(&ctx, result->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
		nrf_rpc_encode_str(&ctx, result->mNetworkName.m8, strlen(result->mNetworkName.m8));
		nrf_rpc_encode_buffer(&ctx, result->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
		nrf_rpc_encode_buffer(&ctx, result->mSteeringData.m8, OT_STEERING_DATA_MAX_LENGTH);
		nrf_rpc_encode_uint(&ctx, result->mPanId);
		nrf_rpc_encode_uint(&ctx, result->mJoinerUdpPort);
		nrf_rpc_encode_uint(&ctx, result->mChannel);
		nrf_rpc_encode_int(&ctx, result->mRssi);
		nrf_rpc_encode_uint(&ctx, result->mLqi);
		nrf_rpc_encode_uint(&ctx, result->mVersion);
		nrf_rpc_encode_bool(&ctx, result->mIsNative);
		nrf_rpc_encode_bool(&ctx, result->mDiscover);
		nrf_rpc_encode_bool(&ctx, result->mIsJoinable);
	} else {
		nrf_rpc_encode_null(&ctx);
	}

	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_DISCOVER_CB, &ctx, ot_rpc_decode_void,
				NULL);
}

NRF_RPC_CBKPROXY_HANDLER(ot_thread_discover_cb_encoder, ot_thread_discover_cb,
			 (otActiveScanResult *result, void *context), (result, context));

static void ot_rpc_thread_discover_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t scan_channels;
	uint16_t pan_id;
	bool joiner;
	bool enable_eui64_filtering;
	otHandleActiveScanResult cb;
	void *cb_ctx;
	otError error;

	scan_channels = nrf_rpc_decode_uint(ctx);
	pan_id = nrf_rpc_decode_uint(ctx);
	joiner = nrf_rpc_decode_bool(ctx);
	enable_eui64_filtering = nrf_rpc_decode_bool(ctx);
	cb = (otHandleActiveScanResult)nrf_rpc_decode_callbackd(ctx, ot_thread_discover_cb_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_THREAD_DISCOVER);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadDiscover(openthread_get_default_instance(), scan_channels, pan_id, joiner,
				 enable_eui64_filtering, cb, cb_ctx);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_thread_discover, OT_RPC_CMD_THREAD_DISCOVER,
			 ot_rpc_thread_discover_rpc_handler, NULL);

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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_thread_set_enabled, OT_RPC_CMD_THREAD_SET_ENABLED,
			 ot_rpc_cmd_thread_set_enabled, NULL);

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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_thread_get_device_role,
			 OT_RPC_CMD_THREAD_GET_DEVICE_ROLE, ot_rpc_cmd_thread_get_device_role,
			 NULL);

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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_link_mode, OT_RPC_CMD_THREAD_SET_LINK_MODE,
			 ot_rpc_cmd_set_link_mode, NULL);

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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_link_mode, OT_RPC_CMD_THREAD_GET_LINK_MODE,
			 ot_rpc_cmd_get_link_mode, NULL);

static void ot_rpc_cmd_get_version(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	nrf_rpc_rsp_send_uint(group, otThreadGetVersion());
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_version, OT_RPC_CMD_THREAD_GET_VERSION,
			 ot_rpc_cmd_get_version, NULL);
