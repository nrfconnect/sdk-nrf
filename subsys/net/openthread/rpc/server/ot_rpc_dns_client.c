/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <zephyr/net/openthread.h>

static void ot_rpc_dns_client_get_default_config(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const otDnsQueryConfig *config;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();

	config = otDnsClientGetDefaultConfig(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, OT_RPC_DNS_QUERY_CONFIG_SIZE);

	ot_rpc_encode_dns_query_config(&rsp_ctx, config);

	ot_rpc_mutex_unlock();

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_dns_client_set_default_config(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otDnsQueryConfig config;
	bool decoded_config;

	decoded_config = ot_rpc_decode_dns_query_config(ctx, &config);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG);
		return;
	}

	ot_rpc_mutex_lock();

	otDnsClientSetDefaultConfig(openthread_get_default_instance(),
				    (decoded_config ? &config : NULL));

	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

static void address_response_callback(otError error, const otDnsAddressResponse *response,
				      void *context, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, ((2 * sizeof(uint32_t)) + 2) +
					   ((2 * sizeof(uintptr_t)) + 2));

	nrf_rpc_encode_uint(&ctx, (uint32_t) error);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)response);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DNS_ADDRESS_RESPONSE_CB, &ctx,
				ot_rpc_decode_void, NULL);
}


NRF_RPC_CBKPROXY_HANDLER(address_response_callback_encoder, address_response_callback,
			 (otError error, const otDnsAddressResponse *response, void *context),
			 (error, response, context));

static void browse_response_callback(otError error, const otDnsBrowseResponse *response,
				     void *context, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, ((2 * sizeof(uint32_t)) + 2) +
					   ((2 * sizeof(uintptr_t)) + 2));

	nrf_rpc_encode_uint(&ctx, (uint32_t) error);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)response);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DNS_BROWSE_RESPONSE_CB, &ctx,
				ot_rpc_decode_void, NULL);
}

NRF_RPC_CBKPROXY_HANDLER(browse_response_callback_encoder, browse_response_callback,
			 (otError error, const otDnsBrowseResponse *response, void *context),
			 (error, response, context));

static void service_response_callback(otError error, const otDnsServiceResponse *response,
				     void *context, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, ((2 * sizeof(uint32_t)) + 2) +
					   ((2 * sizeof(uintptr_t)) + 2));

	nrf_rpc_encode_uint(&ctx, (uint32_t) error);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)response);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DNS_SERVICE_RESPONSE_CB, &ctx,
				ot_rpc_decode_void, NULL);
}

NRF_RPC_CBKPROXY_HANDLER(service_response_callback_encoder, service_response_callback,
			 (otError error, const otDnsServiceResponse *response, void *context),
			 (error, response, context));

static void resolve_address(enum ot_rpc_cmd_server cmd, const struct nrf_rpc_group *group,
			    struct nrf_rpc_cbor_ctx *ctx)
{
	otDnsQueryConfig config;
	otDnsAddressCallback cb;
	otError error = OT_ERROR_FAILED;
	char hostname[OT_DNS_MAX_NAME_SIZE];
	void *cb_ctx;
	bool decoded_config;

	nrf_rpc_decode_str(ctx, hostname, sizeof(hostname));

	cb = (otDnsAddressCallback)nrf_rpc_decode_callbackd(ctx, address_response_callback_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	decoded_config = ot_rpc_decode_dns_query_config(ctx, &config);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(cmd);
		return;
	}

	ot_rpc_mutex_lock();

	switch (cmd) {
	case OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS:
		error = otDnsClientResolveAddress(openthread_get_default_instance(), hostname, cb,
						  cb_ctx, (decoded_config ? &config : NULL));
		break;
	case OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS:
		error = otDnsClientResolveIp4Address(openthread_get_default_instance(), hostname,
						     cb, cb_ctx, (decoded_config ? &config : NULL));
		break;
	default:
		error = OT_ERROR_INVALID_COMMAND;
		break;
	}

	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_dns_client_resolve_address(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ARG_UNUSED(handler_data);

	resolve_address(OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS, group, ctx);
}

static void ot_rpc_dns_client_resolve_ip4_address(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ARG_UNUSED(handler_data);

	resolve_address(OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS, group, ctx);
}

static void ot_rpc_dns_client_browse(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char service_name[OT_DNS_MAX_NAME_SIZE];
	otDnsQueryConfig config;
	otDnsBrowseCallback cb;
	void *cb_ctx;
	bool decoded_config;
	otError error;

	nrf_rpc_decode_str(ctx, service_name, sizeof(service_name));

	cb = (otDnsBrowseCallback)nrf_rpc_decode_callbackd(ctx, browse_response_callback_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	decoded_config = ot_rpc_decode_dns_query_config(ctx, &config);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_DNS_CLIENT_BROWSE);
		return;
	}

