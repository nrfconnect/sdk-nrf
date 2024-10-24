/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#if IS_ENABLED(CONFIG_NRF_MODEM)
#include <modem/nrf_modem_lib.h>
#endif
#include <modem/modem_jwt.h>
#include "fakes.h"
#include "string.h"

#define RUNTIME_ID "test"
#define UUID_LEN 36

/* This function runs before each test */
static void run_before(void *fixture)
{
	ARG_UNUSED(fixture);

#if !IS_ENABLED(CONFIG_NRF_MODEM_LIB)
	RESET_FAKE(nrf_modem_lib_init);
	RESET_FAKE(nrf_modem_lib_shutdown);
	RESET_FAKE(nrf_modem_at_cmd);
	RESET_FAKE(modem_jwt_get_uuids);
	RESET_FAKE(hw_id_get);
#endif
	int err = nrf_modem_lib_init();

	printk("MODEM LIB INIT: %d\n", err);

}

/* This function runs after each completed test */
static void run_after(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST_SUITE(nrf_cloud_client_id_test, NULL, NULL, run_before, run_after, NULL);

/*
 *
 */
ZTEST(nrf_cloud_client_id_test, test_nrf_cloud_client_id)
{
	int ret;
	char buf[NRF_CLOUD_CLIENT_ID_MAX_LEN + 2];
	size_t len;

#if IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
	printk("IMEI TEST\n");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
	printk("COMPTIME TEST\n");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
	printk("UUID TEST\n");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
	printk("HWID TEST\n");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
	printk("RUNTIME TEST\n");
#endif

	ret = nrf_cloud_client_id_get(buf, 0);
	zassert_true((ret < 0), "Zero len buffer returned a value >= 0.");
	zassert_equal(ret, -EINVAL, "Zero len returned wrong error.");

	len = NRF_CLOUD_CLIENT_ID_MAX_LEN;
	ret = nrf_cloud_client_id_get(NULL, len);
	zassert_true((ret < 0), "NULL pointer returned a value >= 0.");
	zassert_equal(ret, -EINVAL, "NULL pointer returned wrong error.");

#if IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
	len = NRF_CLOUD_CLIENT_ID_MAX_LEN;
	ret = nrf_cloud_client_id_get(buf, len);
	zassert_true((ret < 0), "Unset runtime ID returned a value >= 0.");
	zassert_equal(ret, -ENXIO, "Wrong error returned when runtime ID not set.");

	ret = nrf_cloud_client_id_runtime_set("");
	zassert_equal(ret, -ENODATA, "Wrong error returned when empty runtime ID set.");

	memset(buf, 'A', NRF_CLOUD_CLIENT_ID_MAX_LEN + 1);
	buf[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1] = '\0';
	ret = nrf_cloud_client_id_runtime_set(buf);
	zassert_equal(ret, -EINVAL, "Wrong error returned when too large runtime ID set.");

	ret = nrf_cloud_client_id_runtime_set(RUNTIME_ID);
	zassert_equal(ret, 0, "Unexpected error when setting runtime client id");
#endif

	len = 1;
	ret = nrf_cloud_client_id_get(buf, len);
	zassert_true((ret < 0), "Too-small buffer returned a value >= 0.");
	zassert_equal(ret, -EMSGSIZE, "Wrong error returned with too-small buffer.");

	len = NRF_CLOUD_CLIENT_ID_MAX_LEN;
	ret = nrf_cloud_client_id_get(buf, len);
	printk("nrf_cloud_client_id_get: ret = %d, id: %.*s\n", ret, len, buf);
	zassert_equal(ret, 0, "Unexpected error when getting client id");

#if IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
	ret = strncmp(buf, CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX,
		      strlen(CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX));
	zassert_equal(ret, 0, "Unexpected prefix on IMEI client id");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
	ret = strncmp(buf, CONFIG_NRF_CLOUD_CLIENT_ID,
		      strlen(CONFIG_NRF_CLOUD_CLIENT_ID));
	zassert_equal(ret, 0, "Unexpected miscompare on compile time client id");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID)
	zassert_equal(strlen(buf), UUID_LEN, "Unexpected length of UUID id");
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID)
	/* TODO: check length returned depending on CONFIG_HW_ID_LIBRARY_SOURCE */
#elif IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
	ret = strncmp(buf, RUNTIME_ID, len);
	zassert_equal(ret, 0, "Unexpected miscompare on runtime client id");
#endif
}
