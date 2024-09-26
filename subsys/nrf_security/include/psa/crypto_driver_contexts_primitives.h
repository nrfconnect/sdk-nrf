/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 *  Declaration of context structures for use with the PSA driver wrapper
 *  interface. This file contains the context structures for 'primitive'
 *  operations, i.e. those operations which do not rely on other contexts.
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

#ifndef PSA_CRYPTO_DRIVER_CONTEXTS_PRIMITIVES_H
#define PSA_CRYPTO_DRIVER_CONTEXTS_PRIMITIVES_H

#include "oberon_psa_common.h"

#include "psa/crypto_driver_common.h"

/* Include the context structure definitions for those drivers that were
 * declared during the autogeneration process.
 */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER) || defined(PSA_NEED_CC3XX_HASH_DRIVER)
#include "cc3xx_crypto_primitives.h"
#elif defined(PSA_CRYPTO_DRIVER_CRACEN)
#include "cracen_psa_primitives.h"
#endif /* PSA_CRYPTO_DRIVER_CC3XX */

#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
#include "oberon_cipher.h"
#endif
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
#include "oberon_hash.h"
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
#if defined(PSA_NEED_CC3XX_HASH_DRIVER)
	cc3xx_hash_operation_t cc3xx_driver_ctx;
#endif
#if defined(PSA_NEED_OBERON_HASH_DRIVER)
	oberon_hash_operation_t oberon_driver_ctx;
#endif
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	cracen_hash_operation_t cracen_driver_ctx;
#endif
} psa_driver_hash_context_t;

typedef union {
	unsigned int dummy; /* Make sure this union is always non-empty */
#if defined(PSA_NEED_CC3XX_CIPHER_DRIVER)
	cc3xx_cipher_operation_t cc3xx_driver_ctx;
#endif /* PSA_CRYPTO_DRIVER_CC3XX */
#if defined(PSA_NEED_OBERON_CIPHER_DRIVER)
	oberon_cipher_operation_t oberon_driver_ctx;
#endif /* PSA_CRYPTO_DRIVER_CC3XX */
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	cracen_cipher_operation_t cracen_driver_ctx;
#endif

} psa_driver_cipher_context_t;

#endif /* PSA_CRYPTO_DRIVER_CONTEXTS_PRIMITIVES_H */
/* End of automatically generated file. */
