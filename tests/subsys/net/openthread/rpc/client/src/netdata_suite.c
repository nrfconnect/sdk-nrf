/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/netdata.h>

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

/* Test serialization of otNetDataGet() */
ZTEST(ot_rpc_netdata, test_otNetDataGet)
{
	const uint8_t exp_netdata[] = {NETDATA};
	otError error;
	/* Make the buffer slightly bigger than the actual data */
	uint8_t netdata[NETDATA_LENGTH + 1];
	uint8_t netdata_len = sizeof(netdata);

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET, CBOR_TRUE, CBOR_UINT8(NETDATA_LENGTH + 1)),
		RPC_RSP(OT_ERROR_NONE, CBOR_NETDATA));
	error = otNetDataGet(NULL, true, netdata, &netdata_len);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(netdata_len, NETDATA_LENGTH);
	zassert_mem_equal(netdata, exp_netdata, NETDATA_LENGTH);
}

/* Test serialization of otNetDataGet() returning OT_ERROR_NO_BUFS */
ZTEST(ot_rpc_netdata, test_otNetDataGet_nobufs)
{
	otError error;
	/* Make the buffer slightly smaller than the actual data */
	uint8_t netdata[NETDATA_LENGTH - 1];
	uint8_t netdata_len = sizeof(netdata);

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET, CBOR_FALSE, CBOR_UINT8(NETDATA_LENGTH - 1)),
		RPC_RSP(OT_ERROR_NO_BUFS));
	error = otNetDataGet(NULL, false, netdata, &netdata_len);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NO_BUFS);
}

/* Test serialization of otNetDataGetNextService() returning OT_ERROR_NONE */
ZTEST(ot_rpc_netdata, test_otNetDataGetNextService)
{
	const uint8_t exp_svc_data[] = {NETDATA_SVC_DATA};
	const uint8_t exp_svr_data[] = {NETDATA_SVR_DATA};
	otError error;
	otNetworkDataIterator iter = UINT32_MAX - 1;
	otServiceConfig service;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE, CBOR_UINT32(UINT32_MAX - 1)),
		RPC_RSP(CBOR_UINT32(UINT32_MAX), CBOR_NETDATA_SVC, OT_ERROR_NONE));
	error = otNetDataGetNextService(NULL, &iter, &service);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(iter, UINT32_MAX);
	zassert_equal(service.mServiceId, NETDATA_SVC_ID);
	zassert_equal(service.mEnterpriseNumber, NETDATA_SVC_ENTERPRISE);
	zassert_equal(service.mServiceDataLength, sizeof(exp_svc_data));
	zassert_mem_equal(service.mServiceData, exp_svc_data, service.mServiceDataLength);
	zassert_true(service.mServerConfig.mStable);
	zassert_equal(service.mServerConfig.mServerDataLength, sizeof(exp_svr_data));
	zassert_mem_equal(service.mServerConfig.mServerData, exp_svr_data,
			  service.mServerConfig.mServerDataLength);
	zassert_equal(service.mServerConfig.mRloc16, NETDATA_SVR_RLOC);
}

/* Test serialization of otNetDataGetNextOnMeshPrefix() returning OT_ERROR_NONE */
ZTEST(ot_rpc_netdata, test_otNetDataGetNextOnMeshPrefix)
{
	const uint8_t exp_prefix[] = {ADDR_1};
	otError error;
	otNetworkDataIterator iter = UINT32_MAX - 1;
	otBorderRouterConfig br;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX, CBOR_UINT32(UINT32_MAX - 1)),
		RPC_RSP(CBOR_UINT32(UINT32_MAX), CBOR_NETDATA_BR, OT_ERROR_NONE));
	error = otNetDataGetNextOnMeshPrefix(NULL, &iter, &br);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(iter, UINT32_MAX);
	zassert_equal(br.mPrefix.mLength, 64);
	zassert_mem_equal(br.mPrefix.mPrefix.mFields.m8, exp_prefix, 64 / 8);
	zassert_equal(br.mPreference, -1);
	zassert_true(br.mPreferred);
	zassert_false(br.mSlaac);
	zassert_true(br.mDhcp);
	zassert_false(br.mConfigure);
	zassert_true(br.mDefaultRoute);
	zassert_false(br.mOnMesh);
	zassert_true(br.mStable);
	zassert_false(br.mNdDns);
	zassert_true(br.mDp);
}

/* Test serialization of otNetDataGetStableVersion() */
ZTEST(ot_rpc_netdata, test_otNetDataGetStableVersion)
{
	uint8_t version;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_NET_DATA_GET_STABLE_VERSION),
				   RPC_RSP(CBOR_UINT8(UINT8_MAX)));
	version = otNetDataGetStableVersion(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(version, UINT8_MAX);
}

/* Test serialization of otNetDataGetVersion() */
ZTEST(ot_rpc_netdata, test_otNetDataGetVersion)
{
	uint8_t version;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_NET_DATA_GET_VERSION),
				   RPC_RSP(CBOR_UINT8(UINT8_MAX)));
	version = otNetDataGetVersion(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(version, UINT8_MAX);
}

ZTEST_SUITE(ot_rpc_netdata, NULL, NULL, tc_setup, NULL, NULL);
