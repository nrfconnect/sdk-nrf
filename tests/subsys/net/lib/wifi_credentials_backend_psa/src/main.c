/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/fff.h>

#include <net/wifi_credentials.h>

#include "wifi_credentials_internal.h"
#include "psa/protected_storage.h"

#define SSID1 "test1"
#define PSK1 "super secret"
#define SECURITY1 WIFI_SECURITY_TYPE_PSK
#define BSSID1 "abcdef"
#define FLAGS1 WIFI_CREDENTIALS_FLAG_BSSID

#define SSID2 "test2"
#define PSK2 NULL
#define SECURITY2 WIFI_SECURITY_TYPE_NONE
#define BSSID2 NULL
#define FLAGS2 0

DEFINE_FFF_GLOBALS;

K_MUTEX_DEFINE(wifi_credentials_mutex);

FAKE_VOID_FUNC(wifi_credentials_cache_ssid, size_t, const struct wifi_credentials_header *);
FAKE_VALUE_FUNC(psa_status_t, psa_ps_get, psa_storage_uid_t, size_t, size_t, void *, size_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_ps_set, psa_storage_uid_t, size_t, const void *,
		psa_storage_create_flags_t);
FAKE_VALUE_FUNC(psa_status_t, psa_ps_remove, psa_storage_uid_t);

static const struct wifi_credentials_personal example1 = {
	.header = {
		.ssid = SSID1,
		.ssid_len = strlen(SSID1),
		.type = SECURITY1,
		.bssid = BSSID1,
		.flags = FLAGS1,
	},
	.password = PSK1,
	.password_len = strlen(PSK1),
};

static const struct wifi_credentials_personal example2 = {
	.header = {
		.ssid = SSID2,
		.ssid_len = strlen(SSID2),
		.type = SECURITY2,
		.flags = FLAGS2,
	},
};

psa_status_t custom_psa_ps_get(psa_storage_uid_t uid, size_t data_offset, size_t data_size,
			       void *p_data, size_t *p_data_length)
{
	/* confirm that we read the requested amount of data */
	*p_data_length = data_size;
	return PSA_SUCCESS;
}

void setUp(void)
{
	RESET_FAKE(wifi_credentials_cache_ssid);
	RESET_FAKE(psa_ps_get);
	RESET_FAKE(psa_ps_set);
	RESET_FAKE(psa_ps_remove);
	psa_ps_get_fake.custom_fake = custom_psa_ps_get;
}

void tearDown(void)
{
}

void test_init(void)
{
	int ret;

	ret = wifi_credentials_backend_init();

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES, psa_ps_get_fake.call_count);
	TEST_ASSERT_EQUAL(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES,
			  wifi_credentials_cache_ssid_fake.call_count);
}

void test_add(void)
{
	int ret;

	ret = wifi_credentials_store_entry(0, &example1, sizeof(struct wifi_credentials_personal));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_set_fake.arg0_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_ps_set_fake.arg1_val);
	TEST_ASSERT_EQUAL(&example1, psa_ps_set_fake.arg2_val);
	TEST_ASSERT_EQUAL(0, psa_ps_set_fake.arg3_val);

	ret = wifi_credentials_store_entry(1, &example2, sizeof(struct wifi_credentials_personal));
	TEST_ASSERT_EQUAL(1 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_set_fake.arg0_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_ps_set_fake.arg1_val);
	TEST_ASSERT_EQUAL(&example2, psa_ps_set_fake.arg2_val);
	TEST_ASSERT_EQUAL(0, psa_ps_set_fake.arg3_val);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(2, psa_ps_set_fake.call_count);
}

void test_get(void)
{
	int ret;

	uint8_t buf[ENTRY_MAX_LEN];

	ret = wifi_credentials_load_entry(0, buf, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_get_fake.arg0_val);
	TEST_ASSERT_EQUAL(0, psa_ps_get_fake.arg1_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_ps_get_fake.arg2_val);
	TEST_ASSERT_EQUAL(buf, psa_ps_get_fake.arg3_val);

	ret = wifi_credentials_load_entry(1, buf, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_get_fake.arg0_val);
	TEST_ASSERT_EQUAL(0, psa_ps_get_fake.arg1_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_ps_get_fake.arg2_val);
	TEST_ASSERT_EQUAL(buf, psa_ps_get_fake.arg3_val);

	TEST_ASSERT_EQUAL(2, psa_ps_get_fake.call_count);
}

void test_delete(void)
{
	int ret;

	ret = wifi_credentials_delete_entry(0);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_remove_fake.arg0_val);

	ret = wifi_credentials_delete_entry(1);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1 + CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET,
			  psa_ps_remove_fake.arg0_val);

	TEST_ASSERT_EQUAL(2, psa_ps_remove_fake.call_count);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	/* use the runner from test_runner_generate() */
	(void)unity_main();

	return 0;
}
