/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ztest.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>

#include "common_utils.h"
#include "simple.h"
#include "data.h"
#include "test_events.h"


void test_initialization(void)
{
	int ret;
	const struct device *ipc_instance  = REMOTE_IPC_DEV;

	ret = app_event_manager_init();
	zassert_ok(ret, "Event manager init failed (%d)", ret);

	ret = event_manager_proxy_add_remote(ipc_instance);
	zassert_ok(ret, "Cannot add remote (%d)", ret);

	REMOTE_EVENT_SUBSCRIBE_TEST(ipc_instance, test_end_remote_event);
	REMOTE_EVENT_SUBSCRIBE_TEST(ipc_instance, test_start_ack_event);

	simple_register();
	data_register();

	ret = event_manager_proxy_start();
	zassert_ok(ret, "Cannot start event manager proxy (%d)", ret);

	ret = event_manager_proxy_wait_for_remotes(K_SECONDS(1));
	zassert_ok(ret, "Error when waiting for remote (%d)", ret);
}


void test_main(void)
{
	ztest_test_suite(test_init,
			 ztest_unit_test(test_initialization)
			 );

	ztest_run_test_suite(test_init);

	simple_run();
	data_run();
}
