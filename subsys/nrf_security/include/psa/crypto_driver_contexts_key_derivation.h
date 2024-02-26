/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_KEY_DERIVATION_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_KEY_DERIVATION_H

#include "psa/crypto_driver_common.h"

/* Include the context structure definitions for enabled drivers. */

#ifdef PSA_NEED_OBERON_KEY_DERIVATION_DRIVER
#include "oberon_key_derivation.h"
#endif
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
#include "oberon_pake.h"
#endif
#ifdef PSA_NEED_OBERON_CTR_DRBG_DRIVER
#include "oberon_ctr_drbg.h"
#endif
#ifdef PSA_NEED_OBERON_HMAC_DRBG_DRIVER
#include "oberon_hmac_drbg.h"
#endif

#if defined(PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER) || defined(PSA_NEED_CRACEN_PAKE_DRIVER)
#include "cracen_psa_primitives.h"
#endif

/*
 * Define the context to be used for an operation that is executed through the
 * PSA Driver wrapper layer as the union of all possible drivers' contexts.
 *
 * The union members are the driver's context structures, and the member names
 * are formatted as `'drivername'_ctx`.
 */

typedef union {
	/* Make sure this union is always non-empty */
	unsigned int dummy;
#ifdef PSA_NEED_OBERON_KEY_DERIVATION_DRIVER
	oberon_key_derivation_operation_t oberon_kdf_ctx;
#endif
#ifdef PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER
	cracen_key_derivation_operation_t cracen_kdf_ctx;
#endif
} psa_driver_key_derivation_context_t;

typedef union {
	/* Make sure this union is always non-empty */
	unsigned int dummy;
#ifdef PSA_NEED_OBERON_PAKE_DRIVER
	oberon_pake_operation_t oberon_pake_ctx;
#endif
#ifdef PSA_NEED_CRACEN_PAKE_DRIVER
	cracen_pake_operation_t cracen_pake_ctx;
#endif
} psa_driver_pake_context_t;

typedef union {
	/* Make sure this union is always non-empty */
	unsigned int dummy;
#ifdef PSA_NEED_OBERON_CTR_DRBG_DRIVER
	oberon_ctr_drbg_context_t oberon_ctr_drbg_ctx;
#endif
#ifdef PSA_NEED_OBERON_HMAC_DRBG_DRIVER
	oberon_hmac_drbg_context_t oberon_hmac_drbg_ctx;
#endif
} psa_driver_random_context_t;

#endif /* PSA_CRYPTO_DRIVER_CONTEXTS_KEY_DERIVATION_H */
