/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H

#include "psa/crypto_driver_common.h"

/* Include the context structure definitions for enabled drivers. */

#if defined(PSA_NEED_OBERON_KDF_DRIVER)
#include "oberon_kdf.h"
#endif

#if defined(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
#include "oberon_ctr_drbg.h"
#endif

#if defined(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
#include "oberon_hmac_drbg.h"
#endif

#if defined(PSA_NEED_OBERON_JPAKE_DRIVER)
#include "oberon_jpake.h"
#endif

#if defined(PSA_NEED_OBERON_SPAKE2P_DRIVER)
#include "oberon_spake2p.h"
#endif

#if defined(PSA_NEED_OBERON_SRP_DRIVER)
#include "oberon_srp.h"
#endif

/* Define the context to be used for an operation that is executed through the
 * PSA Driver wrapper layer as the union of all possible drivers' contexts.
 *
 * The union members are the driver's context structures, and the member names
 * are formatted as `'drivername'_ctx`.
 */

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#ifdef PSA_NEED_OBERON_KDF_DRIVER
	oberon_key_derivation_operation_t oberon_kdf_ctx;
#endif
} psa_driver_key_derivation_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#if defined(PSA_NEED_OBERON_JPAKE_DRIVER)
	oberon_jpake_operation_t oberon_jpake_ctx;
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_DRIVER)
	oberon_spake2p_operation_t oberon_spake2p_ctx;
#endif
#if defined(PSA_NEED_OBERON_SRP_DRIVER)
	oberon_srp_operation_t oberon_srp_ctx;
#endif
} psa_driver_pake_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */

#if defined(PSA_NEED_OBERON_CTR_DRBG_DRIVER)
	oberon_ctr_drbg_context_t oberon_ctr_drbg_ctx;
#endif
#if defined(PSA_NEED_OBERON_HMAC_DRBG_DRIVER)
	oberon_hmac_drbg_context_t oberon_hmac_drbg_ctx;
#endif

} psa_driver_random_context_t;

#endif /* PSA_CRYPTO_DRIVER_CONTEXTS_KDF_H */
