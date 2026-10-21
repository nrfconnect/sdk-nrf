/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Unit tests for DECT_RPC_CMD_IF_STATUS CBOR codec (dect_rpc_status_*).
 */

#include "status_codec_helpers.h"
#include "dect_rpc_common.h"

#include <mock_dect_rpc_transport.h>
#include <test_rpc_env.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
	return NULL;
}

ZTEST(dect_rpc_status_codec, test_roundtrip_empty)
{
	struct dect_status_info in, out;

	dect_rpc_status_init_empty(&in);
	zassert_true(dect_rpc_test_status_roundtrip(&in, &out));
	dect_rpc_test_status_assert_equal(&in, &out);
}

ZTEST(dect_rpc_status_codec, test_roundtrip_full_sample)
{
	struct dect_status_info in, out;

	dect_rpc_test_status_fill_sample(&in);
	zassert_true(dect_rpc_test_status_roundtrip(&in, &out));
	dect_rpc_test_status_assert_equal(&in, &out);
}

ZTEST(dect_rpc_status_codec, test_decode_matches_no_dect_if_macro)
{
	struct dect_status_info out;
	static const uint8_t cbor[] = {
		CBOR_FALSE, CBOR_FALSE, CBOR_UINT_SMALL(0), CBOR_FALSE, CBOR_UINT_SMALL(0),
		CBOR_UINT_SMALL(0), CBOR_FALSE,
		CBOR_BSTR(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
		CBOR_UINT_SMALL(0), CBOR_BSTR(1, 0x00),
	};

	zassert_true(dect_rpc_test_status_decode_cbor(cbor, sizeof(cbor), &out));
	zassert_false(out.mdm_activated);
	zassert_false(out.cluster_running);
	zassert_equal(out.cluster_channel, 0);
	zassert_false(out.nw_beacon_running);
	zassert_equal(out.parent_count, 0);
	zassert_equal(out.child_count, 0);
	zassert_false(out.br_global_ipv6_addr_prefix_set);
	zassert_is_null(out.br_net_iface);
}

ZTEST(dect_rpc_status_codec, test_parent_count_gt_one_single_wire_assoc)
{
	struct dect_status_info enc, out;

	dect_rpc_test_status_fill_sample(&enc);
	enc.parent_count = 3;

	zassert_true(dect_rpc_test_status_roundtrip(&enc, &out));
	zassert_equal(out.parent_count, 3);
	dect_rpc_test_status_assert_equal(&enc, &out);
}

ZTEST(dect_rpc_status_codec, test_decode_skips_extra_wire_children)
{
	struct dect_status_info enc, out;
	const uint32_t wire_child_count = DECT_RPC_MAX_STATUS_CHILDREN + 4;
	int i;

	dect_rpc_status_init_empty(&enc);
	enc.child_count = DECT_RPC_MAX_STATUS_CHILDREN;
	for (i = 0; i < DECT_RPC_MAX_STATUS_CHILDREN; i++) {
		enc.child_associations[i].long_rd_id = 0x1000 + i;
	}

	zassert_true(dect_rpc_test_status_decode_wire_extra_children(&enc, wire_child_count, &out));
	dect_rpc_test_status_assert_equal(&enc, &out);
}

ZTEST(dect_rpc_status_codec, test_decode_rejects_child_count_above_int_max)
{
	struct dect_status_info out;
	static const uint8_t cbor[] = {
		CBOR_FALSE, CBOR_FALSE, CBOR_UINT_SMALL(0), CBOR_FALSE, CBOR_UINT_SMALL(0),
		CBOR_UINT32(0x80000000U), CBOR_FALSE,
		CBOR_BSTR(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
		CBOR_UINT_SMALL(0), CBOR_BSTR(1, 0x00),
	};

	zassert_false(dect_rpc_test_status_decode_cbor(cbor, sizeof(cbor), &out));
}

ZTEST_SUITE(dect_rpc_status_codec, NULL, suite_setup, NULL, NULL, NULL);
