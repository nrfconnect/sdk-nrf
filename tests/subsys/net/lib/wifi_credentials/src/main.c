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

#include <zephyr/fff.h>

#include <net/wifi_credentials.h>

#include "wifi_credentials_internal.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, wifi_credentials_store_entry, size_t, const void *, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_load_entry, size_t, void *, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_delete_entry, size_t)
FAKE_VALUE_FUNC(int, wifi_credentials_backend_init)

uint8_t fake_settings_buf[CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES][ENTRY_MAX_LEN];

int custom_wifi_credentials_store_entry(size_t idx, const void *buf, size_t buf_len)
{
	TEST_ASSERT(idx < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES);
	memcpy(fake_settings_buf[idx], buf, MIN(ENTRY_MAX_LEN, buf_len));
	return 0;
}

int custom_wifi_credentials_load_entry(size_t idx, void *buf, size_t buf_len)
{
	TEST_ASSERT(idx < CONFIG_WIFI_CREDENTIALS_MAX_ENTRIES);
	memcpy(buf, fake_settings_buf[idx], MIN(ENTRY_MAX_LEN, buf_len));
	return 0;
}

void setUp(void)
{
	RESET_FAKE(wifi_credentials_store_entry);
	RESET_FAKE(wifi_credentials_load_entry);
	RESET_FAKE(wifi_credentials_delete_entry);
	wifi_credentials_store_entry_fake.custom_fake =
		custom_wifi_credentials_store_entry;
	wifi_credentials_load_entry_fake.custom_fake =
		custom_wifi_credentials_load_entry;
}

#define SSID1 "test1"
#define PSK1 "super secret"
#define SECURITY1 WIFI_SECURITY_TYPE_PSK
#define BSSID1 "abcdef"
#define FLAGS1 WIFI_CREDENTIALS_FLAG_BSSID

#define SSID2 "test2"
#define SECURITY2 WIFI_SECURITY_TYPE_NONE
#define FLAGS2 0

#define SSID3 "test3"
#define PSK3 "extremely secret"
#define SECURITY3 WIFI_SECURITY_TYPE_SAE
#define FLAGS3 0

#define SSID4 "\0what's\0null\0termination\0anyway"
#define PSK4 PSK1
#define SECURITY4 SECURITY1
#define BSSID4 BSSID1
#define FLAGS4 FLAGS1

void tearDown(void)
{
	wifi_credentials_delete_by_ssid(SSID1, ARRAY_SIZE(SSID1));
	wifi_credentials_delete_by_ssid(SSID2, ARRAY_SIZE(SSID2));
	wifi_credentials_delete_by_ssid(SSID3, ARRAY_SIZE(SSID3));
	wifi_credentials_delete_by_ssid(SSID4, ARRAY_SIZE(SSID4));
	wifi_credentials_delete_by_ssid("", 0);
}

/* Verify that attempting to retrieve a non-existent credentials entry raises -ENOENT. */
void test_get_non_existing(void)
{
	int err;
	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(-ENOENT, err);
}

/* Verify that we can successfully set/get a network without a specified BSSID. */
void test_single_no_bssid(void)
{
	int err;

	/* set network credentials without BSSID */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, NULL, 0,
					    PSK1, sizeof(PSK1), 0);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	/* retrieve network credentials without BSSID */
	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(0, strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)));
	TEST_ASSERT_EQUAL(0, flags);
	TEST_ASSERT_EQUAL(SECURITY1, security);

	err = wifi_credentials_delete_by_ssid(SSID1, sizeof(SSID1));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
}

/* Verify that we can successfully set/get a network with a fixed BSSID. */
void test_single_with_bssid(void)
{
	int err;

	/* set network credentials with BSSID */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6,
					    PSK1, sizeof(PSK1), FLAGS1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	/* retrieve network credentials with BSSID */
	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(0, strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)));
	TEST_ASSERT_EQUAL(sizeof(PSK1), psk_len);
	TEST_ASSERT_EQUAL(0, strncmp(BSSID1, bssid_buf, ARRAY_SIZE(bssid_buf)));
	TEST_ASSERT_EQUAL(WIFI_CREDENTIALS_FLAG_BSSID, flags);
	TEST_ASSERT_EQUAL(SECURITY1, security);

	err = wifi_credentials_delete_by_ssid(SSID1, sizeof(SSID1));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
}

