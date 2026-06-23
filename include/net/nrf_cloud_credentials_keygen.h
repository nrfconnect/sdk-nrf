/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CREDENTIALS_KEYGEN_H_
#define NRF_CLOUD_CREDENTIALS_KEYGEN_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_cloud_credentials_keygen nRF Cloud on-device key generation
 * @{
 * @brief On-device generation of the nRF Cloud device private key and a
 *        Certificate Signing Request (CSR).
 *
 * The key is generated with the PSA Crypto API as a persistent, non-exportable
 * ECC P-256 key and is referenced for TLS by its PSA key id, so the private key
 * never leaves the device. These functions allow an application to drive the
 * key generation and CSR extraction directly (for example to onboard a device
 * over BLE, NFC, or another transport), instead of using the shell commands.
 *
 * This API is available when @kconfig{CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN} is
 * enabled.
 *
 * @note Only a single on-device key sec tag can be registered at a time. The
 *       nRF Cloud library uses one device key sec tag, and the opaque-key
 *       registration relies on a single backing slot, so generating or
 *       restoring a key for a second sec tag while another is registered is
 *       rejected. Delete the existing key first to switch to a different sec
 *       tag.
 */

/** @brief Generate the device private key on-device for the given sec tag.
 *
 * Creates a persistent, non-exportable ECC P-256 key pair in PSA secure key
 * storage and registers it with the TLS credentials module as an opaque key
 * (@c TLS_CREDENTIAL_PRIVATE_KEY_PSA). The private key never leaves the device.
 *
 * @param sec_tag The security tag the key is associated with.
 *
 * @retval 0 If successful.
 * @retval -EALREADY If a key already exists for the sec tag. Delete it first.
 * @retval -ENOTSUP If a key is already registered for a different sec tag.
 *                  Only one on-device key sec tag is supported at a time.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_key_generate(uint32_t sec_tag);

/** @brief Delete the on-device device private key for the given sec tag.
 *
 * Destroys the PSA key and removes its TLS credential registration.
 *
 * @param sec_tag The security tag the key is associated with.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_key_delete(uint32_t sec_tag);

/** @brief Generate a Certificate Signing Request (CSR) for the device key.
 *
 * The CSR is signed by the on-device key through PSA and written to @p out in
 * DER format.
 *
 * @param sec_tag  The security tag whose key is used to sign the CSR.
 * @param subject  X.509 subject name string (for example "O=Nordic,CN=<id>").
 * @param out      Destination buffer for the DER-encoded CSR.
 * @param out_size Size of @p out in bytes.
 * @param out_len  On success, set to the number of DER bytes written to @p out.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_csr_generate(uint32_t sec_tag, const char *subject,
				       uint8_t *out, size_t out_size, size_t *out_len);

/** @brief Export the public key of the on-device key pair.
 *
 * The private key is non-exportable; only the public key is returned. Host
 * tooling can compare it against the public key in the installed device
 * certificate to verify that the on-device key matches the certificate.
 *
 * The public key is exported in SEC1 uncompressed point format
 * (``0x04 || X || Y``), which is 65 bytes for an ECC P-256 key.
 *
 * The @kconfig{CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY} Kconfig must be enabled
 * to make this API functional.
 *
 * @param sec_tag  The security tag whose key is exported.
 * @param out      Destination buffer for the public key.
 * @param out_size Size of @p out in bytes.
 * @param out_len  On success, set to the number of bytes written to @p out.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY is disabled.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_pubkey_get(uint32_t sec_tag, uint8_t *out, size_t out_size,
				     size_t *out_len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CREDENTIALS_KEYGEN_H_ */
