/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef PSA_CRYPTO_LITE__
#define PSA_CRYPTO_LITE__

#include "assert.h"
#include "zephyr/sys/util_macro.h"
#include <psa/crypto.h>

/* Macros to verify different signature algorithms */

#define VERIFY_ALG_ED25519(_alg)						\
	UTIL_AND(IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA,			\
				PSA_WANT_ECC_TWISTED_EDWARDS_255),		\
				_alg == PSA_ALG_PURE_EDDSA)

#define VERIFY_ALG_ED25519PH(_alg)						\
	UTIL_AND(IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH,				\
				PSA_WANT_ECC_TWISTED_EDWARDS_255),		\
				_alg == PSA_APSA_ALG_ED25519PH)

#define VERIFY_ALG_ECDSA_SECP_R1_256(_alg)					\
	UTIL_AND(IS_ENABLED_ALL(PSA_WANT_ALG_ECDSA, PSA_WANT_ECC_SECP_R1_256),	\
				PSA_ALG_IS_ECDSA(_alg))

#define VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(_alg)			\
	UTIL_AND(IS_ENABLED_ALL(PSA_WANT_ALG_DETERMINISTIC_ECDSA,		\
				PSA_WANT_ECC_SECP_R1_256),			\
				PSA_ALG_ECDSA_IS_DETERMINISTIC(_alg))

/* Macros to verify different hash algorithms */

#define VERIFY_ALG_SHA_256(_alg) \
	UTIL_AND(IS_ENABLED(PSA_WANT_ALG_SHA_256), _alg == PSA_ALG_SHA_256)

#define VERIFY_ALG_SHA_512(_alg) \
	UTIL_AND(IS_ENABLED(PSA_WANT_ALG_SHA_512), _alg == PSA_ALG_SHA_512)

/* Macros to verify */

#endif /* PSA_CRYPTO_LITE__ */
