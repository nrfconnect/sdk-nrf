/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <errno.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_credentials_keygen.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>
#include <psa/crypto.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>

#include "nrf_cloud_credentials_keygen_internal.h"

LOG_MODULE_REGISTER(nrf_cloud_credentials_keygen, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* The PSA key id is derived directly from the sec tag: the sec tag is the
 * handle the rest of the system already uses for this credential, mirroring the
 * modem's AT%KEYGEN/AT%CMNG model where the key is bound to the sec tag. The
 * value must fall within the PSA user key id range.
 */
#define KEYGEN_KEY_ID(sec_tag) ((psa_key_id_t)(sec_tag))

/* The TLS credentials module stores the buffer pointer, not a copy, so the key
 * id passed to tls_credential_add() must remain valid for as long as the
 * credential is registered. This single static slot is the backing storage for
 * that pointer. Because the storage is shared, only one on-device key sec tag
 * can be registered at a time: registering a second sec tag would repoint this
 * slot and alias the earlier registration to the new key id. The nRF Cloud
 * library only ever uses one device key sec tag, so this is sufficient. The
 * single-sec-tag limitation is documented in the public API.
 */
static psa_key_id_t registered_key_id;
static bool key_registered;

/* A single backing slot means only one sec tag can be registered at a time.
 * Returns true if a different sec tag is already registered, so registering
 * this one would alias the earlier registration (see the slot comment above).
 */
static bool registration_conflict(uint32_t sec_tag)
{
	return key_registered && registered_key_id != KEYGEN_KEY_ID(sec_tag);
}

static int register_key(uint32_t sec_tag)
{
	int err;

	if (registration_conflict(sec_tag)) {
		LOG_DBG("A device key is already registered for sec tag %u; only one "
			"on-device key sec tag is supported at a time",
			(uint32_t)registered_key_id);
		return -ENOTSUP;
	}

	registered_key_id = KEYGEN_KEY_ID(sec_tag);

	err = tls_credential_add(sec_tag, TLS_CREDENTIAL_PRIVATE_KEY_PSA,
				 &registered_key_id, sizeof(registered_key_id));
	if (err == -EEXIST) {
		/* Already registered (for example restored earlier this boot). */
		err = 0;
	} else if (err) {
		LOG_ERR("Failed to register PSA key for sec tag %u, error: %d", sec_tag, err);
		return err;
	}

	key_registered = true;

	return 0;
}

bool nrf_cloud_credentials_keygen_key_exists(uint32_t sec_tag)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed, status: %d", status);
		return false;
	}

	/* PSA secure key storage is the source of truth for key state. */
	status = psa_get_key_attributes(KEYGEN_KEY_ID(sec_tag), &attr);
	if (status != PSA_SUCCESS) {
		LOG_DBG("Key for sec tag %u does not exist, status: %d", sec_tag, status);
		return false;
	}
	psa_reset_key_attributes(&attr);

	return true;
}

int nrf_cloud_credentials_key_generate(uint32_t sec_tag)
{
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = KEYGEN_KEY_ID(sec_tag);
	psa_key_id_t out_id;
	int err;

	if (key_id < PSA_KEY_ID_USER_MIN || key_id > PSA_KEY_ID_USER_MAX) {
		LOG_ERR("Sec tag %u maps to an invalid PSA key id", sec_tag);
		return -EINVAL;
	}

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed, status: %d", status);
		return -EIO;
	}

	/* Reject before generating so a rejected request never leaves a newly
	 * generated key behind in PSA. The single-sec-tag conflict and an already
	 * existing key are both checked up front instead of after generation.
	 */
	if (registration_conflict(sec_tag)) {
		LOG_WRN("A device key is already registered for sec tag %u; only one "
			"on-device key sec tag is supported at a time",
			(uint32_t)registered_key_id);
		return -ENOTSUP;
	}

	if (nrf_cloud_credentials_keygen_key_exists(sec_tag)) {
		LOG_WRN("Key already exists for sec tag %u; delete it before regenerating",
			sec_tag);
		return -EALREADY;
	}

	/* Persistent, non-exportable ECC P-256 key. No PSA_KEY_USAGE_EXPORT:
	 * the private key cannot be read back out of PSA. The VERIFY usages
	 * only permit public-key verification (used by the JWT signer to
	 * self-check its signature); they do not expose the private key.
	 */
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE |
				PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, key_id);

	status = psa_generate_key(&attr, &out_id);
	psa_reset_key_attributes(&attr);

	if (status == PSA_ERROR_ALREADY_EXISTS) {
		/* Existence is checked above, so this is not expected. Handle it
		 * defensively and do not roll back the pre-existing key.
		 */
		LOG_WRN("Key already exists for sec tag %u; delete it before regenerating",
			sec_tag);
		return -EALREADY;
	} else if (status != PSA_SUCCESS) {
		LOG_ERR("psa_generate_key failed, status: %d", status);
		return -EIO;
	}

	LOG_INF("Generated device key in sec tag %u", sec_tag);

	err = register_key(sec_tag);
	if (err) {
		/* Roll back the freshly generated key so a failed registration does
		 * not leave an orphaned key behind in PSA.
		 */
		(void)psa_destroy_key(key_id);
		return err;
	}

	return 0;
}

