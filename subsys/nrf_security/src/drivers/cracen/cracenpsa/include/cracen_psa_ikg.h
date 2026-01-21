/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_ikg
 * @{
 * @brief Internal Key Generation (IKG) support for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * This module provides APIs for generating and using identity keys through
 * the CRACEN hardware's Internal Key Generation (IKG) feature.
 */

#ifndef CRACEN_PSA_IKG_H
#define CRACEN_PSA_IKG_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <psa/crypto.h>

psa_status_t cracen_ikg_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes,
					uint8_t *key_buffer, size_t key_buffer_size,
					size_t *key_buffer_length);

/** @} */

#endif /* CRACEN_PSA_IKG_H */
