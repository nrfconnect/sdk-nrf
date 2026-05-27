/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef PSA_CORE_LITE_H_
#define PSA_CORE_LITE_H_

#include <zephyr/sys/util.h>
#include "util/util_macro.h"
#include <psa/crypto.h>

/* Macros to verify different signature algorithms */

#define VERIFY_ALG_ED25519(_alg)						\
	UTIL_CONCAT_AND(IS_ENABLED_ALL(PSA_WANT_ALG_PURE_EDDSA,			\
				       PSA_WANT_ECC_TWISTED_EDWARDS_255),	\
			_alg == PSA_ALG_PURE_EDDSA)

#define VERIFY_ALG_ED25519PH(_alg)						\
	UTIL_CONCAT_AND(IS_ENABLED_ALL(PSA_WANT_ALG_ED25519PH,			\
				       PSA_WANT_ECC_TWISTED_EDWARDS_255),	\
			_alg == PSA_ALG_ED25519PH)

#define VERIFY_ALG_ECDSA_SECP_R1_256(_alg)					\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_ECDSA, PSA_WANT_ECC_SECP_R1_256),	\
		PSA_ALG_IS_ECDSA(_alg))

#define VERIFY_ALG_ECDSA_SECP_R1_384(_alg)					\
	UTIL_CONCAT_AND(							\
		IS_ENABLED_ALL(PSA_WANT_ALG_ECDSA, PSA_WANT_ECC_SECP_R1_384),	\
		PSA_ALG_IS_ECDSA(_alg))

#define VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_256(_alg)			\
	UTIL_CONCAT_AND(IS_ENABLED_ALL(PSA_WANT_ALG_DETERMINISTIC_ECDSA,	\
				       PSA_WANT_ECC_SECP_R1_256),		\
			PSA_ALG_ECDSA_IS_DETERMINISTIC(_alg))

#define VERIFY_ALG_DETERMINISTIC_ECDSA_SECP_R1_384(_alg)			\
	UTIL_CONCAT_AND(IS_ENABLED_ALL(PSA_WANT_ALG_DETERMINISTIC_ECDSA,	\
				       PSA_WANT_ECC_SECP_R1_384),		\
			PSA_ALG_ECDSA_IS_DETERMINISTIC(_alg))

#define VERIFY_ALG_RSA_PSS(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED(PSA_WANT_ALG_RSA_PSS),				\
		PSA_ALG_IS_RSA_PSS(_alg))

#define VERIFY_ALG_RSA_PKCS1V15(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED(PSA_WANT_ALG_RSA_PKCS1V15_SIGN),			\
		PSA_ALG_IS_RSA_PKCS1V15_SIGN(_alg))

#define VERIFY_ALG_RSA_OAEP(_alg)						\
	UTIL_CONCAT_AND(							\
		IS_ENABLED(PSA_WANT_ALG_RSA_OAEP),				\
		PSA_ALG_IS_RSA_OAEP(_alg))

/* Macros to verify different hash algorithms */

#define VERIFY_ALG_SHA_256(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_256) && _alg == PSA_ALG_SHA_256)

#define VERIFY_ALG_SHA_384(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_384) && _alg == PSA_ALG_SHA_384)

#define VERIFY_ALG_SHA_512(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_SHA_512) && _alg == PSA_ALG_SHA_512)

/* Macros to verify different encryption algorithms */

#define VERIFY_ALG_CTR(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_CTR) && _alg == PSA_ALG_CTR)

/* Macros to verify different key derivation algorithms */

#define VERIFY_ALG_HKDF(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_HKDF) && PSA_ALG_IS_HKDF(_alg))

/* Macros to verify different key agreement algorithms */

#define VERIFY_ALG_ECDH(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_ECDH) && _alg == PSA_ALG_ECDH)

/* Macros to verify different MAC algorithms */

#define VERIFY_ALG_HMAC(_alg) \
	(IS_ENABLED(PSA_WANT_ALG_HMAC) && _alg == PSA_ALG_HMAC(PSA_ALG_SHA_512))

/* Macros to verify configuration */

#if CONFIG_PSA_CORE_LITE_PUB_KEY_MAX_SIZE == 0 && \
    defined(CONFIG_PSA_CORE_LITE_HAS_VERIFY_SIGNATURE)
#error "No valid algorithm for signature validation"
#endif

#if CONFIG_PSA_CORE_LITE_AES_KEY_MAX_SIZE == 0 && \
    defined(PSA_WANT_ALG_CTR)
#error "FW encryption requires either AES-256 or AES-128 being enabled"
#endif

#if CONFIG_PSA_CORE_LITE_PRIV_KEY_MAX_SIZE == 0 && \
    defined(PSA_WANT_ALG_ECDH)
#error "No valid algorithm for key agreement"
#endif

#define PSA_CORE_LITE_KEY_MAX_SIZE	MAX(CONFIG_PSA_CORE_LITE_AES_KEY_MAX_SIZE,   \
					MAX(CONFIG_PSA_CORE_LITE_PUB_KEY_MAX_SIZE,   \
					    CONFIG_PSA_CORE_LITE_PRIV_KEY_MAX_SIZE))

typedef struct {
	psa_key_attributes_t key_attributes;
	uint8_t key[PSA_CORE_LITE_KEY_MAX_SIZE];
	size_t key_size;
} psa_core_lite_key_slot_t;

#endif /* PSA_CORE_LITE_H_ */
