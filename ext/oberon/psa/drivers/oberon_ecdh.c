/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include "psa/crypto.h"
#include "oberon_ecdh.h"

#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_224
#include "ocrypto_ecdh_p224.h"
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_256
#include "ocrypto_ecdh_p256.h"
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_384
#include "ocrypto_ecdh_p384.h"
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_ECDH_MONTGOMERY_255
#include "ocrypto_curve25519.h"
#endif /* PSA_NEED_OBERON_ECDH_MONTGOMERY_255 */

psa_status_t oberon_ecdh(const psa_key_attributes_t *attributes, const uint8_t *key,
			 size_t key_length, psa_algorithm_t alg, const uint8_t *peer_key,
			 size_t peer_key_length, uint8_t *output, size_t output_size,
			 size_t *output_length)
{
    int res = 0;
    size_t bits = psa_get_key_bits(attributes);
    if (alg != PSA_ALG_ECDH) return PSA_ERROR_NOT_SUPPORTED;
    if (key_length != PSA_BITS_TO_BYTES(bits)) return PSA_ERROR_INVALID_ARGUMENT;
    if (output_size < key_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    *output_length = key_length;

    switch (psa_get_key_type(attributes)) {
#if defined(PSA_NEED_OBERON_ECDH_SECP_R1_224) || defined(PSA_NEED_OBERON_ECDH_SECP_R1_256) ||      \
	defined(PSA_NEED_OBERON_ECDH_SECP_R1_384)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (peer_key_length != key_length * 2 + 1) return PSA_ERROR_INVALID_ARGUMENT;
        if (peer_key[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
        switch (bits) {
#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_224
	case 224:
		res = ocrypto_ecdh_p224_common_secret(output, key, &peer_key[1]);
		break;
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_256
	case 256:
		res = ocrypto_ecdh_p256_common_secret(output, key, &peer_key[1]);
		break;
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_ECDH_SECP_R1_384
	case 384:
		res = ocrypto_ecdh_p384_common_secret(output, key, &peer_key[1]);
		break;
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_384 */
	default:
		return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_ARGUMENT;
        break;
#endif /* PSA_NEED_OBERON_ECDH_SECP_R1_224 || PSA_NEED_OBERON_ECDH_SECP_R1_256 ||                  \
	  PSA_NEED_OBERON_ECDH_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_ECDH_MONTGOMERY_255
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
        if (bits != 255) return PSA_ERROR_NOT_SUPPORTED;
        if (peer_key_length != key_length) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_curve25519_scalarmult(output, key, peer_key);
        break;
#endif /* PSA_NEED_OBERON_ECDH_MONTGOMERY_255 */
    default:
        (void)key;
        (void)key_length;
        (void)peer_key;
        (void)peer_key_length;
        (void)output;
	(void)res;
	return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}
