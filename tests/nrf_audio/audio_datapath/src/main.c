/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

/* Included to reach the static err_us_calculate() */
#include "audio_datapath.c"

ZTEST(suite_audio_datapath, test_err_us_no_wrap)
{
	int32_t err_us = err_us_calculate(1010, 1000);

	zassert_equal(err_us, 10, "err_us: %d", err_us);

	err_us = err_us_calculate(1000, 1010);
	zassert_equal(err_us, -10, "err_us: %d", err_us);
}

ZTEST(suite_audio_datapath, test_err_us_sdu_ref_wrapped)
{
	/* sdu_ref_us is 10 us after frame_start_ts_us, across the 32-bit wrap */
	int32_t err_us = err_us_calculate(5, UINT32_MAX - 4);

	zassert_equal(err_us, 10, "err_us: %d", err_us);
}

ZTEST(suite_audio_datapath, test_err_us_frame_start_wrapped)
{
	/* sdu_ref_us is 10 us before frame_start_ts_us, across the 32-bit wrap */
	int32_t err_us = err_us_calculate(UINT32_MAX - 4, 5);

	zassert_equal(err_us, -10, "err_us: %d", err_us);
}

ZTEST_SUITE(suite_audio_datapath, NULL, NULL, NULL, NULL, NULL);
