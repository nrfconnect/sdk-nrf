/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <test_rpc_env.h>
#include <rpc_utils_group.h>
#include <nrf_rpc/rpc_utils/die_temp.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "../common/die_temp_test_data.h"

struct temp_success_test_case {
	const char *name;
	int32_t expected_temp;
	mock_nrf_rpc_pkt_t response;
};

#define SUCCESS_CASE(case_name, temp_centidegrees, cbor_bytes)                                     \
	{                                                                                          \
		.name = case_name,                                                                 \
		.expected_temp = temp_centidegrees,                                                \
		.response = RPC_RSP(NRF_RPC_DIE_TEMP_SUCCESS, UNPACK cbor_bytes),                  \
	}

/* Test cases are defined using X-macros from common header to ensure
 * client and server tests use identical values. See die_temp_test_data.h
 * for complete documentation of test coverage and CBOR encoding details.
 */
#define X(name, centidegrees, cbor_bytes) SUCCESS_CASE(name, centidegrees, cbor_bytes),
static const struct temp_success_test_case success_cases[] = {
	DIE_TEMP_TEST_CASES
};
#undef X

struct temp_error_test_case {
	const char *name;
	enum nrf_rpc_die_temp_error expected_error;
};

static const struct temp_error_test_case error_cases[] = {
	{.name = "not_ready", .expected_error = NRF_RPC_DIE_TEMP_ERR_NOT_READY},
	{.name = "fetch_failed", .expected_error = NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED},
	{.name = "channel_get_failed", .expected_error = NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED},
};

ZTEST(rpc_utils_client_die_temp, test_die_temp_null_pointer)
{
	int ret = nrf_rpc_die_temp_get(NULL);

	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL pointer");
}

ZTEST(rpc_utils_client_die_temp, test_die_temp_success)
{
	for (size_t i = 0; i < ARRAY_SIZE(success_cases); i++) {
		const struct temp_success_test_case *tc = &success_cases[i];
		int32_t temperature = -999;
		int ret;

		TC_PRINT("Testing success case: %s (expected: %d)\n", tc->name, tc->expected_temp);

		mock_nrf_rpc_tr_expect_add(RPC_CMD(RPC_UTIL_DIE_TEMP_GET), tc->response);

		ret = nrf_rpc_die_temp_get(&temperature);

		mock_nrf_rpc_tr_expect_done();

		zassert_equal(ret, NRF_RPC_DIE_TEMP_SUCCESS, "Case %s: Expected success", tc->name);
		zassert_equal(temperature, tc->expected_temp, "Case %s: Expected temperature %d",
			      tc->name, tc->expected_temp);
	}
}

ZTEST(rpc_utils_client_die_temp, test_die_temp_errors)
{
	for (size_t i = 0; i < ARRAY_SIZE(error_cases); i++) {
		const struct temp_error_test_case *tc = &error_cases[i];
		int32_t temperature = -999;
		int ret;

		TC_PRINT("Testing error case: %s (code: %d)\n", tc->name, tc->expected_error);

		mock_nrf_rpc_tr_expect_add(RPC_CMD(RPC_UTIL_DIE_TEMP_GET),
					   RPC_RSP(tc->expected_error, 0x00));

		ret = nrf_rpc_die_temp_get(&temperature);

		mock_nrf_rpc_tr_expect_done();

		zassert_equal(ret, tc->expected_error, "Case %s: Expected error code %d", tc->name,
			      tc->expected_error);
	}
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(NULL));
	mock_nrf_rpc_tr_expect_reset();
}

ZTEST_SUITE(rpc_utils_client_die_temp, NULL, NULL, tc_setup, NULL, NULL);
