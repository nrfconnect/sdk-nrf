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
#include <psa/crypto_values.h>
#include <psa/crypto_extra.h>
#include <zephyr/sys/printk.h>
#include "psa_tests_common.h"

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#include <tfm_builtin_key_ids.h>
#include "cracen_psa_kmu.h"
static psa_key_id_t identity_key_id = TFM_BUILTIN_KEY_ID_IAK;
#else
#include <cracen_psa.h>
static psa_key_id_t identity_key_id = CRACEN_BUILTIN_IDENTITY_KEY_ID;
#endif

/* ======================================================================
 *			Global variables/defines for the IKG signing tests
 */

#define NRF_CRYPTO_TEST_IKG_TEXT_SIZE (68)
#define NRF_CRYPTO_TEST_IKG_SIGNATURE_SIZE (64)
#define NRF_CRYPTO_EXAMPLE_ECDSA_PUBLIC_KEY_SIZE (65)


/* Below text is used as plaintext for signing/verification */
static uint8_t m_plain_text[NRF_CRYPTO_TEST_IKG_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of the IKG identity key."
};

static uint8_t m_pub_key[NRF_CRYPTO_EXAMPLE_ECDSA_PUBLIC_KEY_SIZE];
static uint8_t m_signature[NRF_CRYPTO_TEST_IKG_SIGNATURE_SIZE];
static psa_key_handle_t key_handle;
static psa_key_id_t key_id;
/* ====================================================================== */

LOG_MODULE_DECLARE(app, LOG_LEVEL_DBG);

int get_identity_key(void)
{
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	size_t data_length;

	key_handle = mbedtls_svc_key_id_make(0, identity_key_id);
	psa_key_attributes_t attr = key_attributes;

	status = psa_get_key_attributes(key_handle, &attr);
	if (status != APP_SUCCESS) {
		return status;
	}

	status = psa_export_public_key(key_handle,
		m_pub_key,
		sizeof(m_pub_key),
		&data_length);

	if (status != PSA_SUCCESS) {
		return status;
	}
	return APP_SUCCESS;
}

int import_ecdsa_pub_key(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	status = psa_import_key(&key_attributes, m_pub_key, sizeof(m_pub_key), &key_id);
	if (status != PSA_SUCCESS) {
		return status;
	}

	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int sign_message(void)
{
	uint32_t output_len;
	psa_status_t status;

	status = psa_sign_message(identity_key_id,
		PSA_ALG_ECDSA(PSA_ALG_SHA_256),
		m_plain_text,
		sizeof(m_plain_text),
		m_signature,
		sizeof(m_signature),
		&output_len);

	if (status != PSA_SUCCESS) {
		return status;
	}

	return APP_SUCCESS;
}

int verify_message(void)
{
	psa_status_t status;

	status = psa_verify_message(key_id,
		PSA_ALG_ECDSA(PSA_ALG_SHA_256),
		m_plain_text,
		sizeof(m_plain_text),
		m_signature,
		NRF_CRYPTO_TEST_IKG_SIGNATURE_SIZE);

	if (status != PSA_SUCCESS) {
		return status;
	}

	return APP_SUCCESS;
}

int ikg_identity_key_test(void)
{
	int status;

	status = crypto_init();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = get_identity_key();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = import_ecdsa_pub_key();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = sign_message();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = verify_message();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	status = crypto_finish();
	TEST_VECTOR_ASSERT_EQUAL(APP_SUCCESS, status);

	return status;
}

ZTEST(test_suite_ikg, test_ikg_identity_key)
{
	ikg_identity_key_test();
}
