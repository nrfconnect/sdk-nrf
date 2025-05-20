/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_resource.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/mesh_diag.h>

/* Iterator address used when testing serialization of a function that uses iterator pointers */
#define ITERATOR_ADDR UINT32_MAX

/* Fake functions */

FAKE_VALUE_FUNC(otError, otMeshDiagGetNextIp6Address, otMeshDiagIp6AddrIterator *, otIp6Address *);
FAKE_VALUE_FUNC(otError, otMeshDiagGetNextChildInfo, otMeshDiagChildIterator *,
		otMeshDiagChildInfo *);
FAKE_VALUE_FUNC(otError, otMeshDiagDiscoverTopology, otInstance *, const otMeshDiagDiscoverConfig *,
		otMeshDiagDiscoverCallback, void *);
FAKE_VOID_FUNC(otMeshDiagCancel, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otMeshDiagGetNextIp6Address);                                                            \
	f(otMeshDiagGetNextChildInfo);                                                             \
	f(otMeshDiagDiscoverTopology);                                                             \
	f(otMeshDiagCancel);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

/*
 * Test reception of otMeshDiagCancel() command.
 */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagCancel)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESH_DIAG_CANCEL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMeshDiagCancel_fake.call_count, 1);
}

static otError ot_mesh_diag_discover_topology(otInstance *instance,
					      const otMeshDiagDiscoverConfig *config,
					      otMeshDiagDiscoverCallback callback, void *context)
{
	zassert_true(config->mDiscoverChildTable);
	zassert_true(config->mDiscoverIp6Addresses);

	return OT_ERROR_NONE;
}

/*
 * Test reception of otMeshDiagDiscoverTopology() command.
 * Verify encoding of otMeshDiagDiscoverCallback callback.
 */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagDiscoverTopology)
{
	otMeshDiagDiscoverTopology_fake.custom_fake = ot_mesh_diag_discover_topology;

	otMeshDiagRouterInfo router_info = {
		.mExtAddress.m8 = {INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)},
		.mRloc16 = RLOC16,
		.mRouterId = MESHDIAG_ROUTER_ID,
		.mVersion = THREAD_VERSION,
		.mIsThisDevice = true,
		.mIsThisDeviceParent = true,
		.mIsLeader = true,
		.mIsBorderRouter = true,
		.mLinkQualities = {INT_SEQUENCE(MESHDIAG_LINK_QUALITIES_LEN)},
		.mIp6AddrIterator = (otMeshDiagIp6AddrIterator *)RESOURCE_TABLE_KEY,
		.mChildIterator = (otMeshDiagChildIterator *)RESOURCE_TABLE_KEY,
	};

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY, MESHDIAG_DISCOVER_CONFIG));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMeshDiagDiscoverTopology_fake.call_count, 1);

	/* Verify that callback is encoded correctly */
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB, OT_ERROR_NONE, EXT_ADDR,
			CBOR_UINT16(RLOC16), MESHDIAG_ROUTER_ID, THREAD_VERSION,
			MESHDIAG_ROUTER_INFO_FLAGS, MESHDIAG_LINK_QUALITIES, RESOURCE_TABLE_KEY,
			RESOURCE_TABLE_KEY), RPC_RSP());
	otMeshDiagDiscoverTopology_fake.arg2_val(OT_ERROR_NONE, &router_info, NULL);
	mock_nrf_rpc_tr_expect_done();
}

static otError ot_mesh_diag_get_next_ip6_address(otMeshDiagIp6AddrIterator *iterator,
						 otIp6Address *address)
{
	otIp6Address expected_address = {.mFields.m8 = {ADDR_1}};

	zassert_equal(iterator, (otMeshDiagIp6AddrIterator *)ITERATOR_ADDR);

	memcpy(address->mFields.m8, expected_address.mFields.m8, OT_IP6_ADDRESS_SIZE);
	return OT_ERROR_NONE;
}

/*
 * Test reception of otMeshDiagGetNextIp6Address() command.
 * Test serialization of the result: OT_ERROR_NONE, encoded ip6 address.
 */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagGetNextIp6Address)
{
	ot_rpc_res_tab_key iterator_key =
		ot_res_tab_meshdiag_ip6_it_alloc((otMeshDiagIp6AddrIterator *)ITERATOR_ADDR);

	otMeshDiagGetNextIp6Address_fake.custom_fake = ot_mesh_diag_get_next_ip6_address;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_ADDR1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS, iterator_key));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMeshDiagGetNextIp6Address_fake.call_count, 1);
}


static otError ot_mesh_diag_get_next_child_info(otMeshDiagChildIterator *iterator,
						otMeshDiagChildInfo *child_info)
{
	zassert_equal(iterator, (otMeshDiagChildIterator *)ITERATOR_ADDR);

	child_info->mRloc16 = RLOC16;
	child_info->mMode.mRxOnWhenIdle = true;
	child_info->mMode.mDeviceType = true;
	child_info->mMode.mNetworkData = true;
	child_info->mLinkQuality = LINK_QUALITY;
	child_info->mIsThisDevice = true;
	child_info->mIsBorderRouter = true;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otMeshDiagGetNextChildInfo() command.
 * Test serialization of the result: OT_ERROR_NONE, encoded child info data.
 */
ZTEST(ot_rpc_meshdiag, test_otMeshDiagGetNextChildInfo)
{
	ot_rpc_res_tab_key iterator_key =
		ot_res_tab_meshdiag_child_it_alloc((otMeshDiagChildIterator *)ITERATOR_ADDR);

	otMeshDiagGetNextChildInfo_fake.custom_fake = ot_mesh_diag_get_next_child_info;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT16(RLOC16),
					   MESHDIAG_CHILD_INFO_FLAGS, LINK_QUALITY), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO, iterator_key));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMeshDiagGetNextChildInfo_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_meshdiag, NULL, NULL, tc_setup, NULL, NULL);
