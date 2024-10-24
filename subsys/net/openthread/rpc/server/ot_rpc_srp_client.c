/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>
#include <zephyr/sys/slist.h>

#include <openthread/srp_client.h>
#include <openthread/srp_client_buffers.h>

#include <string.h>

struct host_data {
	char hostname[OT_DNS_MAX_NAME_SIZE];
	otIp6Address *addresses;
	size_t addresses_num;
};

struct service_data {
	sys_snode_t node;
	uintptr_t client_id;
	otSrpClientService service;
	uint8_t *buffer;
};

static struct host_data host_data;
static sys_slist_t service_data = SYS_SLIST_STATIC_INIT(&service_data);

static void host_data_clear(void)
{
	if (host_data.addresses) {
		free(host_data.addresses);
		host_data.addresses = NULL;
	}

	memset(&host_data, 0, sizeof(host_data));
}

static void service_data_clear(void)
{
	struct service_data *service;
	struct service_data *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&service_data, service, next, node) {
		if (service->buffer) {
			free(service->buffer);
		}

		free(service);
	}

	sys_slist_init(&service_data);
}

static struct service_data *service_data_find(uintptr_t client_id, struct service_data **prev)
{
	struct service_data *service;

	SYS_SLIST_FOR_EACH_CONTAINER(&service_data, service, node) {
		if (service->client_id == client_id) {
			return service;
		}

		if (prev) {
			*prev = service;
		}
	}

	return service;
}

static struct service_data *service_data_find_prev(struct service_data *service)
{
	sys_snode_t *prev;

	if (sys_slist_find(&service_data, &service->node, &prev)) {
		return CONTAINER_OF(prev, struct service_data, node);
	}

	return NULL;
}

static void service_buffers_free(struct service_data *service)
{
	if (service->buffer) {
		free(service->buffer);
	}

	free(service);
}

static void service_data_free(struct service_data *service, struct service_data *prev)
{
	sys_slist_remove(&service_data, (prev ? &prev->node : NULL), &service->node);

	service_buffers_free(service);
}

static size_t service_list_length(const otSrpClientService *services)
{
	size_t length = 0;

	for (const otSrpClientService *s = services; s != NULL; s = s->mNext) {
		++length;
	}

	return length;
}

static const char *decode_str(struct nrf_rpc_cbor_ctx *ctx, uint8_t **buffer_pos,
			      size_t *buffer_size)
{
	/* Decode string and update buffer position and current size */
	size_t str_size;
	const char* start;

	start = nrf_rpc_decode_str(ctx, *buffer_pos, *buffer_size);

	str_size = strlen(*buffer_pos) + 1;
	*buffer_size -= str_size;
	*buffer_pos += str_size;

	return start;
}

static void decode_subtypes(struct nrf_rpc_cbor_ctx *ctx, otSrpClientService *service,
			    uint8_t **buffer_pos, size_t *buffer_size)
{
	const char **ptrs = (const char **)*buffer_pos;
	size_t index = 0;

	zcbor_list_start_decode(ctx->zs);

	/* Prepare storage for array of pointers in the buffer */
	*buffer_pos += sizeof(const char *) * (ctx->zs->elem_count + 1);

	while (ctx->zs->elem_count) {
		ptrs[index++] = decode_str(ctx, buffer_pos, buffer_size);
	}

	ptrs[index] = NULL;

	zcbor_list_end_decode(ctx->zs);

	service->mSubTypeLabels = ptrs;
}

