/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>

#include "stubs.h"

ZTEST_SUITE(lwm2m_client_utils_device, NULL, NULL, NULL, NULL, NULL);

static lwm2m_engine_execute_cb_t reboot_cb;
static int reboot_type;

static int lwm2m_register_exec_callback_copy(const struct lwm2m_obj_path *path,
						    lwm2m_engine_execute_cb_t cb)
{
	reboot_cb = cb;
	reboot_type = -1;
	return 0;
}

/* Stubbed */
FUNC_NORETURN void sys_reboot(int types)
{
	reboot_type = types;

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	reboot_cb = NULL;
	lwm2m_register_exec_callback_fake.custom_fake = NULL;
}

/*
 * Tests for initializing the module and test reboot funtionality
 */

ZTEST(lwm2m_client_utils_device, test_init_device)
{
	setup();
	lwm2m_register_exec_callback_fake.custom_fake =
		lwm2m_register_exec_callback_copy;

	call_lwm2m_init_callbacks();
	zassert_not_null(reboot_cb, "NULL");
}
