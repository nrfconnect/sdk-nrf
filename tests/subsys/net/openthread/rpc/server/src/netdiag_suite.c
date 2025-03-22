/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_message.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/netdiag.h>

/* Fake functions */

FAKE_VALUE_FUNC(otError, otThreadSetVendorName, otInstance *, const char *);
FAKE_VALUE_FUNC(otError, otThreadSetVendorModel, otInstance *, const char *);
FAKE_VALUE_FUNC(otError, otThreadSetVendorSwVersion, otInstance *, const char *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorName, otInstance *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorModel, otInstance *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorSwVersion, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otThreadSetVendorName);                                                                  \
	f(otThreadSetVendorModel);                                                                 \
	f(otThreadSetVendorSwVersion);                                                             \
	f(otThreadGetVendorName);                                                                  \
	f(otThreadGetVendorModel);                                                                 \
	f(otThreadGetVendorSwVersion);

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

#define TEST_STRING_SETTER(test_name, cmd, data, data_cbor, func_to_call, max_length, result_exp)  \
	static otError fake_##test_name(otInstance *instance, const char *arg)                     \
	{                                                                                          \
		char data_exp[] = data;                                                            \
                                                                                                   \
		ARG_UNUSED(instance);                                                              \
                                                                                                   \
		if (strlen(arg) > max_length) {                                                    \
			return OT_ERROR_INVALID_ARGS;                                              \
		}                                                                                  \
                                                                                                   \
		zassert_str_equal(func_to_call##_fake.arg1_val, data_exp);                         \
                                                                                                   \
		return OT_ERROR_NONE;                                                              \
	}                                                                                          \
                                                                                                   \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		func_to_call##_fake.custom_fake = fake_##test_name;                                \
                                                                                                   \
		mock_nrf_rpc_tr_expect_add(RPC_RSP(result_exp), NO_RSP);                           \
		mock_nrf_rpc_tr_receive(RPC_CMD(cmd, data_cbor));                                  \
		mock_nrf_rpc_tr_expect_done();                                                     \
                                                                                                   \
		zassert_equal(func_to_call##_fake.call_count, 1);                                  \
	}

#define TEST_STRING_GETTER(test_name, cmd, data, data_cbor, func_to_call)                          \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		char data_exp[] = data;                                                            \
		func_to_call##_fake.return_val = data_exp;                                         \
		mock_nrf_rpc_tr_expect_add(RPC_RSP(data_cbor), NO_RSP);                            \
		mock_nrf_rpc_tr_receive(RPC_CMD(cmd));                                             \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_equal(func_to_call##_fake.call_count, 1);                                  \
	}


/*
 * Test reception of otThreadGetVendorName() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_32.
 */
TEST_STRING_GETTER(test_otThreadGetVendorName, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorName);

/*
 * Test reception of otThreadGetVendorName() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorName_empty, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorName);

/*
 * Test reception of otThreadGetVendorModel() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_32.
 */
TEST_STRING_GETTER(test_otThreadGetVendorModel, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorModel);

/*
 * Test reception of otThreadGetVendorModel() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorModel_empty, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorModel);

/*
 * Test reception of otThreadGetVendorSwVersion() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_16.
 */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadGetVendorSwVersion);

/*
 * Test reception of otThreadGetVendorSwVersion() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion_empty, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorSwVersion);

/*
 * Test reception of otThreadSetVendorName() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorName() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName_empty, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorName() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName_too_long, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_33, CBOR_NETDIAG_VENDOR_DATA_33, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_INVALID_ARGS);

/*
 * Test reception of otThreadSetVendorModel() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorModel() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel_empty, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorModel() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel_too_long, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_33, CBOR_NETDIAG_VENDOR_DATA_33, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_INVALID_ARGS);

/*
 * Test reception of otThreadSetVendorSwVersion() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorSwVersion() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_empty, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorSwVersion() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_too_long,
		   OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_17, CBOR_NETDIAG_VENDOR_DATA_17, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_INVALID_ARGS);


ZTEST_SUITE(ot_rpc_netdiag, NULL, NULL, tc_setup, NULL, NULL);
