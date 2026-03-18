/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_hmac
 * @{
 * @brief Low-level HMAC primitives for the CRACEN driver.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_HMAC_H
#define CRACEN_HMAC_H

#include <psa/crypto.h>
#include <stdint.h>

int mac_create_hmac(const struct sxhashalg *hashalg, struct sxhash *hashopctx, const uint8_t *key,
		    size_t keysz, uint8_t *workmem, size_t workmemsz);

int hmac_produce(struct sxhash *hashctx, const struct sxhashalg *hashalg, uint8_t *out, size_t sz,
		 uint8_t *workmem);

/** @} */

#endif /* CRACEN_HMAC_H */
