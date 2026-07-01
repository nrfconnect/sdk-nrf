/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CREDENTIALS_KEYGEN_INTERNAL_H_
#define NRF_CLOUD_CREDENTIALS_KEYGEN_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal nRF Cloud on-device key generation API.
 *
 * These functions are used by the nRF Cloud library itself (transport
 * initialization and JWT signing) and are not part of the public API.
 */

/** @brief Get the PSA key id used for the on-device key of the given sec tag.
 *
 * @param sec_tag The security tag the key is associated with.
 *
 * @return The PSA key id (psa_key_id_t) for the on-device key.
 */
uint32_t nrf_cloud_credentials_keygen_key_id(uint32_t sec_tag);

/** @brief Check whether an on-device key exists in PSA secure key storage.
 *
 * This queries PSA, the source of truth for key state, so it returns the
 * correct result regardless of whether the key has been registered with the
 * TLS credentials module yet (registration happens during nrf_cloud
 * initialization). Use this instead of probing the TLS credentials module to
 * avoid a false negative when checking credentials before initialization.
 *
 * @param sec_tag The security tag the key is associated with.
 *
 * @retval true If a key exists for the sec tag.
 * @retval false Otherwise.
 */
bool nrf_cloud_credentials_keygen_key_exists(uint32_t sec_tag);

/** @brief Re-register the on-device device private key with the TLS credentials
 * module from PSA secure key storage.
 *
 * Called during nrf_cloud initialization so that a key generated in a previous
 * session is available for the TLS handshake after reboot. Operates on the sec
 * tag returned by @c nrf_cloud_sec_tag_get().
 *
 * @retval 0 If successful.
 * @retval -ENOENT If no on-device key exists for the nRF Cloud sec tag.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_key_restore(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CREDENTIALS_KEYGEN_INTERNAL_H_ */
