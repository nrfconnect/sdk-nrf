/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa/crypto.h"
#include "oberon_asymmetric_signature.h"

#ifdef PSA_NEED_OBERON_ECDSA_SIGN
#include "oberon_ecdsa.h"
#endif
#ifdef PSA_NEED_OBERON_RSA_ANY_SIGN
#include "oberon_rsa.h"
#endif

psa_status_t oberon_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key,
			      size_t key_length, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_ECDSA_SIGN
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_ecdsa_sign_hash(attributes, key, key_length, alg, hash, hash_length,
					      signature, signature_size, signature_length);
	} else
#endif /* PSA_NEED_OBERON_ECDSA_SIGN */

#ifdef PSA_NEED_OBERON_RSA_ANY_SIGN
		if (PSA_KEY_TYPE_IS_RSA(type)) {
		return oberon_rsa_sign_hash(attributes, key, key_length, alg, hash, hash_length,
					    signature, signature_size, signature_length);
	} else
#endif /* PSA_NEED_OBERON_RSA_ANY_SIGN */

	{
		(void)key;
		(void)key_length;
		(void)alg;
		(void)hash;
		(void)hash_length;
		(void)signature;
		(void)signature_size;
		(void)signature_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key,
				 size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_ECDSA_SIGN
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_ecdsa_sign_message(attributes, key, key_length, alg, input,
						 input_length, signature, signature_size,
						 signature_length);
	} else
#endif /* PSA_NEED_OBERON_ECDSA_SIGN */

	{
		(void)key;
		(void)key_length;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)signature;
		(void)signature_size;
		(void)signature_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key,
				size_t key_length, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_ECDSA_SIGN
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_ecdsa_verify_hash(attributes, key, key_length, alg, hash, hash_length,
						signature, signature_length);
	} else
#endif /* PSA_NEED_OBERON_ECDSA_SIGN */

#ifdef PSA_NEED_OBERON_RSA_ANY_SIGN
		if (PSA_KEY_TYPE_IS_RSA(type)) {
		return oberon_rsa_verify_hash(attributes, key, key_length, alg, hash, hash_length,
					      signature, signature_length);
	} else
#endif /* PSA_NEED_OBERON_RSA_ANY_SIGN */

	{
		(void)key;
		(void)key_length;
		(void)alg;
		(void)hash;
		(void)hash_length;
		(void)signature;
		(void)signature_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t oberon_verify_message(const psa_key_attributes_t *attributes, const uint8_t *key,
				   size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				   size_t input_length, const uint8_t *signature,
				   size_t signature_length)
{
	psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_ECDSA_SIGN
	if (PSA_KEY_TYPE_IS_ECC(type)) {
		return oberon_ecdsa_verify_message(attributes, key, key_length, alg, input,
						   input_length, signature, signature_length);
	} else
#endif /* PSA_NEED_OBERON_ECDSA_SIGN */

	{
		(void)key;
		(void)key_length;
		(void)alg;
		(void)input;
		(void)input_length;
		(void)signature;
		(void)signature_length;
		(void)type;
		return PSA_ERROR_NOT_SUPPORTED;
	}
}
