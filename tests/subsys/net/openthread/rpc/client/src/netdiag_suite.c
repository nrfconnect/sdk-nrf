/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/netdiag.h>

#define TEST_STRING_GETTER(test_name, cmd, data, data_cbor, func_to_call)                          \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		const char *result;                                                                \
		char data_exp[] = data;                                                            \
												   \
		mock_nrf_rpc_tr_expect_add(                                                        \
			RPC_CMD(cmd),                                                              \
			RPC_RSP(data_cbor));                                                       \
		result = func_to_call(NULL);                                                       \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_str_equal(result, data_exp);                                               \
	}

#define TEST_STRING_SETTER(test_name, cmd, data, data_cbor, func_to_call, result_exp)              \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		otError error;                                                                     \
		char data_exp[] = data;                                                            \
												   \
		mock_nrf_rpc_tr_expect_add(                                                        \
			RPC_CMD(cmd, data_cbor),                                                   \
			RPC_RSP(result_exp));                                                      \
		error = func_to_call(NULL, data_exp);                                              \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_equal(error, result_exp);                                                  \
	}

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

/* Test serialization of otThreadGetVendorName() */
TEST_STRING_GETTER(test_otThreadGetVendorName, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorName);

/* Test serialization of otThreadGetVendorName() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorName_empty, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorName);

/* Test serialization of otThreadGetVendorModel() */
TEST_STRING_GETTER(test_otThreadGetVendorModel, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorModel);

/* Test serialization of otThreadGetVendorModel() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorModel_empty, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorModel);

/* Test serialization of otThreadGetVendorSwVersion() */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadGetVendorSwVersion);

/* Test serialization of otThreadGetVendorName() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion_empty, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorSwVersion);

/* Test serialization of otThreadSetVendorName() */
TEST_STRING_SETTER(test_otThreadSetVendorName, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32,
		   otThreadSetVendorName, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorName() with empty string */
TEST_STRING_SETTER(test_otThreadSetVendorName_empty, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorName, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorModel() */
TEST_STRING_SETTER(test_otThreadSetVendorModel, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32,
		   otThreadSetVendorModel, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorModel() */
TEST_STRING_SETTER(test_otThreadSetVendorModel_empty, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorModel, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorSwVersion() */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16,
		   otThreadSetVendorSwVersion, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorSwVersion() with empty string */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_empty, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorSwVersion, OT_ERROR_NONE);

ZTEST_SUITE(ot_rpc_netdiag, NULL, NULL, tc_setup, NULL, NULL);