	ot_rpc_mutex_lock();

	error = otDnsClientBrowse(openthread_get_default_instance(), service_name, cb, cb_ctx,
				  (decoded_config ? &config : NULL));

	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

static void resolve_service(enum ot_rpc_cmd_server cmd, const struct nrf_rpc_group *group,
			    struct nrf_rpc_cbor_ctx *ctx)
{
	char instance[OT_DNS_MAX_LABEL_SIZE];
	char service[OT_DNS_MAX_NAME_SIZE];
	otDnsQueryConfig config;
	otDnsServiceCallback cb;
	void *cb_ctx;
	bool decoded_config;
	otError error;

	nrf_rpc_decode_str(ctx, instance, sizeof(instance));
	nrf_rpc_decode_str(ctx, service, sizeof(service));

	cb = (otDnsServiceCallback)nrf_rpc_decode_callbackd(ctx, service_response_callback_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	decoded_config = ot_rpc_decode_dns_query_config(ctx, &config);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(cmd);
		return;
	}

	ot_rpc_mutex_lock();

	switch (cmd) {
	case OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE:
		error = otDnsClientResolveService(openthread_get_default_instance(), instance,
						  service, cb, cb_ctx,
						  (decoded_config ? &config : NULL));
		break;
	case OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS:
		error = otDnsClientResolveServiceAndHostAddress(openthread_get_default_instance(),
								instance, service, cb, cb_ctx,
								(decoded_config ? &config : NULL));
		break;
	default:
		error = OT_ERROR_INVALID_COMMAND;
		break;
	}

	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_dns_client_resolve_service(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ARG_UNUSED(handler_data);

	resolve_service(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE, group, ctx);
}

static void ot_rpc_dns_client_resolve_service_and_host_address(const struct nrf_rpc_group *group,
							       struct nrf_rpc_cbor_ctx *ctx,
							       void *handler_data)
{
	ARG_UNUSED(handler_data);

	resolve_service(OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS, group, ctx);
}

static void resp_get_name(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  uint8_t cmd)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const void *response;
	uint32_t max_name_len;
	uint32_t max_label_len;
	otError error;
	char label[OT_DNS_MAX_LABEL_SIZE];
	char name_buffer[OT_DNS_MAX_NAME_SIZE];
	size_t resp_len = 2; /* otError */
	size_t name_len = 0;
	size_t label_len = 0;

	response = (const void *)nrf_rpc_decode_uint(ctx);
	max_name_len = nrf_rpc_decode_uint(ctx);

	if (cmd == OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME) {
		max_label_len = nrf_rpc_decode_uint(ctx);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(cmd);
		return;
	}

	ot_rpc_mutex_lock();

	switch (cmd) {
	case OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME:
		error = otDnsAddressResponseGetHostName((const otDnsAddressResponse *)response,
							name_buffer, max_name_len);
		break;
	case OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME:
		error = otDnsBrowseResponseGetServiceName((const otDnsBrowseResponse *)response,
							  name_buffer, max_name_len);
		break;
	case OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME:
		error = otDnsServiceResponseGetServiceName((const otDnsServiceResponse *)response,
							   label, max_label_len, name_buffer,
							   max_name_len);
		break;
	default:
		error = OT_ERROR_INVALID_COMMAND;
		break;
	}

	ot_rpc_mutex_unlock();

	if (error == OT_ERROR_NONE) {
		name_len = strlen(name_buffer);
		resp_len += name_len + 2;

		if (cmd == OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME) {
			label_len = strlen(label);
			resp_len += name_len + 2;
		}
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, resp_len);

	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_str(&rsp_ctx, name_buffer, name_len);

		if (cmd == OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME) {
			nrf_rpc_encode_str(&rsp_ctx, label, label_len);
		}
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void resp_get_address(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     uint8_t cmd)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	char hostname[OT_DNS_MAX_NAME_SIZE];
	const void *response;
	otError error;
	otIp6Address address;
	uint32_t index;
	uint32_t ttl;
	size_t resp_len;

