/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KEY_IDS_H
#define CRACEN_PSA_KEY_IDS_H

#define CRACEN_BUILTIN_IDENTITY_KEY_ID ((uint32_t)0x7fffc001)
#define CRACEN_BUILTIN_MKEK_ID	       ((uint32_t)0x7fffc002)
#define CRACEN_BUILTIN_MEXT_ID	       ((uint32_t)0x7fffc003)

#define CRACEN_PROTECTED_RAM_AES_KEY0_ID ((uint32_t)0x7fffc004)

#define CRACEN_IDENTITY_KEY_SLOT_NUMBER 0
#define CRACEN_MKEK_SLOT_NUMBER		1
#define CRACEN_MEXT_SLOT_NUMBER		2

#define PSA_KEY_LOCATION_CRACEN ((psa_key_location_t)(0x800000 | ('N' << 8)))

/*
 * Defines a persistence state where deleted keys are permanently revoked.
 * In this state, once a key is deleted, its corresponding slot cannot be provisioned again.
 */
#define CRACEN_KEY_PERSISTENCE_REVOKABLE 0x02

/*
 * Defines a persistence state where the key can't be erased
 * In this state the key will only be erased if ERASEALL is available and run
 */
 #define CRACEN_KEY_PERSISTENCE_READ_ONLY 0x03

#endif
