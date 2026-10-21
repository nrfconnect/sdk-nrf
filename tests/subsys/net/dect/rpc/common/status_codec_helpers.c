/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "status_codec_helpers.h"
#include "dect_rpc_common.h"

#include <nrf_rpc.h>
#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <zcbor_encode.h>
#include <string.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static void set_ipv6(struct in6_addr *addr, uint8_t b15)
{
	uint8_t raw[16] = { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, b15 };

	memcpy(addr, raw, sizeof(*addr));
}

void dect_rpc_test_status_fill_sample(struct dect_status_info *status)
{
	dect_rpc_status_init_empty(status);

	status->mdm_activated = true;
	status->cluster_running = true;
	status->cluster_channel = 42;
	status->nw_beacon_running = true;

	status->parent_count = 1;
	status->parent_associations[0].long_rd_id = 0xabcd1234U;
	set_ipv6(&status->parent_associations[0].local_ipv6_addr, 1);
	status->parent_associations[0].global_ipv6_addr_set = true;
	set_ipv6(&status->parent_associations[0].global_ipv6_addr, 2);

	status->child_count = 2;
	status->child_associations[0].long_rd_id = 0x11112222U;
	set_ipv6(&status->child_associations[0].local_ipv6_addr, 3);
	status->child_associations[0].global_ipv6_addr_set = false;

	status->child_associations[1].long_rd_id = 0x33334444U;
	set_ipv6(&status->child_associations[1].local_ipv6_addr, 4);
	status->child_associations[1].global_ipv6_addr_set = true;
	set_ipv6(&status->child_associations[1].global_ipv6_addr, 5);

	status->br_global_ipv6_addr_prefix_set = true;
	set_ipv6(&status->br_global_ipv6_addr_prefix, 6);
	status->br_global_ipv6_addr_prefix_len = 8;

	/* Must never be serialized; used to catch accidental pointer on wire. */
	status->br_net_iface = (struct net_if *)0x12345678U;

	strncpy(status->fw_version_str, "fw-1.2.3", sizeof(status->fw_version_str) - 1);
}

static void assert_assoc_equal(const struct dect_association_data *exp,
			       const struct dect_association_data *got, const char *label)
{
	zassert_equal(exp->long_rd_id, got->long_rd_id, "%s long_rd_id", label);
	zassert_mem_equal(&exp->local_ipv6_addr, &got->local_ipv6_addr, sizeof(struct in6_addr),
			  "%s local_ipv6", label);
	zassert_equal(exp->global_ipv6_addr_set, got->global_ipv6_addr_set, "%s global set", label);
	if (exp->global_ipv6_addr_set) {
		zassert_mem_equal(&exp->global_ipv6_addr, &got->global_ipv6_addr,
				  sizeof(struct in6_addr), "%s global_ipv6", label);
	}
}

void dect_rpc_test_status_assert_equal(const struct dect_status_info *expected,
				       const struct dect_status_info *got)
{
	int i;

	zassert_equal(expected->mdm_activated, got->mdm_activated, "mdm_activated");
	zassert_equal(expected->cluster_running, got->cluster_running, "cluster_running");
	zassert_equal(expected->cluster_channel, got->cluster_channel, "cluster_channel");
	zassert_equal(expected->nw_beacon_running, got->nw_beacon_running, "nw_beacon_running");
	zassert_equal(expected->parent_count, got->parent_count, "parent_count");

	if (expected->parent_count > 0) {
		assert_assoc_equal(&expected->parent_associations[0], &got->parent_associations[0],
				   "parent");
	}

	zassert_equal(expected->child_count, got->child_count, "child_count");
	for (i = 0; i < expected->child_count; i++) {
		assert_assoc_equal(&expected->child_associations[i], &got->child_associations[i],
				   "child");
	}

	zassert_equal(expected->br_global_ipv6_addr_prefix_set, got->br_global_ipv6_addr_prefix_set,
		      "br prefix set");
	if (expected->br_global_ipv6_addr_prefix_set) {
		zassert_mem_equal(&expected->br_global_ipv6_addr_prefix,
				  &got->br_global_ipv6_addr_prefix, sizeof(struct in6_addr),
				  "br prefix");
		zassert_equal(expected->br_global_ipv6_addr_prefix_len,
			      got->br_global_ipv6_addr_prefix_len, "br prefix len");
	}

	zassert_is_null(got->br_net_iface, "br_net_iface must stay NULL over RPC");
	zassert_mem_equal(expected->fw_version_str, got->fw_version_str,
			  strlen(expected->fw_version_str) + 1, "fw_version_str");
}

static size_t status_cbor_encoded_len(struct nrf_rpc_cbor_ctx *ctx)
{
	return (size_t)(ctx->zs[0].payload_mut - ctx->out_packet);
}

#define STATUS_CBOR_MAX_PARAMS 255

static void status_cbor_decode_ctx_init(struct nrf_rpc_cbor_ctx *ctx, const uint8_t *data,
					size_t len)
{
	zcbor_new_decode_state(ctx->zs, ARRAY_SIZE(ctx->zs), data, len, STATUS_CBOR_MAX_PARAMS,
			       NULL, 0);
	ctx->zs->constant_state->stop_on_error = true;
}

static void test_status_encode_association(struct nrf_rpc_cbor_ctx *ctx,
					 const struct dect_association_data *assoc)
{
	nrf_rpc_encode_uint(ctx, assoc->long_rd_id);
	nrf_rpc_encode_buffer(ctx, &assoc->local_ipv6_addr, sizeof(struct in6_addr));
	nrf_rpc_encode_bool(ctx, assoc->global_ipv6_addr_set);
	nrf_rpc_encode_buffer(ctx, &assoc->global_ipv6_addr, sizeof(struct in6_addr));
}

