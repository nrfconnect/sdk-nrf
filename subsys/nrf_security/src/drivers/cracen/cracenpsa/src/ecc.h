/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/crypto.h>

int ecc_create_genpubkey(const char *priv_key, char *pub_key, const struct sx_pk_ecurve *curve);

int ecc_create_genprivkey(const struct sx_pk_ecurve *curve, char *priv_key, size_t priv_key_size);
