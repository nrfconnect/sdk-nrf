/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Helpers for IF_STATUS codec tests (dect_rpc_status_* in dect_rpc_common.c).
 */

#ifndef STATUS_CODEC_HELPERS_H_
#define STATUS_CODEC_HELPERS_H_

#include <mock_dect_rpc_transport.h>

#include <net/dect/dect_net_l2.h>
#include <stdbool.h>
#include <stddef.h>

/** Populate all RPC-relevant fields (br_net_iface is non-NULL to verify it is not on wire). */
void dect_rpc_test_status_fill_sample(struct dect_status_info *status);

/** Compare decoded status to expected (br_net_iface must be NULL on got). */
void dect_rpc_test_status_assert_equal(const struct dect_status_info *expected,
				       const struct dect_status_info *got);

/** Encode then decode; returns false if decode fails. */
bool dect_rpc_test_status_roundtrip(const struct dect_status_info *in,
				    struct dect_status_info *out);

/** Decode a bare CBOR payload (no nRF RPC header). */
bool dect_rpc_test_status_decode_cbor(const uint8_t *cbor, size_t len,
				      struct dect_status_info *out);

/** Build an RPC_RSP packet whose CBOR body matches dect_rpc_status_rsp_encode(). */
mock_dect_rpc_pkt_t dect_rpc_test_status_rpc_rsp_from(const struct dect_status_info *status);

/**
 * Decode a hand-built status response whose wire child_count may exceed
 * DECT_RPC_MAX_STATUS_CHILDREN (production encode caps the count on the wire).
 * Fills overflow association blocks with dummy data so skip logic is exercised.
 */
bool dect_rpc_test_status_decode_wire_extra_children(const struct dect_status_info *status,
						     uint32_t wire_child_count,
						     struct dect_status_info *out);

#endif /* STATUS_CODEC_HELPERS_H_ */
