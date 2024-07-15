/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/srp_client.h>

/*
 * Store the host info and registered services locally for two reasons:
 * - to reduce the amount of data sent in the remote SRP client callback invocation
 *   by skipping the fields that are set by the client, anyway. This avoids sending
 *   the same data back and forth.
 * - to validate the service pointers received in the SRP client callback.
 */
static otSrpClientHostInfo host_info;
static otSrpClientService *services;
static otSrpClientAutoStartCallback auto_start_cb;
static void *auto_start_ctx;
static otSrpClientCallback client_cb;
static void *client_ctx;

static void clear_host(void)
{
	memset(&host_info, 0, sizeof(host_info));
	host_info.mState = OT_SRP_CLIENT_ITEM_STATE_REMOVED;
}

/* Find the location of 'next' pointer that points to 'searched'. */
otSrpClientService **find_service_prev_next(otSrpClientService *searched)
{
	otSrpClientService **prev_next = &services;

	for (otSrpClientService *s = services; s != NULL; prev_next = &s->mNext, s = s->mNext) {
		if (s == searched) {
			return prev_next;
		}
	}

	return NULL;
}

static void calc_subtypes_space(const char *const *subtypes, size_t *out_count,
				size_t *out_cbor_size)
{
	size_t count = 0;
	size_t cbor_size = 0;

	for (; *subtypes; ++subtypes) {
		++count;
		cbor_size += 2 + strlen(*subtypes);
	}

	*out_count = count;
	*out_cbor_size = cbor_size;
}

static void calc_txt_space(const otDnsTxtEntry *txt, uint8_t num, size_t *out_cbor_size)
{
	size_t cbor_size = 0;

	for (; num; ++txt, --num) {
		cbor_size += 2 + strlen(txt->mKey);
		cbor_size += 2 + txt->mValueLength;
	}

	*out_cbor_size = cbor_size;
}

otError otSrpClientAddService(otInstance *aInstance, otSrpClientService *aService)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t num_subtypes;
	size_t subtypes_size;
	size_t txt_size;
	size_t cbor_buffer_size;
	otError error;

	ARG_UNUSED(aInstance);
	calc_subtypes_space(aService->mSubTypeLabels, &num_subtypes, &subtypes_size);
	calc_txt_space(aService->mTxtEntries, aService->mNumTxtEntries, &txt_size);

	cbor_buffer_size = 1 + sizeof(uintptr_t); /* Service pointer */
	cbor_buffer_size += 2 + strlen(aService->mName);
	cbor_buffer_size += 2 + strlen(aService->mInstanceName);
	cbor_buffer_size += 1 + subtypes_size; /* Array of service subtypes */
	cbor_buffer_size += 1 + txt_size;      /* Map of TXT entries */
	cbor_buffer_size += 1 + sizeof(aService->mPort);
	cbor_buffer_size += 1 + sizeof(aService->mPriority);
	cbor_buffer_size += 1 + sizeof(aService->mWeight);
	cbor_buffer_size += 1 + sizeof(aService->mLease);
	cbor_buffer_size += 1 + sizeof(aService->mKeyLease);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aService);
	nrf_rpc_encode_str(&ctx, aService->mName, -1);
	nrf_rpc_encode_str(&ctx, aService->mInstanceName, -1);
	zcbor_list_start_encode(ctx.zs, num_subtypes);

	for (const char *const *subtype = aService->mSubTypeLabels; *subtype != NULL; ++subtype) {
		nrf_rpc_encode_str(&ctx, *subtype, -1);
	}

	zcbor_list_end_encode(ctx.zs, num_subtypes);
	zcbor_map_start_encode(ctx.zs, aService->mNumTxtEntries);

	for (size_t i = 0; i < aService->mNumTxtEntries; ++i) {
		nrf_rpc_encode_str(&ctx, aService->mTxtEntries[i].mKey, -1);
		nrf_rpc_encode_buffer(&ctx, aService->mTxtEntries[i].mValue,
				      aService->mTxtEntries[i].mValueLength);
	}

	zcbor_map_end_encode(ctx.zs, aService->mNumTxtEntries);
	nrf_rpc_encode_uint(&ctx, aService->mPort);
	nrf_rpc_encode_uint(&ctx, aService->mPriority);
	nrf_rpc_encode_uint(&ctx, aService->mWeight);
	nrf_rpc_encode_uint(&ctx, aService->mLease);
	nrf_rpc_encode_uint(&ctx, aService->mKeyLease);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, &ctx,
				ot_rpc_decode_error, &error);

	if (error == OT_ERROR_NONE) {
		/* Add the service to the local list */
		aService->mNext = services;
		services = aService;
	}

	return error;
}

void otSrpClientClearHostAndServices(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_CLEAR_HOST_AND_SERVICES, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	/* Clear the local data */
	clear_host();
	services = NULL;
}

