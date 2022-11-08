/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils_fota.h>

#include "stubs.h"

#define MY_KEY "counter"

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
	settings_register_fake.custom_fake = NULL;
}

static struct settings_handler *handler;
static struct update_counter uc;

static int copy_settings_handler(struct settings_handler *cf)
{
	handler = cf;
	return 0;
}

static ssize_t read_cb(void *cb_arg, void *data, size_t len)
{
	memcpy(data, cb_arg, len);
	return len;
}

ZTEST_SUITE(lwm2m_client_utils_fota_settings, NULL, NULL, NULL, NULL, NULL);

ZTEST(lwm2m_client_utils_fota_settings, test_init)
{
	int rc;
	struct update_counter uc_tmp;

	setup();

	/* test subsys_init() fail */
	settings_subsys_init_fake.return_val = -1;
	rc = fota_settings_init();
	zassert_equal(rc, -1, "wrong return value");

	/* test settings_register() fail */
	settings_subsys_init_fake.return_val = 0;
	settings_register_fake.return_val = -EEXIST;
	rc = fota_settings_init();
	zassert_equal(rc, -EEXIST, "wrong return value");

	/* test settings_load_subtree() fail */
	settings_register_fake.return_val = 0;
	settings_load_subtree_fake.return_val = -3;
	rc = fota_settings_init();
	zassert_equal(rc, -3, "wrong return value");

	/* test fota_update_counter_read() and settings handler */
	settings_register_fake.custom_fake = copy_settings_handler;
	settings_name_next_fake.return_val = sizeof(MY_KEY);
	rc = fota_settings_init();
	uc.current = 1;
	uc.update = 2;
	handler->h_set(MY_KEY, sizeof(MY_KEY), read_cb, &uc);
	fota_update_counter_read(&uc_tmp);
	zassert_mem_equal(&uc, &uc_tmp, sizeof(uc), "Incorrect update_counter");

	/* test settings_register() fail */
	settings_register_fake.custom_fake = NULL;
	settings_register_fake.return_val = 0;
	settings_load_subtree_fake.return_val = -1;
	rc = fota_settings_init();
	zassert_equal(rc, -1, "wrong return value");

	/* test init OK */
	settings_load_subtree_fake.return_val = 0;
	rc = fota_settings_init();
	zassert_equal(rc, 0, "wrong return value");

	/* test that init fails if called twice */
	rc = fota_settings_init();
	zassert_equal(rc, -EALREADY, "wrong return value");
}

ZTEST(lwm2m_client_utils_fota_settings, test_update_counter)
{
	int rc;

	setup();

	rc = fota_update_counter_update(COUNTER_CURRENT, 1);
	zassert_equal(rc, 0, "wrong return value");

	rc = fota_update_counter_update(COUNTER_UPDATE, 2);
	zassert_equal(rc, 0, "wrong return value");
}

ZTEST(lwm2m_client_utils_fota_settings, test_update_counter_fail)
{
	int rc;

	setup();

	settings_save_one_fake.return_val = -ENOENT;
	rc = fota_update_counter_update(COUNTER_CURRENT, 1);
	zassert_equal(rc, -ENOENT, "wrong return value");
}

