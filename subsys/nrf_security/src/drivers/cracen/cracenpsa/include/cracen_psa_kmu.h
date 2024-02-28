/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KMU_H
#define CRACEN_PSA_KMU_H

#include <psa/crypto.h>

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

#define CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED 0
#define CRACEN_KMU_KEY_USAGE_SCHEME_SEED      1
#define CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED 2
#define CRACEN_KMU_KEY_USAGE_SCHEME_RAW	      3

#endif /* CRACEN_PSA_KMU_H */