static void test_status_rsp_encode_wire_child_count(const struct dect_status_info *status,
						    uint32_t wire_child_count,
						    struct nrf_rpc_cbor_ctx *ctx)
{
	size_t fw_len;
	int i;

	fw_len = strnlen(status->fw_version_str, sizeof(status->fw_version_str));

	nrf_rpc_encode_bool(ctx, status->mdm_activated);
	nrf_rpc_encode_bool(ctx, status->cluster_running);
	nrf_rpc_encode_uint(ctx, (uint32_t)status->cluster_channel);
	nrf_rpc_encode_bool(ctx, status->nw_beacon_running);
	nrf_rpc_encode_uint(ctx, (uint32_t)status->parent_count);

	for (i = 0; i < status->parent_count && i < 1; i++) {
		test_status_encode_association(ctx, &status->parent_associations[i]);
	}

	nrf_rpc_encode_uint(ctx, wire_child_count);
	for (i = 0; i < (int)wire_child_count; i++) {
		struct dect_association_data dummy;
		const struct dect_association_data *assoc;

		if (i < status->child_count && i < DECT_RPC_MAX_STATUS_CHILDREN) {
			assoc = &status->child_associations[i];
		} else {
			memset(&dummy, 0, sizeof(dummy));
			dummy.long_rd_id = 0xdead0000U + (uint32_t)i;
			set_ipv6(&dummy.local_ipv6_addr, (uint8_t)i);
			dummy.global_ipv6_addr_set = (i % 2) != 0;
			if (dummy.global_ipv6_addr_set) {
				set_ipv6(&dummy.global_ipv6_addr, (uint8_t)(i + 128));
			}
			assoc = &dummy;
		}
		test_status_encode_association(ctx, assoc);
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

bool dect_rpc_test_status_decode_wire_extra_children(const struct dect_status_info *status,
						     uint32_t wire_child_count,
						     struct dect_status_info *out)
{
	struct nrf_rpc_cbor_ctx enc;
	struct nrf_rpc_cbor_ctx dec;
	size_t len;
	bool ok;

	size_t enc_hint = dect_rpc_status_rsp_encode_size(status);

	if (wire_child_count > DECT_RPC_MAX_STATUS_CHILDREN) {
		enc_hint += (wire_child_count - DECT_RPC_MAX_STATUS_CHILDREN) *
			    (4U + sizeof(struct in6_addr) + 1U + sizeof(struct in6_addr));
	}

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, enc, MAX(1024, enc_hint));
	test_status_rsp_encode_wire_child_count(status, wire_child_count, &enc);
	zassert_true(zcbor_nil_put(enc.zs, NULL), "status encode nil");
	len = status_cbor_encoded_len(&enc);

	status_cbor_decode_ctx_init(&dec, enc.out_packet, len);
	ok = dect_rpc_status_rsp_decode(&dec, out);
	if (ok) {
		ok = nrf_rpc_decoding_done_and_check(&dect_rpc_group, &dec);
	}

	NRF_RPC_CBOR_DISCARD(&dect_rpc_group, enc);
	return ok;
}

bool dect_rpc_test_status_roundtrip(const struct dect_status_info *in, struct dect_status_info *out)
{
	struct nrf_rpc_cbor_ctx enc;
	struct nrf_rpc_cbor_ctx dec;
	size_t len;
	bool ok;

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, enc, MAX(1024, dect_rpc_status_rsp_encode_size(in)));
	dect_rpc_status_rsp_encode(&enc, in);
	zassert_true(zcbor_nil_put(enc.zs, NULL), "status encode nil");
	len = status_cbor_encoded_len(&enc);

	status_cbor_decode_ctx_init(&dec, enc.out_packet, len);
	ok = dect_rpc_status_rsp_decode(&dec, out);
	if (ok) {
		ok = nrf_rpc_decoding_done_and_check(&dect_rpc_group, &dec);
	}

	NRF_RPC_CBOR_DISCARD(&dect_rpc_group, enc);
	return ok;
}

bool dect_rpc_test_status_decode_cbor(const uint8_t *cbor, size_t len,
				      struct dect_status_info *out)
{
	struct nrf_rpc_cbor_ctx dec;

	status_cbor_decode_ctx_init(&dec, cbor, len);
	return dect_rpc_status_rsp_decode(&dec, out);
}

mock_dect_rpc_pkt_t dect_rpc_test_status_rpc_rsp_from(const struct dect_status_info *status)
{
	static uint8_t pkt[640];
	struct nrf_rpc_cbor_ctx enc;
	size_t pay_len;

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, enc,
			   MAX(1024, dect_rpc_status_rsp_encode_size(status)));
	dect_rpc_status_rsp_encode(&enc, status);
	pay_len = status_cbor_encoded_len(&enc);

	zassert_true((5U + pay_len + 1U) <= sizeof(pkt), "status RPC_RSP too large");

	pkt[0] = 0x01;
	pkt[1] = 0xff;
	pkt[2] = 0x00;
	pkt[3] = 0x00;
	pkt[4] = 0x00;
	memcpy(pkt + 5, enc.out_packet, pay_len);
	pkt[5 + pay_len] = 0xf6;

	NRF_RPC_CBOR_DISCARD(&dect_rpc_group, enc);

	return (mock_dect_rpc_pkt_t){
		.data = pkt,
		.len = 5 + pay_len + 1,
	};
}
