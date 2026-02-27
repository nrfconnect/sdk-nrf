/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef PSA_CORE_LITE_H_
#define PSA_CORE_LITE_H_

#include "util/util_macro.h"
#include <psa/crypto.h>

/* Macros signalling crypto support */

#if defined(PSA_WANT_ALG_SHA_256) || defined(PSA_WANT_ALG_SHA_384) 	     || \
    defined(PSA_WANT_ALG_SHA_512)
#define PSA_CORE_LITE_SUPPORTS_HASH
#endif

#if (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) || \
     defined(PSA_WANT_ALG_PURE_EDDSA) || defined(PSA_WANT_ALG_RSA_PSS)	      || \
     defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN)) 				      && \
     defined(PSA_CORE_LITE_SUPPORTS_HASH)
#define PSA_CORE_LITE_SUPPORTS_VERIFY_MESSAGE
#endif

#if (defined(PSA_WANT_ALG_ECDSA) || defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) || \
     defined(PSA_WANT_ALG_ED25519PH) || defined(PSA_WANT_ALG_RSA_PSS) 	      || \
     defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN))				      && \
     defined(PSA_CORE_LITE_SUPPORTS_HASH)
#define PSA_CORE_LITE_SUPPORTS_VERIFY_HASH
#endif

#if (defined(PSA_WANT_ALG_RSA_PSS) || defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN))
#define PSA_CORE_LITE_SUPPORTS_RSA
#else
#define PSA_CORE_LITE_SUPPORTS_KMU
#endif

/* Macros to verify different signature algorithms */

#define VERIFY_ALG_ED25519(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA,				\
			       PSA_WANT_ECC_TWISTED_EDWARDS_255),		\
		(_alg == PSA_ALG_PURE_EDDSA))

#define VERIFY_ALG_ED25519PH(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH,				\
			       PSA_WANT_ECC_TWISTED_EDWARDS_255),		\
		(_alg == PSA_ALG_ED25519PH))

#define VERIFY_ALG_ECDSA_SECP_R1_256(_alg)					\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_ECDSA, PSA_WANT_ECC_SECP_R1_256),	\
		PSA_ALG_IS_ECDSA(_alg))

#define VERIFY_ALG_ECDSA_SECP_R1_384(_alg)					\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_ECDSA, PSA_WANT_ECC_SECP_R1_384),	\
		PSA_ALG_IS_ECDSA(_alg))

#define VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(_alg)			\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_DETERMINISTIC_ECDSA,		\
			       PSA_WANT_ECC_SECP_R1_256,			\
			       PSA_WANT_ALG_SHA_256),				\
		PSA_ALG_ECDSA_IS_DETERMINISTIC(_alg))

#define VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(_alg)			\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_DETERMINISTIC_ECDSA,		\
			       PSA_WANT_ECC_SECP_R1_384,			\
			       PSA_WANT_ALG_SHA_384),				\
		PSA_ALG_ECDSA_IS_DETERMINISTIC(_alg))

#define VERIFY_ALG_RSA_PSS(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED(PSA_WANT_ALG_RSA_PSS), 				\
		PSA_ALG_IS_RSA_PSS(_alg))

#define VERIFY_ALG_RSA_PKCS1V15(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED(PSA_WANT_ALG_RSA_PKCS1V15_SIGN),			\
		PSA_ALG_IS_RSA_PKCS1V15_SIGN(_alg))

#define VERIFY_ALG_ANY_SIGNED_MESSAGE(_alg)					\
	UTIL_CONCAT_OR(								\
		VERIFY_ALG_ED25519(_alg),					\
		VERIFY_ALG_ECDSA_SECP_R1_256(_alg),				\
		VERIFY_ALG_ECDSA_SECP_R1_384(_alg),				\
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(_alg),		\
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(_alg),		\
		VERIFY_ALG_RSA_PSS(_alg),					\
		VERIFY_ALG_RSA_PKCS1V15(_alg))					\

#define VERIFY_ALG_ANY_SIGNED_HASH(_alg)					\
	UTIL_CONCAT_OR(								\
		VERIFY_ALG_ED25519PH(_alg),					\
		VERIFY_ALG_ECDSA_SECP_R1_256(_alg),				\
		VERIFY_ALG_ECDSA_SECP_R1_384(_alg),				\
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(_alg),		\
		VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(_alg),		\
		VERIFY_ALG_RSA_PSS(_alg),					\
		VERIFY_ALG_RSA_PKCS1V15(_alg))

/* Macros to verify different hash algorithms */

#define VERIFY_ALG_SHA_256(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_256) && _alg == PSA_ALG_SHA_256)

#define VERIFY_ALG_SHA_384(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_384) && _alg == PSA_ALG_SHA_384)

#define VERIFY_ALG_SHA_512(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_512) && _alg == PSA_ALG_SHA_512)

#define VERIFY_ALG_ANY_HASH(_alg) 						\
	UTIL_CONCAT_OR(VERIFY_ALG_SHA_256(alg),					\
		       VERIFY_ALG_SHA_384(alg),					\
		       VERIFY_ALG_SHA_512(alg))

/* Macros to verify different encryption algorithms */

#define VERIFY_ALG_CTR(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_CTR) && (_alg == PSA_ALG_CTR))

#endif /* PSA_CORE_LITE_H_ */
