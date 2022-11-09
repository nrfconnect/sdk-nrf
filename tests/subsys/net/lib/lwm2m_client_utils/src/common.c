/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils.h>
#include <modem/modem_key_mgmt.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <modem/modem_info.h>

#include "stubs.h"
#include "lwm2m_object.h"

static rsrp_cb_t modem_info_rsrp_cb;

static uint8_t con_mon_mode;

static int8_t modem_rsp_resource;

static enum lte_lc_lte_mode lte_mode;

ZTEST_SUITE(lwm2m_client_utils_common, NULL, NULL, NULL, NULL, NULL);

static int modem_info_rsrp_register_copy(rsrp_cb_t cb)
{
	modem_info_rsrp_cb = cb;
	return 0;
}

static int lwm2m_engine_set_s8_copy(const char *pathstr, int8_t value)
{
	if (strcmp(pathstr, "4/0/2") == 0) {
		modem_rsp_resource = value;
	}
	return 0;
}

static int lwm2m_engine_set_u8_copy(const char *pathstr, uint8_t value)
{
	if (strcmp(pathstr, "4/0/0") == 0) {
		con_mon_mode = value;
	}
	return 0;
}

static int lte_lc_lte_mode_get_copy(enum lte_lc_lte_mode *mode)
{
	*mode = lte_mode;
	return 0;
}

static void test_common_update(uint8_t test_con_mon_mode)
{
	int rc;

	rc = lwm2m_update_connmon();
	zassert_equal(rc, modem_info_rsrp_register_fake.return_val, "wrong return value");
	k_sleep(K_MSEC(100));
	zassert_equal(con_mon_mode, test_con_mon_mode, "wrong con mode value");
}

static void test_signal_update_feed(char rsrp_value)
{
	modem_info_rsrp_cb(rsrp_value);
	k_sleep(K_MSEC(100));
}

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
	/* Clear Custom fake functions */
	lte_lc_lte_mode_get_fake.custom_fake = NULL;
	lwm2m_engine_set_s8_fake.custom_fake = NULL;
	lwm2m_engine_set_u8_fake.custom_fake = NULL;
	modem_info_rsrp_register_fake.custom_fake = NULL;
	/* Clear Custom fake states */
	modem_info_rsrp_cb = NULL;
	con_mon_mode = 0;
	modem_rsp_resource = 0;
	lte_mode = LTE_LC_LTE_MODE_NONE;
}

/*
 * Tests for initializing the module and setting the credentials
 */

ZTEST(lwm2m_client_utils_common, test_init_conmon_fails)
{
	int rc;

	setup();
	/* First, test if settings fail */
	modem_info_init_fake.return_val = -1;
	rc = lwm2m_init_connmon();
	zassert_equal(rc, modem_info_init_fake.return_val, "wrong return value");
	modem_info_init_fake.return_val = 0;
	rc = lwm2m_init_connmon();
	zassert_equal(rc, modem_info_init_fake.return_val, "wrong return value");
}

/*
 * Tests for initializing the module and setting the credentials
 */

ZTEST(lwm2m_client_utils_common, test_update_commands)
{
	setup();
	/* First, test if settings fail */
	modem_info_rsrp_register_fake.return_val = -1;

	modem_info_params_get_fake.return_val = -1;
	lte_lc_lte_mode_get_fake.return_val = -1;
	test_common_update(0);
	modem_info_params_get_fake.return_val = 0;
	test_common_update(0);
	/* All return OK but no update */
	lte_lc_lte_mode_get_fake.return_val = 0;
	lte_lc_lte_mode_get_fake.custom_fake = lte_lc_lte_mode_get_copy;
	lwm2m_engine_set_u8_fake.custom_fake = lwm2m_engine_set_u8_copy;
	modem_info_rsrp_register_fake.return_val = 0;
	test_common_update(0);
	/* Test detetct mode LTE_LC_LTE_MODE_LTEM */
	lte_mode = LTE_LC_LTE_MODE_LTEM;
	test_common_update(6);
	/* Test detetct mode LTE_LC_LTE_MODE_NBIOT */
	lte_mode = LTE_LC_LTE_MODE_NBIOT;
	test_common_update(7);
}

/*
 * Tests for initializing the module and setting the credentials
 */

ZTEST(lwm2m_client_utils_common, test_signal_update)
{
	int rc;
	int8_t modem_test_val;

	setup();
	/* First, test if settings fail */
	modem_info_rsrp_register_fake.custom_fake = modem_info_rsrp_register_copy;
	lwm2m_engine_set_s8_fake.custom_fake = lwm2m_engine_set_s8_copy;

	modem_test_val = modem_rsp_resource;
	k_sleep(K_SECONDS(2));
	rc = lwm2m_update_connmon();
	zassert_equal(rc, 0, "wrong return value");
	zassert_not_null(modem_info_rsrp_cb, "NULL");

	/* Test too big number that it not change value */
	test_signal_update_feed(98);
	zassert_equal(modem_test_val, modem_rsp_resource, "wrong RSP value");
	/* Test that init value is updated */
	test_signal_update_feed(97);
	zassert_not_equal(modem_test_val, modem_rsp_resource, "wrong RSP value");
	modem_test_val = modem_rsp_resource;
	/* Test that init value is updated is not updated before CONFIG_LWM2M_CONN_HOLD_TIME_RSRP */
	k_sleep(K_SECONDS(2));
	test_signal_update_feed(96);
	zassert_equal(modem_test_val, modem_rsp_resource, "wrong RSP value");
	k_sleep(K_SECONDS(CONFIG_LWM2M_CONN_HOLD_TIME_RSRP));
	/* Test that init value is updated after CONFIG_LWM2M_CONN_HOLD_TIME_RSRP */
	test_signal_update_feed(96);
	zassert_not_equal(modem_test_val, modem_rsp_resource, "wrong RSP value");
}
