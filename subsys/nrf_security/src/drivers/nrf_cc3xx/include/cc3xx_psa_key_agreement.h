/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CC3XX_PSA_KEY_AGREEMENT_H
#define CC3XX_PSA_KEY_AGREEMENT_H

/** \file cc3xx_psa_key_agreement.h
 *
 * This file contains the declaration of the entry points associated to the
 * raw key agreement (i.e. ECDH) as described by the PSA * Cryptoprocessor
 * Driver interface specification
 *
 */

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Performs a raw key agreement (i.e. Elliptic Curve Diffie Hellman)
 *
 * \param[in]  attributes      Attributes of the key to use
 * \param[in]  priv_key        Private key of the initiating party
 * \param[in]  priv_key_size   Size in bytes of priv_key
 * \param[in]  publ_key        Public key of the peer party
 * \param[in]  publ_key_size   Size in bytes of publ_key
 * \param[out] output          Buffer to hold the agreed key
 * \param[in]  output_size     Size in bytes of the output buffer
 * \param[out] output_length   Size in bytes of the agreed key
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_key_agreement(
        const psa_key_attributes_t *attributes,
        const uint8_t *priv_key, size_t priv_key_size,
        const uint8_t *publ_key, size_t publ_key_size,
        uint8_t *output, size_t output_size, size_t *output_length,
        psa_algorithm_t alg);

#ifdef __cplusplus
}
#endif
#endif /* CC3XX_PSA_KEY_AGREEMENT_H */