	response = (const void *)nrf_rpc_decode_uint(ctx);
	index = nrf_rpc_decode_uint(ctx);

	if (cmd != OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS) {
		nrf_rpc_decode_str(ctx, hostname, sizeof(hostname));
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(cmd);
		return;
	}

	ot_rpc_mutex_lock();

	switch (cmd) {
	case OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS:
		error = otDnsAddressResponseGetAddress((const otDnsAddressResponse *)response,
						      (uint16_t)index, &address, &ttl);
		break;
	case OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS:
		error = otDnsBrowseResponseGetHostAddress((const otDnsBrowseResponse *)response,
							  hostname, (uint16_t)index, &address,
							  &ttl);
		break;
	case OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS:
		error = otDnsServiceResponseGetHostAddress((const otDnsServiceResponse *)response,
							   hostname, (uint16_t)index, &address,
							   &ttl);
		break;
	default:
		error = OT_ERROR_INVALID_COMMAND;
		break;
	}

	ot_rpc_mutex_unlock();

	resp_len = sizeof(uint32_t) + 1;
	resp_len += (error == OT_ERROR_NONE) ? sizeof(otIp6Address) + sizeof(uint32_t) + 2 : 0;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, resp_len);

	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_buffer(&rsp_ctx, &address, sizeof(otIp6Address));
		nrf_rpc_encode_uint(&rsp_ctx, ttl);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void resp_get_info(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  uint8_t cmd)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const void *response;
	uint32_t error;
	uint32_t max_name_size;
	uint32_t max_txt_size;
	otDnsServiceInfo info;
	char instance[OT_DNS_MAX_LABEL_SIZE];
	char name[OT_DNS_MAX_NAME_SIZE];
	char txt[CONFIG_OPENTHREAD_RPC_DNS_MAX_TXT_DATA_SIZE];
	size_t alloc_len = 6 * (sizeof(uint32_t) + 1) + sizeof(info.mHostAddress) + 1;
	size_t name_size = 0;

	memset(&info, 0, sizeof(otDnsServiceInfo));

	response = (const void *)nrf_rpc_decode_uint(ctx);
	max_name_size = nrf_rpc_decode_uint(ctx);
	max_txt_size = nrf_rpc_decode_uint(ctx);

	if (cmd == OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO) {
		nrf_rpc_decode_str(ctx, instance, sizeof(instance));
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(cmd);
		return;
	}

	if (max_name_size) {
		info.mHostNameBuffer = name;
		info.mHostNameBufferSize = max_name_size;
	}

	if (max_txt_size) {
		info.mTxtData = txt;
		info.mTxtDataSize = max_txt_size;
	}

	ot_rpc_mutex_lock();

	switch (cmd) {
	case OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO:
		error = otDnsBrowseResponseGetServiceInfo((const otDnsBrowseResponse *)response,
							  instance, &info);
		break;
	case OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO:
		error = otDnsServiceResponseGetServiceInfo((const otDnsServiceResponse *)response,
							   &info);
		break;
	default:
		error = OT_ERROR_INVALID_COMMAND;
		break;
	}

	ot_rpc_mutex_unlock();

	if (max_name_size) {
		name_size = strlen(name);
		alloc_len += name_size + 2;
	}

	if (max_txt_size) {
		alloc_len += info.mTxtDataSize + 3;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, alloc_len + 11);

	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_uint(&rsp_ctx, info.mTtl);
		nrf_rpc_encode_uint(&rsp_ctx, info.mPort);
		nrf_rpc_encode_uint(&rsp_ctx, info.mPriority);
		nrf_rpc_encode_uint(&rsp_ctx, info.mWeight);

		if (max_name_size) {
			nrf_rpc_encode_str(&rsp_ctx, name, name_size);
		}

		nrf_rpc_encode_buffer(&rsp_ctx, &info.mHostAddress, sizeof(info.mHostAddress));
		nrf_rpc_encode_uint(&rsp_ctx, info.mHostAddressTtl);

		if (max_txt_size) {
			nrf_rpc_encode_buffer(&rsp_ctx, info.mTxtData, info.mTxtDataSize);
		}

