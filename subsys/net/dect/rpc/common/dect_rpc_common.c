/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dect_rpc_common.h"
#include "dect_rpc_ids.h"
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <limits.h>

#if defined(CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT)
#define DECT_RPC_STATUS_CHILD_SLOTS CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT
#else
#define DECT_RPC_STATUS_CHILD_SLOTS 1
#endif

LOG_MODULE_REGISTER(dect_rpc, CONFIG_NET_DECT_RPC_LOG_LEVEL);

void dect_rpc_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  void *handler_data)
{
	nrf_rpc_rsp_decode_void(group, ctx, handler_data);
}

void dect_rpc_decode_if_send_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	nrf_rpc_rsp_decode_i32(group, ctx, handler_data);
}

void dect_rpc_int32_rsp_encode(struct nrf_rpc_cbor_ctx *ctx, int32_t status)
{
	nrf_rpc_encode_int(ctx, status);
}

bool dect_rpc_int32_rsp_decode(struct nrf_rpc_cbor_ctx *ctx, int32_t *status)
{
	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	*status = nrf_rpc_decode_int(ctx);
	return true;
}

void dect_rpc_report_cmd_decoding_error(uint8_t cmd_evt_id)
{
	LOG_ERR("DECT NR+ RPC command decoding error: %u", cmd_evt_id);
}

void dect_rpc_decode_skip_extra_ipv6_addrs(struct nrf_rpc_cbor_ctx *ctx, uint32_t claimed,
					   uint32_t applied)
{
	for (uint32_t i = applied; i < claimed; i++) {
		size_t sz;

		if (!nrf_rpc_decode_valid(ctx)) {
			return;
		}
		if (nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz) == NULL) {
			return;
		}
	}
}

void dect_rpc_decode_skip_extra_ipv6_prefixes(struct nrf_rpc_cbor_ctx *ctx, uint32_t claimed,
					      uint32_t applied)
{
	for (uint32_t i = applied; i < claimed; i++) {
		size_t sz;

		if (!nrf_rpc_decode_valid(ctx)) {
			return;
		}
		if (nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz) == NULL) {
			return;
		}
		(void)nrf_rpc_decode_uint(ctx);
	}
}

void dect_rpc_decode_skip_status_association(struct nrf_rpc_cbor_ctx *ctx)
{
	size_t sz;

	if (!nrf_rpc_decode_valid(ctx)) {
		return;
	}
	(void)nrf_rpc_decode_uint(ctx);
	if (nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz) == NULL) {
		return;
	}
	if (!nrf_rpc_decode_valid(ctx)) {
		return;
	}
	if (nrf_rpc_decode_bool(ctx)) {
		(void)nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
	} else {
		nrf_rpc_decode_skip(ctx);
	}
}

static void dect_rpc_status_encode_association(struct nrf_rpc_cbor_ctx *ctx,
					       const struct dect_association_data *assoc)
{
	nrf_rpc_encode_uint(ctx, assoc->long_rd_id);
	nrf_rpc_encode_buffer(ctx, &assoc->local_ipv6_addr, sizeof(struct in6_addr));
	nrf_rpc_encode_bool(ctx, assoc->global_ipv6_addr_set);
	nrf_rpc_encode_buffer(ctx, &assoc->global_ipv6_addr, sizeof(struct in6_addr));
}

static bool dect_rpc_status_decode_association(struct nrf_rpc_cbor_ctx *ctx,
					       struct dect_association_data *assoc)
{
	const uint8_t *p;
	size_t sz;

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	assoc->long_rd_id = nrf_rpc_decode_uint(ctx);
	p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
	if (!p || sz < sizeof(struct in6_addr)) {
		return false;
	}
	memcpy(&assoc->local_ipv6_addr, p, sizeof(struct in6_addr));

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	if (nrf_rpc_decode_bool(ctx)) {
		assoc->global_ipv6_addr_set = true;
		p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
		if (!p || sz < sizeof(struct in6_addr)) {
			return false;
		}
		memcpy(&assoc->global_ipv6_addr, p, sizeof(struct in6_addr));
	} else {
		assoc->global_ipv6_addr_set = false;
		memset(&assoc->global_ipv6_addr, 0, sizeof(assoc->global_ipv6_addr));
		(void)nrf_rpc_decode_skip(ctx);
	}

	return true;
}

void dect_rpc_status_req_encode(struct nrf_rpc_cbor_ctx *ctx, uint32_t seq)
{
	nrf_rpc_encode_uint(ctx, seq);
}

void dect_rpc_status_init_empty(struct dect_status_info *status)
{
	memset(status, 0, sizeof(*status));
	status->br_net_iface = NULL;
	status->fw_version_str[0] = '\0';
}

size_t dect_rpc_status_rsp_encode_size(const struct dect_status_info *status)
{
	int n_child = status->child_count;
	size_t fw_len;

	if (n_child > DECT_RPC_MAX_STATUS_CHILDREN) {
		n_child = DECT_RPC_MAX_STATUS_CHILDREN;
	}
	fw_len = strnlen(status->fw_version_str, sizeof(status->fw_version_str));
	if (fw_len == 0) {
		fw_len = 1;
	}

	return 4 + 2 + 4 + 2 + 2 + (status->parent_count > 0 ? (4 + 16 + 1 + 16) : 0) +
	       n_child * (4 + 16 + 1 + 16) + 1 + 16 + 4 + 4 + fw_len;
}