static void decode_txt_data(struct nrf_rpc_cbor_ctx *ctx, otSrpClientService *service,
			    uint8_t **buffer_pos, size_t *buffer_size)
{
	otDnsTxtEntry *entries = (otDnsTxtEntry *)*buffer_pos;
	const void *rpc_buffer;
	size_t rpc_buffer_size;
	size_t index = 0;

	zcbor_map_start_decode(ctx->zs);

	service->mNumTxtEntries = ctx->zs->elem_count;

	if (!ctx->zs->elem_count) {
		service->mTxtEntries = NULL;

		goto out;
	}

	*buffer_pos += ctx->zs->elem_count * sizeof(otDnsTxtEntry);

	while (ctx->zs->elem_count) {
		entries[index].mKey = decode_str(ctx, buffer_pos, buffer_size);
		rpc_buffer = nrf_rpc_decode_buffer_ptr_and_size(ctx, &rpc_buffer_size);

		memcpy(*buffer_pos, rpc_buffer, rpc_buffer_size);

		entries[index].mValueLength = (uint16_t)rpc_buffer_size;
		*buffer_pos += rpc_buffer_size;

		index++;
	}

out:
	zcbor_map_end_decode(ctx->zs);
}

static void ot_rpc_cmd_srp_client_add_service(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	uintptr_t client_service_id;
	struct service_data *service;
	uint8_t *buffer_pos;
	size_t buffer_size = 0;

	service = malloc(sizeof(*service));

	if (!service) {
		error = OT_ERROR_NO_BUFS;
		goto out;
	}

	client_service_id = nrf_rpc_decode_uint(ctx);

	/* Add two bytes as nrf_rpc_decode_str sets NULL char to name and instance */
	buffer_size = nrf_rpc_decode_uint(ctx) + 2;

	service->buffer = malloc(buffer_size);

	if (!service->buffer) {
		error = OT_ERROR_NO_BUFS;
		goto out;
	}

	buffer_pos = service->buffer;

	service->service.mName = (const char *)buffer_pos;
	decode_str(ctx, &buffer_pos, &buffer_size);

	service->service.mInstanceName = (const char *)buffer_pos;
	decode_str(ctx, &buffer_pos, &buffer_size);

	decode_subtypes(ctx, &service->service, &buffer_pos, &buffer_size);
	decode_txt_data(ctx, &service->service, &buffer_pos, &buffer_size);

	service->service.mPort = nrf_rpc_decode_uint(ctx);
	service->service.mPriority = nrf_rpc_decode_uint(ctx);
	service->service.mWeight = nrf_rpc_decode_uint(ctx);
	service->service.mLease = nrf_rpc_decode_uint(ctx);
	service->service.mKeyLease = nrf_rpc_decode_uint(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSrpClientAddService(openthread_get_default_instance(), &service->service);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error == OT_ERROR_NONE) {
		sys_slist_prepend(&service_data, &service->node);
	}

out:
	if (service != NULL && error != OT_ERROR_NONE) {
		service_buffers_free(service);
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_add_service,
			 OT_RPC_CMD_SRP_CLIENT_ADD_SERVICE, ot_rpc_cmd_srp_client_add_service,
			 NULL);

static void ot_rpc_cmd_srp_client_clear_host_and_services(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientClearHostAndServices(openthread_get_default_instance());
	host_data_clear();
	service_data_clear();
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_clear_host_and_services,
			 OT_RPC_CMD_SRP_CLIENT_CLEAR_HOST_AND_SERVICES,
			 ot_rpc_cmd_srp_client_clear_host_and_services, NULL);

static void ot_rpc_cmd_srp_client_clear_service(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uintptr_t client_service_id;
	struct service_data *service;
	struct service_data *prev;
	otError error;

	client_service_id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_CLEAR_SERVICE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	service = service_data_find(client_service_id, &prev);

	if (!service) {
		error = OT_ERROR_NOT_FOUND;
		goto out;
	}

	error = otSrpClientClearService(openthread_get_default_instance(), &service->service);
	service_data_free(service, prev);

out:
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_clear_service,
			 OT_RPC_CMD_SRP_CLIENT_CLEAR_SERVICE, ot_rpc_cmd_srp_client_clear_service,
			 NULL);

static void ot_rpc_cmd_srp_client_disable_auto_start_mode(const struct nrf_rpc_group *group,
							  struct nrf_rpc_cbor_ctx *ctx,
							  void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientDisableAutoStartMode(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_disable_auto_start_mode,
			 OT_RPC_CMD_SRP_CLIENT_DISABLE_AUTO_START_MODE,
			 ot_rpc_cmd_srp_client_disable_auto_start_mode, NULL);

static void ot_rpc_cmd_srp_client_enable_auto_host_addr(const struct nrf_rpc_group *group,
							struct nrf_rpc_cbor_ctx *ctx,
							void *handler_data)
{
	otError error;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSrpClientEnableAutoHostAddress(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (host_data.addresses) {
		free(host_data.addresses);

		host_data.addresses = NULL;
		host_data.addresses_num = 0;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_enable_auto_host_addr,
			 OT_RPC_CMD_SRP_CLIENT_ENABLE_AUTO_HOST_ADDR,
			 ot_rpc_cmd_srp_client_enable_auto_host_addr, NULL);

static void ot_rpc_srp_client_auto_start_cb(const otSockAddr *server_sock_addr, void *context)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	ARG_UNUSED(context);

	cbor_buffer_size += (1 + sizeof(server_sock_addr->mPort));
	cbor_buffer_size += (1 + OT_IP6_ADDRESS_SIZE);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_buffer(&ctx, server_sock_addr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_uint(&ctx, server_sock_addr->mPort);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_AUTO_START_CB, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void ot_rpc_cmd_srp_client_enable_auto_start_mode(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientEnableAutoStartMode(openthread_get_default_instance(),
				       ot_rpc_srp_client_auto_start_cb, NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_enable_auto_start_mode,
			 OT_RPC_CMD_SRP_CLIENT_ENABLE_AUTO_START_MODE,
			 ot_rpc_cmd_srp_client_enable_auto_start_mode, NULL);

static void ot_rpc_cmd_srp_client_remove_host_and_services(const struct nrf_rpc_group *group,
							   struct nrf_rpc_cbor_ctx *ctx,
							   void *handler_data)
{
	bool remove_key_lease;
	bool send_unreg_to_server;
	otError error;

	remove_key_lease = nrf_rpc_decode_bool(ctx);
	send_unreg_to_server = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_REMOVE_HOST_AND_SERVICES);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSrpClientRemoveHostAndServices(openthread_get_default_instance(),
						 remove_key_lease, send_unreg_to_server);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_remove_host_and_services,
			 OT_RPC_CMD_SRP_CLIENT_REMOVE_HOST_AND_SERVICES,
			 ot_rpc_cmd_srp_client_remove_host_and_services, NULL);

static void ot_rpc_cmd_srp_client_remove_service(const struct nrf_rpc_group *group,
						 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uintptr_t client_service_id;
	struct service_data *service;
	otError error;

	client_service_id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_REMOVE_HOST_AND_SERVICES);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	service = service_data_find(client_service_id, NULL);

	if (!service) {
		error = OT_ERROR_NOT_FOUND;
		goto out;
	}

	error = otSrpClientRemoveService(openthread_get_default_instance(), &service->service);

out:
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_remove_service,
			 OT_RPC_CMD_SRP_CLIENT_REMOVE_SERVICE, ot_rpc_cmd_srp_client_remove_service,
			 NULL);

static void ot_rpc_srp_client_callback(otError error, const otSrpClientHostInfo *host_info,
				       const otSrpClientService *services,
				       const otSrpClientService *removed_services, void *context)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	ARG_UNUSED(context);

	cbor_buffer_size += (1 + sizeof(error));
	cbor_buffer_size += 1; /* host_info->mState */
	cbor_buffer_size += (2 + sizeof(uintptr_t)) * service_list_length(services);
	cbor_buffer_size += (2 + sizeof(uintptr_t)) * service_list_length(removed_services);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, error);
	nrf_rpc_encode_uint(&ctx, host_info->mState);

	for (const otSrpClientService *s = services; s != NULL; s = s->mNext) {
		const struct service_data *sd = CONTAINER_OF(s, struct service_data, service);

		nrf_rpc_encode_uint(&ctx, sd->client_id);
		nrf_rpc_encode_uint(&ctx, s->mState);
	}

	for (const otSrpClientService *s = removed_services; s != NULL; s = s->mNext) {
		struct service_data *sd = CONTAINER_OF(s, struct service_data, service);
		struct service_data *prev = service_data_find_prev(sd);

		nrf_rpc_encode_uint(&ctx, sd->client_id);
		nrf_rpc_encode_uint(&ctx, s->mState);

		service_data_free(sd, prev);
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SRP_CLIENT_CB, &ctx, nrf_rpc_rsp_decode_void,
				NULL);
}

static void ot_rpc_cmd_srp_client_set_callback(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool callback_set;

	callback_set = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_SET_CALLBACK);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientSetCallback(openthread_get_default_instance(),
			       callback_set ? ot_rpc_srp_client_callback : NULL, NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_callback,
			 OT_RPC_CMD_SRP_CLIENT_SET_CALLBACK, ot_rpc_cmd_srp_client_set_callback,
			 NULL);

static void ot_rpc_cmd_srp_client_set_hostname(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char hostname[OT_DNS_MAX_NAME_SIZE];
	otError error;

	nrf_rpc_decode_str(ctx, hostname, sizeof(hostname));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_SET_HOSTNAME);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	strcpy(host_data.hostname, hostname);
	error = otSrpClientSetHostName(openthread_get_default_instance(), host_data.hostname);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_hostname,
			 OT_RPC_CMD_SRP_CLIENT_SET_HOSTNAME, ot_rpc_cmd_srp_client_set_hostname,
			 NULL);

static void ot_rpc_cmd_srp_client_set_key_lease_interval(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	uint32_t interval;

	interval = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_SET_KEY_LEASE_INTERVAL);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientSetKeyLeaseInterval(openthread_get_default_instance(), interval);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_key_lease_interval,
			 OT_RPC_CMD_SRP_CLIENT_SET_KEY_LEASE_INTERVAL,
			 ot_rpc_cmd_srp_client_set_key_lease_interval, NULL);

