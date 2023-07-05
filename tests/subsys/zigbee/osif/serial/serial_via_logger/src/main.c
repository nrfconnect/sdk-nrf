/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#define ZB_HAVE_SERIAL
#define ZB_DONT_NEED_TRACE_FILE_ID
#define ZB_TRACE_LEVEL 4
#define ZB_TRACE_MASK  0x00000800

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zboss_api.h>
#include "log_mock.h"


static uint8_t test_logger_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
				  11, 12, 13, 14, 15, 16, 17, 18};


ZTEST_SUITE(nrf_osif_logger_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_osif_logger_tests, test_logger)
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
