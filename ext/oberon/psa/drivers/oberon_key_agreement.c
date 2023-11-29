/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include "psa/crypto.h"
#include "oberon_key_agreement.h"

#ifdef PSA_NEED_OBERON_ECDH
#include "oberon_ecdh.h"
#endif /* PSA_NEED_OBERON_ECDH */


psa_status_t oberon_key_agreement(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *peer_key, size_t peer_key_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_ECDH
    if (PSA_KEY_TYPE_IS_ECC(type)) {
        return oberon_ecdh(
            attributes, key, key_length,
            alg,
            peer_key, peer_key_length,
            output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_ECDH */

    {
        (void)type;
        (void)key;
        (void)key_length;
        (void)alg;
        (void)peer_key;
        (void)peer_key_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}