/* Verify that we can successfully set/get an open network. */
void test_single_without_psk(void)
{
	int err;

	/* set network credentials without PSK/BSSID */
	err = wifi_credentials_set_personal(SSID2, sizeof(SSID2), SECURITY2, NULL, 6,
					    NULL, 0, FLAGS2);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	/* retrieve network credentials without PSK/BSSID */
	err = wifi_credentials_get_by_ssid_personal(SSID2, sizeof(SSID2), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(0, psk_len);
	TEST_ASSERT_EQUAL(0, flags);

	err = wifi_credentials_delete_by_ssid(SSID2, sizeof(SSID2));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
}

/* Verify that we can set/get a network that is only identified by a BSSID. */
void test_single_without_ssid(void)
{
	int err;

	err = wifi_credentials_set_personal("", 0, SECURITY1, BSSID1, 6,
					    PSK1, sizeof(PSK1), FLAGS1);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	err = wifi_credentials_get_by_ssid_personal("", 0, &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(-EINVAL, err);

	err = wifi_credentials_delete_by_ssid("", 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

/* Verify that we can handle SSIDs that contain NULL characters. */
void test_single_garbled_ssid(void)
{
	int err;

	err = wifi_credentials_set_personal(SSID4, sizeof(SSID4), SECURITY4, BSSID4, 6,
					    PSK4, sizeof(PSK4), FLAGS4);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	err = wifi_credentials_get_by_ssid_personal(SSID4, sizeof(SSID4), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(0, strncmp(PSK4, psk_buf, ARRAY_SIZE(psk_buf)));
	TEST_ASSERT_EQUAL(sizeof(PSK4), psk_len);
	TEST_ASSERT_EQUAL(0, strncmp(BSSID4, bssid_buf, ARRAY_SIZE(bssid_buf)));
	TEST_ASSERT_EQUAL(SECURITY4, security);
	TEST_ASSERT_EQUAL(FLAGS4, flags);

	err = wifi_credentials_delete_by_ssid(SSID4, sizeof(SSID4));
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
}

/* Helper function for test_set_storage_limit, making sure that the SSID cache is correct. */
void verify_ssid_cache_cb(void *cb_arg, const char *ssid, size_t ssid_len)
{
	static int call_count;
	static const char *const ssids[] = {SSID3, SSID2};

	TEST_ASSERT_EQUAL(0, strncmp(ssids[call_count++], ssid, ssid_len));
	TEST_ASSERT_EQUAL_PTR(NULL, cb_arg);
}

/* Verify that wifi_credentials behaves correctly when the storage limit is reached. */
void test_storage_limit(void)
{
	int err;

	/* Set two networks */
	err = wifi_credentials_set_personal(SSID1, sizeof(SSID1), SECURITY1, BSSID1, 6,
					    PSK1, sizeof(PSK1), FLAGS1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	err = wifi_credentials_set_personal(SSID2, sizeof(SSID2), SECURITY2, NULL, 6,
					    NULL, 0, FLAGS2);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	enum wifi_security_type security = -1;
	uint8_t bssid_buf[WIFI_MAC_ADDR_LEN] = "";
	char psk_buf[WIFI_CREDENTIALS_MAX_PASSWORD_LEN] = "";
	size_t psk_len = 0;
	uint32_t flags = 0;

	/* Get two networks */
	err = wifi_credentials_get_by_ssid_personal(SSID1, sizeof(SSID1), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(0, strncmp(PSK1, psk_buf, ARRAY_SIZE(psk_buf)));
	TEST_ASSERT_EQUAL(sizeof(PSK1), psk_len);
	TEST_ASSERT_EQUAL(0, strncmp(BSSID1, bssid_buf, ARRAY_SIZE(bssid_buf)));
	TEST_ASSERT_EQUAL(SECURITY1, security);
	TEST_ASSERT_EQUAL(FLAGS1, flags);

	err = wifi_credentials_get_by_ssid_personal(SSID2, sizeof(SSID2), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(SECURITY2, security);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(FLAGS2, flags);

	/* Set third network */
	err = wifi_credentials_set_personal(SSID3, sizeof(SSID3), SECURITY3, NULL, 6,
					    PSK3, sizeof(PSK3), FLAGS3);
	TEST_ASSERT_EQUAL(-ENOBUFS, err);

	/* Not enough space? Delete the first one. */
	wifi_credentials_delete_by_ssid(SSID1, ARRAY_SIZE(SSID1));
	err = wifi_credentials_set_personal(SSID3, sizeof(SSID3), SECURITY3, NULL, 6,
					    PSK3, sizeof(PSK3), FLAGS3);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);

	err = wifi_credentials_get_by_ssid_personal(SSID3, sizeof(SSID3), &security,
				bssid_buf, ARRAY_SIZE(bssid_buf),
				psk_buf, ARRAY_SIZE(psk_buf), &psk_len, &flags);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, err);
	TEST_ASSERT_EQUAL(SECURITY3, security);
	TEST_ASSERT_EQUAL(sizeof(PSK3), psk_len);
	TEST_ASSERT_EQUAL(0, strncmp(PSK3, psk_buf, ARRAY_SIZE(psk_buf)));
	TEST_ASSERT_EQUAL(FLAGS3, flags);

	wifi_credentials_for_each_ssid(verify_ssid_cache_cb, NULL);
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
