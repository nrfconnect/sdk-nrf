/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#define ZB_BINARY_TRACE
#define ZB_HAVE_SERIAL
#define ZB_SERIAL_FOR_TRACE

#include <zephyr/types.h>
#include <ztest.h>
#include <zboss_api.h>
#include "log_mock.h"


static uint8_t test_logger_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
				  11, 12, 13, 14, 15, 16, 17, 18};


static void test_logger(void)
{
	int32_t i;

	zb_osif_serial_put_bytes(test_logger_data,
				 ARRAY_SIZE(test_logger_data));
	zb_osif_serial_flush();

	zassert_equal(log_mock_get_buf_len(),
		      ARRAY_SIZE(test_logger_data),
		      "data length error");

	for (i = 0; i < log_mock_get_buf_len(); i++) {
		zassert_equal(log_mock_get_buf_data(i),
			      test_logger_data[i],
			      "data compare error");
	}
}

void test_main(void)
{
	ztest_test_suite(nrf_osif_logger_tests,
			 ztest_unit_test(test_logger)
	);

	ztest_run_test_suite(nrf_osif_logger_tests);
}
