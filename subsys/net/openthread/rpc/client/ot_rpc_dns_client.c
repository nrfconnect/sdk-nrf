/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <openthread/dns_client.h>

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>
#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <zcbor_encode.h>

static otDnsQueryConfig default_config;

static otError decode_ot_error(struct nrf_rpc_cbor_ctx *ctx)
{
	otError error;

	if (!zcbor_uint_decode(ctx->zs, &error, sizeof(error))) {
		error = OT_ERROR_PARSE;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, ctx);

	return error;
}

const otDnsQueryConfig *otDnsClientGetDefaultConfig(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool decoded_ok;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG, &ctx);

	decoded_ok = ot_rpc_decode_dns_query_config(&ctx, NULL, &default_config);

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG, NRF_RPC_PACKET_TYPE_RSP);
	}

	return &default_config;
}

void otDnsClientSetDefaultConfig(otInstance *aInstance, const otDnsQueryConfig *aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool encoded;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, OT_RPC_DNS_QUERY_CONFIG_SIZE);

	encoded = ot_rpc_encode_dns_query_config(&ctx, aConfig);

	if (encoded) {
		nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG, &ctx,
					ot_rpc_decode_void, NULL);
	}
}

static otError resolve_or_browse(enum ot_rpc_cmd_server cmd, const char *name, const char *service,
				 void *cb, void *data, const otDnsQueryConfig *config)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t to_alloc;
	size_t name_len;
	size_t service_len;

	if (!name) {
		return OT_ERROR_INVALID_ARGS;
	}

	name_len = strlen(name);

	to_alloc = name_len + 2;
	to_alloc += sizeof(uintptr_t) + 1;
	to_alloc += sizeof(uint32_t) + 1;

	if (service) {
		service_len = strlen(service);
		to_alloc += service_len + 2;
	}

	if (config) {
		to_alloc += OT_RPC_DNS_QUERY_CONFIG_SIZE;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, to_alloc);

	nrf_rpc_encode_str(&ctx, name, name_len + 1);

	if (service) {
		nrf_rpc_encode_str(&ctx, service, service_len + 1);
	}

	nrf_rpc_encode_callback(&ctx, cb);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)data);

	if (config) {
		ot_rpc_encode_dns_query_config(&ctx, config);
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, cmd, &ctx);

	return decode_ot_error(&ctx);
}

static otError fetch_strings_from_dns_response(enum ot_rpc_cmd_server cmd, const void *response,
					       uint16_t *index, char *primary, uint16_t primary_len,
					       char *secondary, uint16_t secondary_len)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	struct zcbor_string result;

	if (!response || !primary || !primary_len) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 4 * (sizeof(uint32_t) + 1));

	nrf_rpc_encode_uint(&ctx, (uintptr_t)(response));

	if (index) {
		nrf_rpc_encode_uint(&ctx, *index);
	}

	nrf_rpc_encode_uint(&ctx, primary_len);

	if (secondary) {
		nrf_rpc_encode_uint(&ctx, secondary_len);
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, cmd, &ctx);

	if (!zcbor_uint_decode(ctx.zs, &error, sizeof(uint32_t))) {
		goto fail;
	}

	/* Verify that we can proceed with decoding the name */
	if (error != OT_ERROR_NONE) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		return error;
	}

	if (!zcbor_tstr_decode(ctx.zs, &result)) {
		goto fail;
	}

	memcpy(primary, result.value, result.len);

	if (secondary) {
		if (!zcbor_tstr_decode(ctx.zs, &result)) {
			goto fail;
		}
		memcpy(secondary, result.value, result.len);
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return OT_ERROR_NONE;

fail:
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
	ot_rpc_report_decoding_error(cmd);

	return OT_ERROR_NO_BUFS;
}

static otError fetch_host_address_from_response(enum ot_rpc_cmd_server cmd, const void *response,
						const char *hostname, uint16_t index,
						otIp6Address *address, uint32_t *ttl)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	struct zcbor_string res_addr;
	size_t name_len;
	size_t alloc_len = 2 * (sizeof(uint32_t) + 1);

	if (!response || !address) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (hostname) {
		name_len = strlen(hostname) + 1; /* Include NULL char */
		alloc_len += name_len + 2;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, alloc_len);

	nrf_rpc_encode_uint(&ctx, (uintptr_t)(response));
	nrf_rpc_encode_uint(&ctx, index);

	if (hostname) {
		nrf_rpc_encode_str(&ctx, hostname, name_len);
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, cmd, &ctx);

	if (!zcbor_uint_decode(ctx.zs, &error, sizeof(uint32_t))) {
		goto fail;
	}

	/* Verify that we can proceed with decoding the name */
	if (error != OT_ERROR_NONE) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

		return error;
	}

	if (!zcbor_bstr_decode(ctx.zs, &res_addr)) {
		goto fail;
	}

	if (ttl && !zcbor_uint_decode(ctx.zs, ttl, sizeof(uint32_t))) {
		goto fail;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	memcpy(address, res_addr.value, res_addr.len);

	return OT_ERROR_NONE;
fail:
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
	ot_rpc_report_decoding_error(cmd);

	return OT_ERROR_NO_BUFS;
}

