/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef ECC_H
#define ECC_H

#include <psa/crypto.h>
#include <stdint.h>

int ecc_genpubkey(const uint8_t *priv_key, uint8_t *pub_key, const struct sx_pk_ecurve *curve);

int ecc_genprivkey(const struct sx_pk_ecurve *curve, uint8_t *priv_key, size_t priv_key_size);

#endif /* ECC_H */
