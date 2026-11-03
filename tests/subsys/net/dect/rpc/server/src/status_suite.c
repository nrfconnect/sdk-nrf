/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Tests the server's DECT_RPC_CMD_IF_STATUS decoder (dect_rpc_cmd_if_status() in
 * server/dect_rpc_if.c) against a real, booted-but-unassociated "dect0" interface (see
 * server_dect_fixture.c): net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, ...) succeeds (the
 * driver is booted), so the server takes the real status path, not the "no dect0
 * interface" fallback -- other suites in this binary (if_send_suite.c, link_state_suite.c)
 * already boot the shared DECT stack, and this suite's own setup ensures it regardless of
 * suite run order.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <status_codec_helpers.h>
#include <test_rpc_env.h>
#include "dect_mdm_mock_test_seam.h"
#include "server_dect_fixture.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	zassert_not_null(server_dect_fixture_ensure_booted(), "DECT stack did not boot");
	return NULL;
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	dect_mdm_mock_status_info_clear();
	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

static void tc_teardown(void *f)
{
	ARG_UNUSED(f);

	dect_mdm_mock_status_info_clear();
}

/*
 * Real DECT_RPC_CMD_IF_STATUS response for a booted-but-unassociated dect0: modem
 * activated, no cluster/beacon, no parent/child associations, no border router prefix,
 * and the mock driver firmware string (DECT_MDM_MOCK_FW_VERSION). Field order/shape
 * must match
 * dect_rpc_status_rsp_encode() in common/dect_rpc_common.c.
 */
#define STATUS_RSP_BOOTED_UNASSOCIATED                                                             \
	CBOR_TRUE,              /* mdm_activated */                                                \
		CBOR_FALSE,     /* cluster_running */                                              \
		CBOR_UINT_SMALL(0), /* cluster_channel */                                          \
		CBOR_FALSE,         /* nw_beacon_running */                                        \
		CBOR_UINT_SMALL(0), /* parent_count */                                             \
		CBOR_UINT_SMALL(0), /* child_count */                                              \
		CBOR_FALSE,         /* br_global_ipv6_addr_prefix_set */                           \
		CBOR_BSTR(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                     \
		/* prefix, unused */                                                               \
		CBOR_UINT_SMALL(0), /* prefix len, unused */                                       \
		CBOR_BSTR(13, 'N', 'o', 't', ' ', 'a', 'v', 'a', 'i', 'l', 'a', 'b', 'l', 'e')

ZTEST(dect_rpc_server_status, test_status_booted_unassociated)
{
	mock_dect_rpc_tr_expect_add(RPC_RSP(STATUS_RSP_BOOTED_UNASSOCIATED), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_STATUS, CBOR_UINT_SMALL(0)));
	mock_dect_rpc_tr_expect_done();
}

ZTEST(dect_rpc_server_status, test_status_full_from_modem_mock)
{
	struct dect_status_info sample;

	dect_rpc_test_status_fill_sample(&sample);
	dect_mdm_mock_status_info_set(&sample);

	mock_dect_rpc_tr_expect_add(dect_rpc_test_status_rpc_rsp_from(&sample), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_STATUS, CBOR_UINT_SMALL(0)));
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_server_status, NULL, suite_setup, tc_setup, tc_teardown, NULL);
