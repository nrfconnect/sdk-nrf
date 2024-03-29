/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>

void test_ipc_streamer_requestor_no_response(void);
void test_ipc_streamer_requestor(void);
void test_full_stack(void);

ZTEST_SUITE(test_fetch_source_mgr, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_fetch_source_mgr, test_ipc_streamer_requestor_no_response)
{
	test_ipc_streamer_requestor_no_response();
}

ZTEST(test_fetch_source_mgr, test_ipc_streamer_requestor)
{
	test_ipc_streamer_requestor();
}

ZTEST(test_fetch_source_mgr, test_full_stack)
{
	test_full_stack();
}
