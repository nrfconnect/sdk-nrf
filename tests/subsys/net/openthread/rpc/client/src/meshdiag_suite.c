/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/mesh_diag.h>

/* Fake functions */

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();
}

/* Test serialization of otMeshDiagDiscoverTopology() and otMeshDiagCancel() */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagDiscoverTopology)
{
	otMeshDiagDiscoverConfig config = {
		.mDiscoverIp6Addresses = true,
		.mDiscoverChildTable = true,
	};
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY, MESHDIAG_DISCOVER_CONFIG),
		RPC_RSP(OT_ERROR_NONE));
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_CANCEL),
		RPC_RSP());
	error = otMeshDiagDiscoverTopology(NULL, &config, NULL, NULL);
	otMeshDiagCancel(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

FAKE_VOID_FUNC(handle_mesh_diag_discover, otError, otMeshDiagRouterInfo *, void *);

static void fake_handle_mesh_diag_discover(otError error, otMeshDiagRouterInfo *router_info,
					   void *context)
{
	otExtAddress expected_ext_addr = {{INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)}};
	uint8_t expected_link_qualities[] = {INT_SEQUENCE(MESHDIAG_LINK_QUALITIES_LEN)};

	zassert_equal(error, OT_ERROR_NONE);

	zassert_mem_equal(router_info->mExtAddress.m8, expected_ext_addr.m8, OT_EXT_ADDRESS_SIZE);
	zassert_equal(router_info->mRloc16, RLOC16);
	zassert_equal(router_info->mRouterId, MESHDIAG_ROUTER_ID);
	zassert_equal(router_info->mVersion, THREAD_VERSION);
	zassert_true(router_info->mIsThisDevice);
	zassert_true(router_info->mIsThisDeviceParent);
	zassert_true(router_info->mIsLeader);
	zassert_true(router_info->mIsBorderRouter);
	zassert_mem_equal(router_info->mLinkQualities, expected_link_qualities,
			  OT_NETWORK_MAX_ROUTER_ID + 1);
	zassert_equal((ot_rpc_res_tab_key)router_info->mIp6AddrIterator, RESOURCE_TABLE_KEY);
	zassert_equal((ot_rpc_res_tab_key)router_info->mChildIterator, RESOURCE_TABLE_KEY);
}

/* Test serialization of otMeshDiagDiscoverTopology() and otMeshDiagDiscoverCallback */
ZTEST(ot_rpc_meshdiag, test_mesh_diag_discover)
{
	otError error;
	otMeshDiagDiscoverConfig config = {
		.mDiscoverIp6Addresses = true,
		.mDiscoverChildTable = true,
	};
	RESET_FAKE(handle_mesh_diag_discover);

	handle_mesh_diag_discover_fake.custom_fake = fake_handle_mesh_diag_discover;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY, MESHDIAG_DISCOVER_CONFIG),
		RPC_RSP(OT_ERROR_NONE));
	error = otMeshDiagDiscoverTopology(NULL, &config, handle_mesh_diag_discover, NULL);
	zassert_equal(error, OT_ERROR_NONE);
	mock_nrf_rpc_tr_expect_done();

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB,
					OT_ERROR_NONE, EXT_ADDR, CBOR_UINT16(RLOC16),
					MESHDIAG_ROUTER_ID, THREAD_VERSION,
					MESHDIAG_ROUTER_INFO_FLAGS, MESHDIAG_LINK_QUALITIES,
					RESOURCE_TABLE_KEY, RESOURCE_TABLE_KEY));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(handle_mesh_diag_discover_fake.call_count, 1);
}

/* Test serialization of otMeshDiagGetNextIp6Address() */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagGetNextIp6Address)
{
	otError error;
	ot_rpc_res_tab_key ip6_iterator_key = RESOURCE_TABLE_KEY;
	const otIp6Address expected_address = {.mFields.m8 = {ADDR_1}};
	otIp6Address address = {.mFields.m8 = {0}};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS, RESOURCE_TABLE_KEY),
		RPC_RSP(OT_ERROR_NONE, CBOR_ADDR1));
	error = otMeshDiagGetNextIp6Address((otMeshDiagIp6AddrIterator *)ip6_iterator_key,
					   &address);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);

	zassert_mem_equal(address.mFields.m8, expected_address.mFields.m8, OT_IP6_ADDRESS_SIZE);
}

/* Test serialization of otMeshDiagGetNextChildInfo() */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagGetNextChildInfo)
{
	otError error;
	ot_rpc_res_tab_key child_iterator_key = RESOURCE_TABLE_KEY;
	otMeshDiagChildInfo child_info;

	memset(&child_info, 0, sizeof(child_info));

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO, RESOURCE_TABLE_KEY),
		RPC_RSP(OT_ERROR_NONE, CBOR_UINT16(RLOC16), MESHDIAG_CHILD_INFO_FLAGS,
			LINK_QUALITY));

	error = otMeshDiagGetNextChildInfo((otMeshDiagChildIterator *)child_iterator_key,
					   &child_info);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);

	zassert_equal(child_info.mRloc16, RLOC16);
	zassert_true(child_info.mMode.mRxOnWhenIdle);
	zassert_true(child_info.mMode.mDeviceType);
	zassert_true(child_info.mMode.mNetworkData);
	zassert_equal(child_info.mLinkQuality, LINK_QUALITY);
	zassert_true(child_info.mIsThisDevice);
	zassert_true(child_info.mIsBorderRouter);
}

ZTEST_SUITE(ot_rpc_meshdiag, NULL, NULL, tc_setup, NULL, NULL);