otError otSrpClientClearService(otInstance *aInstance, otSrpClientService *aService)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	otSrpClientService **service_prev_next = find_service_prev_next(aService);

	if (!service_prev_next) {
		/* The service has never been registered */
		return OT_ERROR_NOT_FOUND;
	}

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aService);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_CLEAR_SERVICE, &ctx,
				ot_rpc_decode_error, &error);

	if (error == OT_ERROR_NONE) {
		/* Remove the service from the local list */
		*service_prev_next = aService->mNext;
		aService->mNext = NULL;
	}

	return error;
}

void otSrpClientDisableAutoStartMode(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_DISABLE_AUTO_START_MODE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otSrpClientEnableAutoHostAddress(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_ENABLE_AUTO_HOST_ADDR, &ctx,
				ot_rpc_decode_error, &error);

	if (error == OT_ERROR_NONE) {
		host_info.mAutoAddress = true;
	}

	return error;
}

void otSrpClientEnableAutoStartMode(otInstance *aInstance, otSrpClientAutoStartCallback aCallback,
				    void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, aCallback != NULL);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_ENABLE_AUTO_START_MODE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	auto_start_cb = aCallback;
	auto_start_ctx = aContext;
}

otError otSrpClientRemoveHostAndServices(otInstance *aInstance, bool aRemoveKeyLease,
					 bool aSendUnregToServer)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);
	nrf_rpc_encode_bool(&ctx, aRemoveKeyLease);
	nrf_rpc_encode_bool(&ctx, aSendUnregToServer);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_REMOVE_HOST_AND_SERVICES, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otSrpClientRemoveService(otInstance *aInstance, otSrpClientService *aService)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(uintptr_t));
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aService);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_REMOVE_SERVICE, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

void otSrpClientSetCallback(otInstance *aInstance, otSrpClientCallback aCallback, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, aCallback != NULL);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_SET_CALLBACK, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	client_cb = aCallback;
	client_ctx = aContext;
}

otError otSrpClientSetHostName(otInstance *aInstance, const char *aName)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	if (aName == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2 + strlen(aName));
	nrf_rpc_encode_str(&ctx, aName, -1);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_SET_HOSTNAME, &ctx,
				ot_rpc_decode_error, &error);

	if (error == OT_ERROR_NONE) {
		host_info.mName = aName;
	}

	return error;
}

void otSrpClientSetKeyLeaseInterval(otInstance *aInstance, uint32_t aInterval)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(aInterval));
	nrf_rpc_encode_uint(&ctx, aInterval);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_SET_KEY_LEASE_INTERVAL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void otSrpClientSetLeaseInterval(otInstance *aInstance, uint32_t aInterval)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(aInterval));
	nrf_rpc_encode_uint(&ctx, aInterval);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_SET_LEASE_INTERVAL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void otSrpClientSetTtl(otInstance *aInstance, uint32_t aTtl)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(aTtl));
	nrf_rpc_encode_uint(&ctx, aTtl);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_SET_TTL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void ot_rpc_cmd_srp_client_cb(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	otSrpClientService *service;
	otSrpClientItemState state;
	otSrpClientService **service_prev_next;
	otSrpClientService *removed_services = NULL;

	error = nrf_rpc_decode_uint(ctx);
	host_info.mState = nrf_rpc_decode_uint(ctx);

	while (!nrf_rpc_decode_is_null(ctx)) {
		service = (otSrpClientService *)nrf_rpc_decode_uint(ctx);
		state = nrf_rpc_decode_uint(ctx);

		if (!nrf_rpc_decode_valid(ctx)) {
			break;
		}

		service_prev_next = find_service_prev_next(service);

		if (!service_prev_next) {
			/* Provided service pointer unknown to the client */
			nrf_rpc_cbor_decoding_done(&ot_group, ctx);
			ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_CB);
			return;
		}

		service->mState = state;

		if (state == OT_SRP_CLIENT_ITEM_STATE_REMOVED) {
			/* Move the service to the 'removed' list */
			*service_prev_next = service->mNext;
			service->mNext = removed_services;
			removed_services = service;
		}
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_CB);
		return;
	}

	if (client_cb != NULL) {
		client_cb(error, &host_info, services, removed_services, client_ctx);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_cb, OT_RPC_CMD_SRP_CLIENT_CB,
			 ot_rpc_cmd_srp_client_cb, NULL);

static void ot_rpc_cmd_srp_client_auto_start_cb(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otSockAddr addr;

	nrf_rpc_decode_buffer(ctx, addr.mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	addr.mPort = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_AUTO_START_CB);
		return;
	}

	if (auto_start_cb != NULL) {
		auto_start_cb(&addr, auto_start_ctx);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_auto_start_cb,
			 OT_RPC_CMD_SRP_CLIENT_AUTO_START_CB, ot_rpc_cmd_srp_client_auto_start_cb,
			 NULL);