otError fetch_service_info_from_dns_response(enum ot_rpc_cmd_server cmd, const void *response,
					     const char *instance, otDnsServiceInfo *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	size_t alloc_len = 3 * (sizeof(uint32_t) + 1);
	size_t instance_len;
	uint32_t tmp;
	struct zcbor_string resp_str;

	if (!info) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (instance) {
		instance_len = strlen(instance) + 1; /* Include NULL char */
		alloc_len += instance_len + 2;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, alloc_len);

	nrf_rpc_encode_uint(&ctx, (uintptr_t)(response));
	nrf_rpc_encode_uint(&ctx, info->mHostNameBuffer ? info->mHostNameBufferSize : 0);
	nrf_rpc_encode_uint(&ctx, info->mTxtData ? info->mTxtDataSize : 0);

	if (instance) {
		nrf_rpc_encode_str(&ctx, instance, instance_len);
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, cmd, &ctx);

	if (!zcbor_uint_decode(ctx.zs, &error, sizeof(otError))) {
		goto fail;
	}

	if (error != OT_ERROR_NONE) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

		return (otError)error;
	}

	if (!zcbor_uint32_decode(ctx.zs, &info->mTtl)) {
		goto fail;
	}

	if (!zcbor_uint32_decode(ctx.zs, &tmp)) {
		goto fail;
	}

	info->mPort = (uint16_t)tmp;

	if (!zcbor_uint32_decode(ctx.zs, &tmp)) {
		goto fail;
	}

	info->mPriority = (uint16_t)tmp;

	if (!zcbor_uint32_decode(ctx.zs, &tmp)) {
		goto fail;
	}

	info->mWeight = (uint16_t)tmp;

	if (info->mHostNameBuffer) {
		if (!zcbor_tstr_decode(ctx.zs, &resp_str)) {
			goto fail;
		}

		memcpy(info->mHostNameBuffer, resp_str.value, resp_str.len);
	}

	if (!zcbor_bstr_decode(ctx.zs, &resp_str)) {
		goto fail;
	}

	memcpy(&info->mHostAddress, resp_str.value, resp_str.len);

	if (!zcbor_uint32_decode(ctx.zs, &info->mHostAddressTtl)) {
		goto fail;
	}

	if (info->mTxtData) {
		if (!zcbor_bstr_decode(ctx.zs, &resp_str)) {
			goto fail;
		}

		info->mTxtDataSize = resp_str.len;
		memcpy(info->mTxtData, resp_str.value, resp_str.len);
	}

	if (!zcbor_bool_decode(ctx.zs, &info->mTxtDataTruncated)) {
		goto fail;
	}

	if (!zcbor_uint32_decode(ctx.zs, &info->mTxtDataTtl)) {
		goto fail;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return OT_ERROR_NONE;

fail:
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
	ot_rpc_report_decoding_error(cmd);

	return OT_ERROR_NO_BUFS;
}

otError otDnsClientResolveAddress(otInstance *aInstance, const char *aHostName,
				  otDnsAddressCallback aCallback, void *aContext,
				  const otDnsQueryConfig *aConfig)
{
	ARG_UNUSED(aInstance);

	return resolve_or_browse(OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS, aHostName, NULL, aCallback,
				 aContext, aConfig);
}

otError otDnsClientResolveIp4Address(otInstance *aInstance, const char *aHostName,
				     otDnsAddressCallback aCallback, void *aContext,
				     const otDnsQueryConfig *aConfig)
{
	ARG_UNUSED(aInstance);

	return resolve_or_browse(OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS, aHostName, NULL,
				 aCallback, aContext, aConfig);
}

otError otDnsAddressResponseGetHostName(const otDnsAddressResponse *aResponse, char *aNameBuffer,
					uint16_t aNameBufferSize)
{
	return fetch_strings_from_dns_response(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME,
					      (const void *)aResponse, NULL, aNameBuffer,
					      aNameBufferSize, NULL, 0);
}

otError otDnsAddressResponseGetAddress(const otDnsAddressResponse *aResponse, uint16_t aIndex,
				       otIp6Address *aAddress, uint32_t *aTtl)
{
	return fetch_host_address_from_response(OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS,
						(const void *)aResponse, NULL, aIndex, aAddress,
						aTtl);
}

otError otDnsClientBrowse(otInstance *aInstance, const char *aServiceName,
			  otDnsBrowseCallback aCallback, void *aContext,
			  const otDnsQueryConfig *aConfig)
{
	ARG_UNUSED(aInstance);

	return resolve_or_browse(OT_RPC_CMD_DNS_CLIENT_BROWSE, aServiceName, NULL, aCallback,
				 aContext, aConfig);
}