static void ot_rpc_cmd_srp_client_set_lease_interval(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t interval;

	interval = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_SET_LEASE_INTERVAL);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientSetLeaseInterval(openthread_get_default_instance(), interval);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_lease_interval,
			 OT_RPC_CMD_SRP_CLIENT_SET_LEASE_INTERVAL,
			 ot_rpc_cmd_srp_client_set_lease_interval, NULL);

static void ot_rpc_cmd_srp_client_set_ttl(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t ttl;

	ttl = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_SET_TTL);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientSetTtl(openthread_get_default_instance(), ttl);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_ttl, OT_RPC_CMD_SRP_CLIENT_SET_TTL,
			 ot_rpc_cmd_srp_client_set_ttl, NULL);

static void ot_rpc_cmd_srp_client_start(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otSockAddr sockaddr;
	otError error;

	ot_rpc_decode_sockaddr(ctx, &sockaddr);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_SRP_CLIENT_START);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSrpClientStart(openthread_get_default_instance(), &sockaddr);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_start, OT_RPC_CMD_SRP_CLIENT_START,
			 ot_rpc_cmd_srp_client_start, NULL);

static void ot_rpc_cmd_srp_client_stop(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otSrpClientStop(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_stop, OT_RPC_CMD_SRP_CLIENT_STOP,
			 ot_rpc_cmd_srp_client_stop, NULL);

static void ot_rpc_cmd_srp_client_is_running(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool running;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	running = otSrpClientIsRunning(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_bool(group, running);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_is_running,
			 OT_RPC_CMD_SRP_CLIENT_IS_RUNNING, ot_rpc_cmd_srp_client_is_running, NULL);

static void ot_rpc_cmd_srp_client_get_server_address(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otSockAddr *sockaddr;

	nrf_rpc_cbor_decoding_done(group, ctx);

	NRF_RPC_CBOR_ALLOC(&ot_group, rsp_ctx, OT_IP6_ADDRESS_SIZE + sizeof(uint32_t) + 2);

	openthread_api_mutex_lock(openthread_get_default_context());
	sockaddr = otSrpClientGetServerAddress(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	ot_rpc_encode_sockaddr(&rsp_ctx, sockaddr);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_get_server_address,
			 OT_RPC_CMD_SRP_CLIENT_GET_SERVER_ADDRESS,
			 ot_rpc_cmd_srp_client_get_server_address, NULL);

static void ot_rpc_cmd_srp_client_get_ttl(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t ttl;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	ttl = otSrpClientGetTtl(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, ttl);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_get_ttl,
			 OT_RPC_CMD_SRP_CLIENT_GET_TTL, ot_rpc_cmd_srp_client_get_ttl, NULL);

static void ot_rpc_cmd_srp_client_get_lease_interval(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t lease_int;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	lease_int = otSrpClientGetLeaseInterval(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, lease_int);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_get_lease_interval,
			 OT_RPC_CMD_SRP_CLIENT_GET_LEASE_INTERVAL,
			 ot_rpc_cmd_srp_client_get_lease_interval,
			 NULL);

static void ot_rpc_cmd_srp_client_get_key_lease_interval(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	uint32_t key_lease_int;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	key_lease_int = otSrpClientGetKeyLeaseInterval(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, key_lease_int);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_get_key_lease_interval,
			 OT_RPC_CMD_SRP_CLIENT_GET_KEY_LEASE_INTERVAL,
			 ot_rpc_cmd_srp_client_get_key_lease_interval, NULL);


static void ot_rpc_cmd_srp_client_set_host_addresses(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	otError error;
	size_t index = 0;

	zcbor_list_start_decode(ctx->zs);

	/* Check if more memory is needed */
	if (host_data.addresses && (ctx->zs->elem_count > host_data.addresses_num)) {
		free(host_data.addresses);
		host_data.addresses = malloc(ctx->zs->elem_count * OT_IP6_ADDRESS_SIZE);
	} else if (!host_data.addresses) {
		host_data.addresses = malloc(ctx->zs->elem_count * OT_IP6_ADDRESS_SIZE);
	}

	host_data.addresses_num = ctx->zs->elem_count;

	while (ctx->zs->elem_count) {
		nrf_rpc_decode_buffer(ctx, &host_data.addresses[index++], OT_IP6_ADDRESS_SIZE);
	}

	zcbor_list_end_decode(ctx->zs);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otSrpClientSetHostAddresses(openthread_get_default_instance(), host_data.addresses,
					    host_data.addresses_num);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_set_host_addresses,
			 OT_RPC_CMD_SRP_CLIENT_SET_HOST_ADDRESSES,
			 ot_rpc_cmd_srp_client_set_host_addresses, NULL);

static void ot_rpc_cmd_srp_client_is_auto_start_mode_enabled(const struct nrf_rpc_group *group,
							     struct nrf_rpc_cbor_ctx *ctx,
							     void *handler_data)
{
	bool enabled;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	enabled = otSrpClientIsAutoStartModeEnabled(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_bool(group, enabled);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_srp_client_is_auto_start_mode_enabled,
			 OT_RPC_CMD_SRP_CLIENT_IS_AUTO_START_MODE_ENABLED,
			 ot_rpc_cmd_srp_client_is_auto_start_mode_enabled, NULL);



static int ot_rpc_srp_client_init(void)
{
	memset(&host_data, 0, sizeof(host_data));

	return 0;
}

SYS_INIT(ot_rpc_srp_client_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
