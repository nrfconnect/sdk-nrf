/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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

#include <openthread/netdata.h>

/* Fake functions */

FAKE_VALUE_FUNC(otError, otNetDataGet, otInstance *, bool, uint8_t *, uint8_t *);
FAKE_VALUE_FUNC(otError, otNetDataGetNextService, otInstance *, otNetworkDataIterator *,
		otServiceConfig *);
FAKE_VALUE_FUNC(otError, otNetDataGetNextOnMeshPrefix, otInstance *, otNetworkDataIterator *,
		otBorderRouterConfig *);
FAKE_VALUE_FUNC(uint8_t, otNetDataGetStableVersion, otInstance *);
FAKE_VALUE_FUNC(uint8_t, otNetDataGetVersion, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otNetDataGet);                                                                           \
	f(otNetDataGetNextService);                                                                \
	f(otNetDataGetNextOnMeshPrefix);                                                           \
	f(otNetDataGetStableVersion);                                                              \
	f(otNetDataGetVersion);

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

static otError ot_netdata_get(otInstance *instance, bool stable, uint8_t *out, uint8_t *out_len)
{
	const uint8_t netdata[] = {NETDATA};

	if (sizeof(netdata) > *out_len) {
		return OT_ERROR_NO_BUFS;
	}

	memcpy(out, netdata, sizeof(netdata));
	*out_len = sizeof(netdata);

	return OT_ERROR_NONE;
}

/*
 * Test reception of otNetDataGet() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGet)
{
	otNetDataGet_fake.custom_fake = ot_netdata_get;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_NETDATA), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET, CBOR_TRUE, CBOR_UINT8(NETDATA_LENGTH + 1)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGet_fake.call_count, 1);
	zassert_true(otNetDataGet_fake.arg1_val);
}

/*
 * Test reception of otNetDataGet() command.
 * Test serialization of the result: OT_ERROR_NO_BUFS.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGet_nobufs)
{
	otNetDataGet_fake.custom_fake = ot_netdata_get;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NO_BUFS), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET, CBOR_FALSE, CBOR_UINT8(NETDATA_LENGTH - 1)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGet_fake.call_count, 1);
	zassert_false(otNetDataGet_fake.arg1_val);
}

static otError ot_netdata_get_next_service(otInstance *instance, otNetworkDataIterator *iter,
					   otServiceConfig *service)
{
	const uint8_t svc_data[] = {NETDATA_SVC_DATA};
	const uint8_t svr_data[] = {NETDATA_SVR_DATA};

	/* Just increment the iterator */
	++*iter;

	service->mServiceId = NETDATA_SVC_ID;
	service->mEnterpriseNumber = NETDATA_SVC_ENTERPRISE;
	service->mServiceDataLength = sizeof(svc_data);
	memcpy(service->mServiceData, svc_data, sizeof(svc_data));
	service->mServerConfig.mServerDataLength = sizeof(svr_data);
	memcpy(service->mServerConfig.mServerData, svr_data, sizeof(svr_data));
	service->mServerConfig.mStable = true;
	service->mServerConfig.mRloc16 = NETDATA_SVR_RLOC;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otNetDataGetNextService() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGetNextService)
{
	otNetDataGetNextService_fake.custom_fake = ot_netdata_get_next_service;

	mock_nrf_rpc_tr_expect_add(
		RPC_RSP(CBOR_UINT32(UINT32_MAX), CBOR_NETDATA_SVC, OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE, CBOR_UINT32(UINT32_MAX - 1)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGetNextService_fake.call_count, 1);
}

static otError ot_netdata_get_next_on_mesh_prefix(otInstance *instance, otNetworkDataIterator *iter,
						  otBorderRouterConfig *br)
{
	const uint8_t prefix[] = {ADDR_1};

	/* Just increment the iterator */
	++*iter;

	memcpy(br->mPrefix.mPrefix.mFields.m8, prefix, sizeof(prefix));
	br->mPrefix.mLength = 64;
	br->mPreference = -1;
	br->mPreferred = true;
	br->mSlaac = false;
	br->mDhcp = true;
	br->mConfigure = false;
	br->mDefaultRoute = true;
	br->mOnMesh = false;
	br->mStable = true;
	br->mNdDns = false;
	br->mDp = true;
	br->mRloc16 = NETDATA_BR_RLOC;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otNetDataGetNextOnMeshPrefix() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGetNextOnMeshPrefix)
{
	otNetDataGetNextOnMeshPrefix_fake.custom_fake = ot_netdata_get_next_on_mesh_prefix;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(UINT32_MAX), CBOR_NETDATA_BR, OT_ERROR_NONE),
				   NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX, CBOR_UINT32(UINT32_MAX - 1)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGetNextOnMeshPrefix_fake.call_count, 1);
}

/*
 * Test reception of otNetDataGetStableVersion() command.
 * Test serialization of the result: UINT8_MAX.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGetStableVersion)
{
	otNetDataGetStableVersion_fake.return_val = UINT8_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(UINT8_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_NET_DATA_GET_STABLE_VERSION));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGetStableVersion_fake.call_count, 1);
}

/*
 * Test reception of otNetDataGetVersion() command.
 * Test serialization of the result: UINT8_MAX.
 */
ZTEST(ot_rpc_netdata, test_otNetDataGetVersion)
{
	otNetDataGetVersion_fake.return_val = UINT8_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(UINT8_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_NET_DATA_GET_VERSION));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otNetDataGetVersion_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_netdata, NULL, NULL, tc_setup, NULL, NULL);
