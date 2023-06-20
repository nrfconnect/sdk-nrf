/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 *  Declaration of context structures for use with the PSA driver wrapper
 *  interface. This file contains the context structures for 'composite'
 *  operations, i.e. those operations which need to make use of other operations
 *  from the primitives (crypto_driver_contexts_primitives.h)
 *
 *  Warning: This file will be auto-generated in the future.
 *
 * \note This file may not be included directly. Applications must
 * include psa/crypto.h.
 *
 * \note This header and its content is not part of the Mbed TLS API and
 * applications must not depend on it. Its main purpose is to define the
 * multi-part state objects of the PSA drivers included in the cryptographic
 * library. The definition of these objects are then used by crypto_struct.h
 * to define the implementation-defined types of PSA multi-part state objects.
 */

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_COMPOSITES_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_COMPOSITES_H

#include "common.h"

#include "psa/crypto_driver_common.h"

/* Include the context structure definitions for those drivers that were
 * declared during the autogeneration process.
 */
#if defined(PSA_CRYPTO_DRIVER_CC3XX)
#include "cc3xx_crypto_primitives.h"
#endif

#if defined(PSA_CRYPTO_DRIVER_HAS_MAC_SUPPORT_OBERON)
#include "oberon_mac.h"
#endif
#if defined(PSA_CRYPTO_DRIVER_HAS_AEAD_SUPPORT_OBERON)
#include "oberon_aead.h"
#endif

/* Define the context to be used for an operation that is executed through the
 * PSA Driver wrapper layer as the union of all possible driver's contexts.
 *
 * The union members are the driver's context structures, and the member names
 * are formatted as `'drivername'_ctx`. This allows for procedural generation
 * of both this file and the content of psa_crypto_driver_wrappers.c
 */

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#if defined(PSA_CRYPTO_DRIVER_CC3XX)
	cc3xx_mac_operation_t cc3xx_driver_ctx;
#endif
#if defined(PSA_CRYPTO_DRIVER_HAS_MAC_SUPPORT_OBERON)
	oberon_mac_operation_t oberon_driver_ctx;
#endif
} psa_driver_mac_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#if defined(PSA_CRYPTO_DRIVER_CC3XX)
	struct {
		cc3xx_aead_operation_t cc3xx_driver_ctx;
	};
#endif /* PSA_CRYPTO_DRIVER_CC3XX */
#if defined(PSA_CRYPTO_DRIVER_HAS_AEAD_SUPPORT_OBERON)
	oberon_aead_operation_t oberon_driver_ctx;
#endif

} psa_driver_aead_context_t;

#endif /* PSA_CRYPTO_DRIVER_CONTEXTS_COMPOSITES_H */
/* End of automatically generated file. */
