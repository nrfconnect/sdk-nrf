/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Public client hooks from net/dect/dect_rpc.h (group pointer and post-init notify).
 */

#include "dect_rpc_common.h"
#include "dect_rpc_client_net.h"
#include <mock_dect_rpc_transport.h>
#include <net/dect/dect_rpc.h>
#include <test_rpc_env.h>

#include <nrf_rpc.h>
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

ZTEST(dect_rpc_client_api, test_get_group_returns_dect_rpc_group)
{
	const struct nrf_rpc_group *group = dect_rpc_client_get_group();

	zassert_not_null(group, "group pointer");
	zassert_equal_ptr(group, &dect_rpc_group, "must be the dect_rpc nRF RPC group");
	zassert_true(NRF_RPC_GROUP_STATUS(dect_rpc_group), "group bound after mock init");
}

ZTEST(dect_rpc_client_api, test_notify_rpc_init_done_no_auto_sync)
{
	/* CONFIG_DECT_NR_RPC_AUTO_SYNC=n in client/prj.conf: must be safe to call. */
	dect_rpc_client_notify_rpc_init_done();
}

ZTEST_SUITE(dect_rpc_client_api, NULL, suite_setup, NULL, NULL, NULL);