int nrf_cloud_credentials_key_delete(uint32_t sec_tag)
{
	psa_status_t status;

	(void)tls_credential_delete(sec_tag, TLS_CREDENTIAL_PRIVATE_KEY_PSA);

	status = psa_destroy_key(KEYGEN_KEY_ID(sec_tag));
	if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_HANDLE) {
		LOG_ERR("psa_destroy_key failed for sec tag %u, status: %d", sec_tag, status);
		return -EIO;
	}

	/* Free the single registration slot so another sec tag can be used. */
	if (key_registered && registered_key_id == KEYGEN_KEY_ID(sec_tag)) {
		key_registered = false;
		registered_key_id = 0;
	}

	LOG_INF("Deleted device key in sec tag %u", sec_tag);

	return 0;
}

uint32_t nrf_cloud_credentials_keygen_key_id(uint32_t sec_tag)
{
	return (uint32_t)KEYGEN_KEY_ID(sec_tag);
}

int nrf_cloud_credentials_key_restore(void)
{
	const uint32_t sec_tag = nrf_cloud_sec_tag_get();

	if (!nrf_cloud_credentials_keygen_key_exists(sec_tag)) {
		LOG_DBG("No on-device key in sec tag %u", sec_tag);
		return -ENOENT;
	}

	LOG_DBG("Restoring device key registration for sec tag %u", sec_tag);

	return register_key(sec_tag);
}

int nrf_cloud_credentials_csr_generate(uint32_t sec_tag, const char *subject,
				       uint8_t *out, size_t out_size, size_t *out_len)
{
	int ret;
	mbedtls_pk_context pk;
	mbedtls_x509write_csr csr;
	mbedtls_svc_key_id_t svc_key_id;
	psa_key_id_t key_id = KEYGEN_KEY_ID(sec_tag);

	if (subject == NULL || out == NULL || out_len == NULL) {
		return -EINVAL;
	}

	svc_key_id = mbedtls_svc_key_id_make(0, key_id);

	mbedtls_pk_init(&pk);
	mbedtls_x509write_csr_init(&csr);

	/* Wrap the PSA key as an opaque PK; the CSR self-signature is performed
	 * by PSA, so the private key is never exported.
	 */
	ret = mbedtls_pk_wrap_psa(&pk, svc_key_id);
	if (ret != 0) {
		LOG_ERR("mbedtls_pk_wrap_psa failed: -0x%04x", (unsigned int)-ret);
		ret = -EIO;
		goto cleanup;
	}

	mbedtls_x509write_csr_set_md_alg(&csr, MBEDTLS_MD_SHA256);

	ret = mbedtls_x509write_csr_set_subject_name(&csr, subject);
	if (ret != 0) {
		LOG_ERR("Invalid CSR subject name: -0x%04x", (unsigned int)-ret);
		ret = -EINVAL;
		goto cleanup;
	}

	mbedtls_x509write_csr_set_key(&csr, &pk);

	/* mbedtls_x509write_csr_der() writes to the END of the buffer and
	 * returns the length, so move the result to the front.
	 */
	ret = mbedtls_x509write_csr_der(&csr, out, out_size);
	if (ret < 0) {
		LOG_ERR("mbedtls_x509write_csr_der failed: -0x%04x", (unsigned int)-ret);
		ret = -EIO;
		goto cleanup;
	}

	*out_len = (size_t)ret;
	memmove(out, out + out_size - *out_len, *out_len);
	ret = 0;

cleanup:
	mbedtls_x509write_csr_free(&csr);
	mbedtls_pk_free(&pk);

	return ret;
}

int nrf_cloud_credentials_pubkey_get(uint32_t sec_tag, uint8_t *out, size_t out_size,
				     size_t *out_len)
{
	psa_status_t status;

	if (!IS_ENABLED(CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY)) {
		return -ENOTSUP;
	}

	if (out == NULL || out_len == NULL) {
		return -EINVAL;
	}

	(void)psa_crypto_init();

	/* The public key is never secret and is always exportable from the key
	 * pair, regardless of the key's usage policy.
	 */
	status = psa_export_public_key(KEYGEN_KEY_ID(sec_tag), out, out_size, out_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_export_public_key failed for sec tag %u, status: %d", sec_tag, status);
		return -EIO;
	}

	return 0;
}
