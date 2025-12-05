/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "cracen_psa_kmu.h"
#include "psa_tests_common.h"

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

LOG_MODULE_DECLARE(app, LOG_LEVEL_DBG);

/* ====================================================================== */
/*			Global variables/defines for the IKG encryption sample		  */
#define NRF_CRYPTO_EXAMPLE_EDDSA_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_EDDSA_SIGNATURE_SIZE (64)
#define KMU_SLOT_NUM 125

/* Below text is used as plaintext for signing/verification */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_EDDSA_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of EDDSA."
};

/* Below signature is pregenerated for use with verifying the message */
static uint8_t m_signature[NRF_CRYPTO_EXAMPLE_EDDSA_SIGNATURE_SIZE] = {
	0x81, 0xf5, 0x22, 0xd3, 0xa1, 0x6d, 0x26, 0x0d, 0xc8, 0xc4, 0xd6, 0xbd, 0x51,
	0xf0, 0xf8, 0x46, 0x09, 0xdc, 0x00, 0x85, 0x60, 0x56, 0x99, 0xbd, 0xc3, 0x97,
	0x5e, 0xa6, 0x6f, 0x7d, 0x0c, 0x97, 0xd9, 0xeb, 0xf0, 0x1c, 0xeb, 0xff, 0xbd,
	0x6f, 0x16, 0x7b, 0xa8, 0x58, 0x7f, 0xc2, 0x86, 0xbb, 0x5e, 0x8d, 0x46, 0xef,
	0xf0, 0x58, 0x41, 0xe5, 0xac, 0x42, 0x2c, 0xd1, 0xed, 0x29, 0x74, 0x0e
};
static psa_key_handle_t key_handle;
/* ====================================================================== */


int get_eddsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	key_handle = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, KMU_SLOT_NUM);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	status = psa_get_key_attributes(key_handle, &attr);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int verify_message_kmu(void)
{
	psa_status_t status;

	status = get_eddsa_pub_key();
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Verify the signature of the message */
	status = psa_verify_message(key_handle,
		PSA_ALG_PURE_EDDSA,
		m_plain_text,
		sizeof(m_plain_text),
		m_signature,
		NRF_CRYPTO_EXAMPLE_EDDSA_SIGNATURE_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return APP_SUCCESS;
}

int kmu_use(void)
{
	int status;

	status = crypto_init();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = verify_message_kmu();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	return status;
}

ZTEST(test_suite_ikg, test_kmu_2_use)
{
	kmu_use();
}