void dect_rpc_status_rsp_encode(struct nrf_rpc_cbor_ctx *ctx, const struct dect_status_info *status)
{
	int n_child = status->child_count;
	size_t fw_len;
	int i;

	if (n_child > DECT_RPC_MAX_STATUS_CHILDREN) {
		n_child = DECT_RPC_MAX_STATUS_CHILDREN;
	}
	fw_len = strnlen(status->fw_version_str, sizeof(status->fw_version_str));

	nrf_rpc_encode_bool(ctx, status->mdm_activated);
	nrf_rpc_encode_bool(ctx, status->cluster_running);
	nrf_rpc_encode_uint(ctx, (uint32_t)status->cluster_channel);
	nrf_rpc_encode_bool(ctx, status->nw_beacon_running);
	nrf_rpc_encode_uint(ctx, (uint32_t)status->parent_count);

	for (i = 0; i < status->parent_count && i < 1; i++) {
		dect_rpc_status_encode_association(ctx, &status->parent_associations[i]);
	}

	nrf_rpc_encode_uint(ctx, (uint32_t)n_child);
	for (i = 0; i < n_child; i++) {
		dect_rpc_status_encode_association(ctx, &status->child_associations[i]);
	}

	nrf_rpc_encode_bool(ctx, status->br_global_ipv6_addr_prefix_set);
	nrf_rpc_encode_buffer(ctx, &status->br_global_ipv6_addr_prefix, sizeof(struct in6_addr));
	nrf_rpc_encode_uint(ctx, (uint32_t)status->br_global_ipv6_addr_prefix_len);
	if (fw_len == 0) {
		nrf_rpc_encode_buffer(ctx, (const uint8_t *)"", 1);
	} else {
		nrf_rpc_encode_buffer(ctx, status->fw_version_str, fw_len);
	}
}

bool dect_rpc_status_rsp_decode(struct nrf_rpc_cbor_ctx *ctx, struct dect_status_info *status)
{
	uint32_t wire_parent_count;
	uint32_t wire_child_count;
	uint32_t n_child_store;
	const uint8_t *p;
	size_t sz;

	dect_rpc_status_init_empty(status);

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	status->mdm_activated = nrf_rpc_decode_bool(ctx);
	status->cluster_running = nrf_rpc_decode_bool(ctx);
	status->cluster_channel = (uint16_t)nrf_rpc_decode_uint(ctx);
	status->nw_beacon_running = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	wire_parent_count = nrf_rpc_decode_uint(ctx);
	status->parent_count = (uint8_t)MIN(wire_parent_count, UINT8_MAX);

	if (wire_parent_count > 0) {
		if (!dect_rpc_status_decode_association(ctx, &status->parent_associations[0])) {
			return false;
		}
		/* Server may report parent_count >1; wire has one parent block. */
	}

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	wire_child_count = nrf_rpc_decode_uint(ctx);
	n_child_store = MIN(wire_child_count,
			    MIN((uint32_t)DECT_RPC_MAX_STATUS_CHILDREN,
				(uint32_t)DECT_RPC_STATUS_CHILD_SLOTS));

	for (uint32_t i = 0; i < n_child_store; i++) {
		if (!dect_rpc_status_decode_association(ctx, &status->child_associations[i])) {
			return false;
		}
	}
	for (uint32_t i = n_child_store; i < wire_child_count; i++) {
		if (!nrf_rpc_decode_valid(ctx)) {
			return false;
		}
		dect_rpc_decode_skip_status_association(ctx);
		if (!nrf_rpc_decode_valid(ctx)) {
			return false;
		}
	}
	status->child_count = (uint8_t)n_child_store;

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	status->br_global_ipv6_addr_prefix_set = nrf_rpc_decode_bool(ctx);
	if (status->br_global_ipv6_addr_prefix_set) {
		p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
		if (!p || sz < sizeof(struct in6_addr)) {
			return false;
		}
		memcpy(&status->br_global_ipv6_addr_prefix, p, sizeof(struct in6_addr));
		if (!nrf_rpc_decode_valid(ctx)) {
			return false;
		}
		status->br_global_ipv6_addr_prefix_len = (int)nrf_rpc_decode_uint(ctx);
	} else {
		(void)nrf_rpc_decode_skip(ctx);
		(void)nrf_rpc_decode_skip(ctx);
		status->br_global_ipv6_addr_prefix_len = 0;
	}

	if (!nrf_rpc_decode_valid(ctx)) {
		return false;
	}

	p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
	if (p && sz > 0) {
		if (sz >= sizeof(status->fw_version_str)) {
			sz = sizeof(status->fw_version_str) - 1;
		}
		memcpy(status->fw_version_str, p, sz);
		status->fw_version_str[sz] = '\0';
	}

	return nrf_rpc_decode_valid(ctx);
}
