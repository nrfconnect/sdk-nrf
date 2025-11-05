/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <test_rpc_env.h>
#include <rpc_utils_group.h>
#include <nrf_rpc/rpc_utils/die_temp.h>
#include "mock_temp_sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>

#include "../common/die_temp_test_data.h"

/* Note: device_is_ready() failure is not testable in unit tests as it
 * depends on Zephyr's device initialization infrastructure.
 */

/* Test data structure mapping sensor values to their CBOR-encoded byte sequences.
 * CBOR encoding according to RFC 8949.
 */
struct temp_cbor_test_case {
	const char *name;
	struct sensor_value sensor_val;
	mock_nrf_rpc_pkt_t expected_response;
};

/* Helper macros to convert centidegrees to sensor_value components.
 * sensor_value format: val1=integer degrees, val2=fractional part in microseconds
 * Formula: centidegrees → val1 = centidegrees / 100, val2 = (centidegrees % 100) * 10000
 */
#define CENTIDEG_TO_VAL1(centideg) ((centideg) / 100)
#define CENTIDEG_TO_VAL2(centideg) (((centideg) % 100) * 10000)

#define TEMP_TEST_CASE(test_name, temp_centidegrees, cbor_bytes)                                   \
	{.name = test_name,                                                                        \
	 .sensor_val = {.val1 = CENTIDEG_TO_VAL1(temp_centidegrees),                               \
			.val2 = CENTIDEG_TO_VAL2(temp_centidegrees)},                              \
	 .expected_response = RPC_RSP(NRF_RPC_DIE_TEMP_SUCCESS, UNPACK cbor_bytes)}

/* Test cases are defined using X-macros from common header to ensure
 * client and server tests use identical values. See die_temp_test_data.h
 * for complete documentation of test coverage and CBOR encoding details.
 */
#define X(name, centidegrees, cbor_bytes) TEMP_TEST_CASE(name, centidegrees, cbor_bytes),
static const struct temp_cbor_test_case temp_success_cases[] = {
	DIE_TEMP_TEST_CASES
};
#undef X

ZTEST(rpc_utils_server_die_temp, test_die_temp_fetch_failed)
{
	mock_temp_sensor_set_fetch_result(-EIO);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED, 0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(RPC_UTIL_DIE_TEMP_GET));
	mock_nrf_rpc_tr_expect_done();

	mock_temp_sensor_set_fetch_result(0);
}

ZTEST(rpc_utils_server_die_temp, test_die_temp_channel_get_failed)
{
	mock_temp_sensor_set_fetch_result(0);
	mock_temp_sensor_set_channel_get_result(-ENOTSUP);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED, 0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(RPC_UTIL_DIE_TEMP_GET));
	mock_nrf_rpc_tr_expect_done();

	mock_temp_sensor_set_channel_get_result(0);
}

ZTEST(rpc_utils_server_die_temp, test_die_temp_success_cases)
{
	for (size_t i = 0; i < ARRAY_SIZE(temp_success_cases); i++) {
		const struct temp_cbor_test_case *test_case = &temp_success_cases[i];

		TC_PRINT("Testing case: %s\n", test_case->name);

		mock_temp_sensor_set_fetch_result(0);
		mock_temp_sensor_set_channel_get_result(0);
		mock_temp_sensor_set_value((struct sensor_value *)&test_case->sensor_val);

		mock_nrf_rpc_tr_expect_add(test_case->expected_response, NO_RSP);
		mock_nrf_rpc_tr_receive(RPC_CMD(RPC_UTIL_DIE_TEMP_GET));
		mock_nrf_rpc_tr_expect_done();
	}
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(NULL));
	mock_nrf_rpc_tr_expect_reset();

	mock_temp_sensor_set_fetch_result(0);
	mock_temp_sensor_set_channel_get_result(0);
}

ZTEST_SUITE(rpc_utils_server_die_temp, NULL, NULL, tc_setup, NULL, NULL);
