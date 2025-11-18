/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <cracen_psa_kmu.h>

#define PSA_KEY_INDEX_SIZE 2
#define PSA_KEY_STARTING_ID_BL 226
#define PSA_KEY_STARTING_ID_UROT 242

#define MAKE_PSA_KMU_KEY_ID(id) \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, id)

static psa_key_id_t key_ids[] =  {
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_BL),
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_BL + PSA_KEY_INDEX_SIZE),
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_BL + (2 * PSA_KEY_INDEX_SIZE)),
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_UROT),
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_UROT + PSA_KEY_INDEX_SIZE),
	MAKE_PSA_KMU_KEY_ID(PSA_KEY_STARTING_ID_UROT + (2 * PSA_KEY_INDEX_SIZE)),
};

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#define BUILD_WITH_B0 (CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER != -1)
#else
#define BUILD_WITH_B0 CONFIG_SECURE_BOOT
#endif


#if defined(CONFIG_BOOTLOADER_MCUBOOT) && !BUILD_WITH_B0
static bool locked[] = {false, false, false, true, true, true};
#elif !defined(CONFIG_BOOTLOADER_MCUBOOT) && BUILD_WITH_B0
static bool locked[] = {false, false, false, true, true, true};
#elif defined(CONFIG_BOOTLOADER_MCUBOOT) && BUILD_WITH_B0
static bool locked[] = {true, true, true, true, true, true};
#else
/* No bootloader is used, so all keys should be unlocked */
static bool locked[] = {false, false, false, false, false, false};
#endif

#define EDDSA_SIGNATURE_LENGTH  64

uint8_t signature[EDDSA_SIGNATURE_LENGTH] = {
	0x30, 0x31, 0x80, 0x08, 0x97, 0xe0, 0x90, 0x50,
	0xca, 0x3a, 0xe9, 0x01, 0xd5, 0x99, 0x24, 0x63,
	0x79, 0x73, 0x6a, 0x54, 0xe0, 0x95, 0x14, 0x33,
	0x77, 0x6b, 0xb6, 0xc4, 0x33, 0xc8, 0xbc, 0xcb,
	0x31, 0xfc, 0xb2, 0x30, 0xe2, 0x09, 0x2e, 0x74,
	0x76, 0xcf, 0x18, 0xc3, 0xc3, 0xe7, 0x74, 0x96,
	0x7e, 0xd0, 0xc4, 0x16, 0xb2, 0xf4, 0x9f, 0x08,
	0x37, 0x6e, 0x83, 0x23, 0x6d, 0xf5, 0x25, 0x0a
};
uint8_t message[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};

ZTEST(bootloader_kmu_keys_lock, test_smoke_test_keys_unlocked)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;
	psa_key_id_t key_id = 0;

	zassert_equal(psa_crypto_init(), PSA_SUCCESS);

	for (int i = 0; i < ARRAY_SIZE(key_ids); i++) {
		key_id = key_ids[i];
		printk("Verifying if key 0x%x is locked, should be %s\n", key_id,
		       locked[i] ? "locked" : "unlocked");
		status = psa_verify_message(key_id, PSA_ALG_PURE_EDDSA, message,
					     sizeof(message), signature,
					     sizeof(signature));
		if (locked[i]) {
			zassert_equal(status, PSA_ERROR_INVALID_ARGUMENT,
				      "Key 0x%x should be locked (expected status %d, got %d)",
				      key_id, PSA_ERROR_INVALID_ARGUMENT, status);
		} else {
			zassert_equal(status, PSA_SUCCESS,
				      "Key 0x%x should be unlocked (expected status %d, got %d)",
				      key_id, PSA_SUCCESS, status);
		}
	}
}

ZTEST_SUITE(bootloader_kmu_keys_lock, NULL, NULL, NULL, NULL, NULL);
