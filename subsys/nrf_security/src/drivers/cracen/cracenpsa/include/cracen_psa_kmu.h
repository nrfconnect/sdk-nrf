/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KMU_H
#define CRACEN_PSA_KMU_H

#include <psa/crypto.h>

#ifdef CONFIG_BUILD_WITH_TFM
/** A slot number identifying a key in a driver.
 *
 * Values of this type are used to identify built-in keys.
 */
typedef uint64_t psa_drv_slot_number_t;
#endif

#define PSA_KEY_LOCATION_CRACEN_KMU (PSA_KEY_LOCATION_VENDOR_FLAG | ('N' << 8) | 'K')

/* Key id 0x7fffXYZZ */
/* X: Key usage scheme */
/* Y: Reserved (0) */
/* Z: KMU Slot index */

/* Construct a PSA Key handle for key stored in KMU */
#define PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(scheme, slot_id)                                       \
	(0x7fff0000 | ((scheme) << 12) | ((slot_id)&0xff))

/* Retrieve key usage scheme from PSA key id. */
#define CRACEN_PSA_GET_KEY_USAGE_SCHEME(key_id) (((key_id) >> 12) & 0xf)

/* Retrieve KMU slot number for PSA key id. */
#define CRACEN_PSA_GET_KMU_SLOT(key_id) ((key_id)&0xff)

enum cracen_kmu_metadata_key_usage_scheme {
	/**
	 * These keys can only be pushed to CRACEN's protected RAM.
	 * The keys are not encrypted. Only AES supported.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,
	/**
	 * CRACEN's IKG seed uses 3 key slots. Pushed to the seed register.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_SEED,
	/**
	 * These keys are stored in encrypted form. They will be decrypted
	 * to @ref kmu_push_area for usage.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED,
	/**
	 * These keys are not encrypted. Pushed to @ref kmu_push_area.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_RAW
};

/**
 * @brief Retrieves the slot number for a given key handle.
 *
 * @param[in]  key_id      Key handler.
 * @param[out] lifetime    Lifetime for key.
 * @param[out] slot_number The key's slot number.
 *
 * @return psa_status_t
 */
psa_status_t cracen_kmu_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				     psa_drv_slot_number_t *slot_number);

#endif /* CRACEN_PSA_KMU_H */