		nrf_rpc_encode_bool(&rsp_ctx, info.mTxtDataTruncated);
		nrf_rpc_encode_uint(&rsp_ctx, info.mTxtDataTtl);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_dns_address_response_get_host_name(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_name(group, ctx, OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME);
}

static void ot_rpc_dns_address_response_get_host_address(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_address(group, ctx, OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS);
}

static void ot_rpc_dns_browse_response_get_service_name(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_name(group, ctx, OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME);
}

static void ot_rpc_dns_browse_response_get_service_instance(const struct nrf_rpc_group *group,
							    struct nrf_rpc_cbor_ctx *ctx,
							    void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otDnsBrowseResponse *response;
	char instance[OT_DNS_MAX_LABEL_SIZE];
	uint16_t index;
	uint32_t max_len;
	otError error;
	size_t name_len;
	size_t resp_len = sizeof(uint32_t) + 1;

	response = (const void *)nrf_rpc_decode_uint(ctx);
	index = nrf_rpc_decode_uint(ctx);
	max_len = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE);
		return;
	}

	ot_rpc_mutex_lock();

	error = otDnsBrowseResponseGetServiceInstance(response, index, instance, max_len);

	ot_rpc_mutex_unlock();

	if (error == OT_ERROR_NONE) {
		name_len = strlen(instance);
		resp_len += name_len + 2;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, resp_len);

	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_str(&rsp_ctx, instance, name_len);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_dns_browse_response_get_service_info(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_info(group, ctx, OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO);
}

static void ot_rpc_dns_browse_response_get_host_address(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_address(group, ctx, OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS);
}


static void ot_rpc_dns_service_response_get_service_name(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_name(group, ctx, OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME);
}

static void ot_rpc_dns_service_response_get_host_address(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_address(group, ctx, OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS);
}

static void ot_rpc_dns_service_response_get_service_info(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	ARG_UNUSED(handler_data);

	resp_get_info(group, ctx, OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_get_default_config,
			 OT_RPC_CMD_DNS_CLIENT_GET_DEFAULT_CONFIG,
			 ot_rpc_dns_client_get_default_config, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_set_default_config,
			 OT_RPC_CMD_DNS_CLIENT_SET_DEFAULT_CONFIG,
			 ot_rpc_dns_client_set_default_config, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_resolve_address,
			 OT_RPC_CMD_DNS_CLIENT_RESOLVE_ADDRESS,
			 ot_rpc_dns_client_resolve_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_resolve_ip4_address,
			 OT_RPC_CMD_DNS_CLIENT_RESOLVE_IP4_ADDRESS,
			 ot_rpc_dns_client_resolve_ip4_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_browse, OT_RPC_CMD_DNS_CLIENT_BROWSE,
			 ot_rpc_dns_client_browse, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_resolve_service,
			 OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE, ot_rpc_dns_client_resolve_service,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_client_resolve_service_and_host_address,
			 OT_RPC_CMD_DNS_CLIENT_RESOLVE_SERVICE_AND_HOST_ADDRESS,
			 ot_rpc_dns_client_resolve_service_and_host_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_address_response_get_host_name,
			 OT_RPC_CMD_DNS_ADDRESS_RESP_GET_HOST_NAME,
			 ot_rpc_dns_address_response_get_host_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_address_response_get_host_address,
			 OT_RPC_CMD_DNS_ADDRESS_RESP_GET_ADDRESS,
			 ot_rpc_dns_address_response_get_host_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_browse_response_get_service_name,
			 OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_NAME,
			 ot_rpc_dns_browse_response_get_service_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_browse_response_get_service_instance,
			 OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INSTANCE,
			 ot_rpc_dns_browse_response_get_service_instance, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_browse_response_get_service_info,
			 OT_RPC_CMD_DNS_BROWSE_RESP_GET_SERVICE_INFO,
			 ot_rpc_dns_browse_response_get_service_info, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_browse_response_get_host_address,
			 OT_RPC_CMD_DNS_BROWSE_RESP_GET_HOST_ADDRESS,
			 ot_rpc_dns_browse_response_get_host_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_service_response_get_service_name,
			 OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_NAME,
			 ot_rpc_dns_service_response_get_service_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_service_response_get_service_info,
			 OT_RPC_CMD_DNS_SERVICE_RESP_GET_SERVICE_INFO,
			 ot_rpc_dns_service_response_get_service_info, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dns_service_response_get_host_address,
			 OT_RPC_CMD_DNS_SERVICE_RESP_GET_HOST_ADDRESS,
			 ot_rpc_dns_service_response_get_host_address, NULL);