otError otDnsBrowseResponseGetServiceName(const otDnsBrowseResponse *aResponse, char *aNameBuffer,
					  uint16_t aNameBufferSize)
{
	return fetch_strings_from_dns_response(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME,
					       (const void *)aResponse, NULL, aNameBuffer,
					       aNameBufferSize, NULL, 0);
}

otError otDnsBrowseResponseGetServiceInstance(const otDnsBrowseResponse *aResponse,
					      uint16_t aIndex, char *aLabelBuffer,
					      uint8_t aLabelBufferSize)
{
	return fetch_strings_from_dns_response(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE,
					       (const void *)aResponse, &aIndex, aLabelBuffer,
					       aLabelBufferSize, NULL, 0);
}

otError otDnsBrowseResponseGetServiceInfo(const otDnsBrowseResponse *aResponse,
					  const char *aInstanceLabel,
					  otDnsServiceInfo *aServiceInfo)
{
	return fetch_service_info_from_dns_response(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
						    (const void *)aResponse, aInstanceLabel,
						    aServiceInfo);
}

otError otDnsBrowseResponseGetHostAddress(const otDnsBrowseResponse *aResponse,
					  const char *aHostName, uint16_t aIndex,
					  otIp6Address *aAddress, uint32_t *aTtl)
{
	return fetch_host_address_from_response(OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS,
						(const void *)aResponse, aHostName, aIndex,
						aAddress, aTtl);
}

otError otDnsClientResolveService(otInstance *aInstance, const char *aInstanceLabel,
				  const char *aServiceName, otDnsServiceCallback aCallback,
				  void *aContext, const otDnsQueryConfig *aConfig)
{
	ARG_UNUSED(aInstance);

	return resolve_or_browse(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE, aInstanceLabel,
				 aServiceName, aCallback, aContext, aConfig);
}

otError otDnsClientResolveServiceAndHostAddress(otInstance *aInstance, const char *aInstanceLabel,
						const char *aServiceName,
						otDnsServiceCallback aCallback, void *aContext,
						const otDnsQueryConfig *aConfig)
{
	ARG_UNUSED(aInstance);

	return resolve_or_browse(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS,
				 aInstanceLabel, aServiceName, aCallback, aContext, aConfig);
}

otError otDnsServiceResponseGetServiceName(const otDnsServiceResponse *aResponse,
					   char *aLabelBuffer, uint8_t aLabelBufferSize,
					   char *aNameBuffer, uint16_t aNameBufferSize)
{
	return fetch_strings_from_dns_response(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME,
					       (void *)aResponse, NULL, aNameBuffer,
					       aNameBufferSize, aLabelBuffer, aLabelBufferSize);
}

otError otDnsServiceResponseGetServiceInfo(const otDnsServiceResponse *aResponse,
					   otDnsServiceInfo *aServiceInfo)
{
	return fetch_service_info_from_dns_response(OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO,
						    (const void *)aResponse, NULL, aServiceInfo);
}

otError otDnsServiceResponseGetHostAddress(const otDnsServiceResponse *aResponse,
					   const char *aHostName, uint16_t aIndex,
					   otIp6Address *aAddress, uint32_t *aTtl)
{
	return fetch_host_address_from_response(OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS,
						(const void *)aResponse, aHostName, aIndex,
						aAddress, aTtl);
}

static void decode_dns_callback(struct nrf_rpc_cbor_ctx *ctx, otError *error, const void **response,
				void **callback, void **context)
{
	*error = (otError)nrf_rpc_decode_uint(ctx);
	*response = (const void *)nrf_rpc_decode_uint(ctx);
	*context = (void *)nrf_rpc_decode_uint(ctx);
	*callback = (void *)nrf_rpc_decode_callback_call(ctx);
}

static void ot_rpc_dns_address_cb_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	otDnsAddressCallback callback;
	const otDnsAddressResponse *response;
	void *context;

	decode_dns_callback(ctx, &error, (const void **)&response, (void **)&callback, &context);

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (callback) {
		callback(error, response, context);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_address_cb, OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB,
			 ot_rpc_dns_address_cb_rpc_handler, NULL);

static void ot_rpc_dns_browse_cb_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	otDnsBrowseCallback callback;
	const otDnsBrowseResponse *response;
	void *context;

	decode_dns_callback(ctx, &error, (const void **)&response, (void **)&callback, &context);

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (callback) {
		callback(error, response, context);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_browse_cb, OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB,
			 ot_rpc_dns_browse_cb_rpc_handler, NULL);

static void ot_rpc_dns_service_cb_rpc_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	otDnsServiceCallback callback;
	const otDnsServiceResponse *response;
	void *context;

	decode_dns_callback(ctx, &error, (const void **)&response, (void **)&callback, &context);

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (callback) {
		callback(error, response, context);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_service_cb, OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB,
			 ot_rpc_dns_service_cb_rpc_handler, NULL);
