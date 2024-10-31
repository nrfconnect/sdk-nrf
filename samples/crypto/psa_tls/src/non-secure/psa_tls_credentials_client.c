/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psa_tls_credentials_client_non_secure);

#include <nrf.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/linker/sections.h>

#include "certificate.h"
#include "psa_tls_credentials.h"
#include "dummy_psk.h"

#include <tfm_ns_interface.h>
#include <psa/storage_common.h>
#include "psa/protected_storage.h"

static unsigned char ca_cert_buf[sizeof(ca_certificate)];
static unsigned char psk_buf[sizeof(psk)];
static unsigned char psk_id_buf[sizeof(psk_id)];

/** @brief Function for storing the CA certificate in Protected Storage.
 */
static int tls_store_credentials_in_ps(void)
{
	LOG_INF("Storing Client CA certificate in Protected Storage");

	psa_status_t status = psa_ps_set(PSA_PS_CA_CERTIFICATE_UID,
					 sizeof(ca_certificate),
					 ca_certificate,
					 PSA_STORAGE_FLAG_WRITE_ONCE);
	if (status == PSA_ERROR_NOT_PERMITTED) {
		LOG_INF("CA certificate is already "
			"stored in Protected Storage");
	} else if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to store CA certificate to"
			" Protected Storage. Status: %d", status);
		return status;
	}

	return APP_SUCCESS;
}

/** @brief Function for fetching the CA certificate from Protected Storage,
 * and registering it for the TLS handshake.
 */
static int tls_set_credentials_from_ps(void)
{
	size_t cred_len;

	LOG_INF("Registering Client CA certificate from Protected Storage");

	psa_status_t status = psa_ps_get(PSA_PS_CA_CERTIFICATE_UID,
					 (size_t) 0,
					 sizeof(ca_certificate),
					 ca_cert_buf,
					 &cred_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to retrieve CA certificate from"
			" Protected Storage. Status: %d", status);
		return status;
	}

	int err = tls_credential_add(CA_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_CA_CERTIFICATE,
				     ca_cert_buf, cred_len);
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate from"
			" Protected Storage: %d", err);
		return err;
	}

	return APP_SUCCESS;
}

/** @brief Function for storing the Pre-shared key in Protected Storage.
 */
static int tls_store_psk_in_ps(void)
{
	LOG_INF("Storing Client PSK in Protected Storage");

	psa_status_t status = psa_ps_set(PSA_PS_PSK_UID, sizeof(psk),
					 psk, PSA_STORAGE_FLAG_WRITE_ONCE);
	if (status == PSA_ERROR_NOT_PERMITTED) {
		LOG_INF("PSK is already "
			"stored in Protected Storage");
	} else if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to store PSK to Protected"
			" Storage. Status: %d",
			status);
		return status;
	}

	status = psa_ps_set(PSA_PS_PSK_IDENTITY_UID, sizeof(psk_id), psk_id,
			    PSA_STORAGE_FLAG_WRITE_ONCE);
	if (status == PSA_ERROR_NOT_PERMITTED) {
		LOG_INF("PSK ID is already "
			"stored in Protected Storage");
	} else if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to store PSK ID to Protected Storage."
			" Status: %d",
			status);
		return status;
	}

	return APP_SUCCESS;
}

/** @brief Function for fetching Pre-shared key from Protected Storage,
 * and registering it for the TLS handshake.
 */
static int tls_set_psk_from_ps(void)
{
	size_t key_len;
	size_t id_len;

	LOG_INF("Registering PSK from Protected Storage");

	psa_status_t status = psa_ps_get(PSA_PS_PSK_UID,
					 (size_t) 0,
					 sizeof(psk),
					 psk_buf,
					 &key_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to retrieve PSK from"
			" Protected Storage. Status: %d", status);
		return status;
	}

	status = psa_ps_get(PSA_PS_PSK_IDENTITY_UID,
			    (size_t) 0,
			    strlen(psk_id),
			    psk_id_buf,
			    &id_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to retrieve PSK ID from"
			" Protected Storage. Status: %d", status);
		return status;
	}

	int err = tls_credential_add(PSK_TAG,
				     TLS_CREDENTIAL_PSK,
				     psk_buf, key_len);
	if (err < 0) {
		LOG_ERR("Failed to register PSK from"
			" Protected Storage: %d", err);
		return err;
	}

	err = tls_credential_add(PSK_TAG,
				     TLS_CREDENTIAL_PSK_ID,
				     psk_id_buf, id_len);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID from"
			" Protected Storage: %d", err);
		return err;
	}

	return APP_SUCCESS;
}

int tls_set_credentials(void)
{
	int err;

	err = tls_store_credentials_in_ps();
	if (err < 0) {
		return err;
	}

	err = tls_set_credentials_from_ps();
	if (err < 0) {
		return err;
	}

	err = tls_store_psk_in_ps();
	if (err < 0) {
		return err;
	}

	err = tls_set_psk_from_ps();
	if (err < 0) {
		return err;
	}

	return APP_SUCCESS;
}
