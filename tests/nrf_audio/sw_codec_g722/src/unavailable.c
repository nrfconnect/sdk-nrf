/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

ZTEST(g722, test_library_unavailable)
{
	ztest_test_skip();
}

ZTEST_SUITE(g722, NULL, NULL, NULL, NULL, NULL);
