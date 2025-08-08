/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include "server_store.h"

ZTEST(suite_server_store, test_srv_store_init)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");
}

ZTEST_SUITE(suite_server_store, NULL, NULL, NULL, NULL, NULL);
