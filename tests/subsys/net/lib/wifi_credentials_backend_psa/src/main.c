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
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

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

#define WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN (PSA_KEY_ID_USER_MIN + \
		CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_OFFSET)

DEFINE_FFF_GLOBALS;

K_MUTEX_DEFINE(wifi_credentials_mutex);

FAKE_VOID_FUNC(wifi_credentials_cache_ssid, size_t, const struct wifi_credentials_header *);
FAKE_VALUE_FUNC(psa_status_t, psa_export_key, mbedtls_svc_key_id_t, uint8_t *, size_t, size_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_import_key, psa_key_attributes_t *, uint8_t *, size_t,
		mbedtls_svc_key_id_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_destroy_key, mbedtls_svc_key_id_t);
FAKE_VOID_FUNC(psa_set_key_id, psa_key_attributes_t *, uint32_t);
FAKE_VOID_FUNC(psa_set_key_usage_flags, psa_key_attributes_t *, psa_key_usage_t);
FAKE_VOID_FUNC(psa_set_key_lifetime, psa_key_attributes_t *, psa_key_lifetime_t);
FAKE_VOID_FUNC(psa_set_key_algorithm, psa_key_attributes_t *, psa_algorithm_t);
FAKE_VOID_FUNC(psa_set_key_type, psa_key_attributes_t *, psa_key_type_t);
FAKE_VOID_FUNC(psa_set_key_bits, psa_key_attributes_t *, size_t);

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

static size_t idx;

psa_status_t custom_psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length)
{
	/* confirm that we read the requested amount of data */
	*data_length = data_size;
	return PSA_SUCCESS;
}

static void custom_psa_set_key_id(psa_key_attributes_t *attributes, mbedtls_svc_key_id_t key)
{
	TEST_ASSERT_EQUAL(key,  idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN);
}

void custom_psa_set_key_bits(psa_key_attributes_t *attributes, size_t bits)
{
	TEST_ASSERT_EQUAL(bits, sizeof(struct wifi_credentials_personal) * 8);
}

void custom_psa_set_key_type(psa_key_attributes_t *attributes, psa_key_type_t type)
{
	TEST_ASSERT_EQUAL(type, PSA_KEY_TYPE_RAW_DATA);
}

void custom_psa_set_key_algorithm(psa_key_attributes_t *attributes, psa_algorithm_t alg)
{
	TEST_ASSERT_EQUAL(alg, PSA_ALG_NONE);
}

void custom_psa_set_key_lifetime(psa_key_attributes_t *attributes, psa_key_lifetime_t lifetime)
{
	TEST_ASSERT_EQUAL(lifetime, PSA_KEY_LIFETIME_PERSISTENT);
}

void custom_psa_set_key_usage_flags(psa_key_attributes_t *attributes, psa_key_usage_t usage_flags)
{
	TEST_ASSERT_EQUAL(usage_flags, PSA_KEY_USAGE_EXPORT);
}

void setUp(void)
{
	RESET_FAKE(wifi_credentials_cache_ssid);
	RESET_FAKE(psa_export_key);
	RESET_FAKE(psa_import_key);
	RESET_FAKE(psa_destroy_key);
	psa_export_key_fake.custom_fake = custom_psa_export_key;
	psa_set_key_id_fake.custom_fake = custom_psa_set_key_id;
	psa_set_key_usage_flags_fake.custom_fake = custom_psa_set_key_usage_flags;
	psa_set_key_lifetime_fake.custom_fake = custom_psa_set_key_lifetime;
	psa_set_key_algorithm_fake.custom_fake = custom_psa_set_key_algorithm;
	psa_set_key_type_fake.custom_fake = custom_psa_set_key_type;
	psa_set_key_bits_fake.custom_fake = custom_psa_set_key_bits;
	idx = 0;
}

void tearDown(void)
{
}

void test_init(void)
{
	int ret;

	ret = wifi_credentials_backend_init();

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES, psa_export_key_fake.call_count);
	TEST_ASSERT_EQUAL(CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES,
			  wifi_credentials_cache_ssid_fake.call_count);
}

void test_add(void)
{
	int ret = wifi_credentials_store_entry(idx, &example1,
					       sizeof(struct wifi_credentials_personal));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_PTR(&example1, psa_import_key_fake.arg1_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_import_key_fake.arg2_val);

	idx++;

	ret = wifi_credentials_store_entry(idx, &example2,
					   sizeof(struct wifi_credentials_personal));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL_PTR(&example2, psa_import_key_fake.arg1_val);
	TEST_ASSERT_EQUAL(sizeof(struct wifi_credentials_personal), psa_import_key_fake.arg2_val);

	TEST_ASSERT_EQUAL(2, psa_import_key_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_id_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_usage_flags_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_lifetime_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_algorithm_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_type_fake.call_count);
	TEST_ASSERT_EQUAL(2, psa_set_key_bits_fake.call_count);
}

void test_get(void)
{
	int ret;
	psa_key_id_t key_id = idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN;
	uint8_t buf[ENTRY_MAX_LEN];

	ret = wifi_credentials_load_entry(idx, buf, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(key_id, psa_export_key_fake.arg0_val);
	TEST_ASSERT_EQUAL_PTR(buf, psa_export_key_fake.arg1_val);
	TEST_ASSERT_EQUAL(ARRAY_SIZE(buf), psa_export_key_fake.arg2_val);

	idx++;
	key_id = idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN;

	ret = wifi_credentials_load_entry(idx, buf, ARRAY_SIZE(buf));

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(key_id, psa_export_key_fake.arg0_val);
	TEST_ASSERT_EQUAL_PTR(buf, psa_export_key_fake.arg1_val);
	TEST_ASSERT_EQUAL(ARRAY_SIZE(buf), psa_export_key_fake.arg2_val);

	TEST_ASSERT_EQUAL(2, psa_export_key_fake.call_count);
}

void test_delete(void)
{
	int ret;

	ret = wifi_credentials_delete_entry(idx);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN,
			  psa_destroy_key_fake.arg0_val);

	idx++;

	ret = wifi_credentials_delete_entry(1);

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(idx + WIFI_CREDENTIALS_BACKEND_PSA_KEY_ID_USER_MIN,
			  psa_destroy_key_fake.arg0_val);

	TEST_ASSERT_EQUAL(2, psa_destroy_key_fake.call_count);
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
