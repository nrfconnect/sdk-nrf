/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_MONTGOMERY_H
#define CRACEN_PSA_MONTGOMERY_H

#include <stdint.h>

int cracen_x448_genpubkey(const uint8_t *priv_key, uint8_t *pub_key);

int cracen_x25519_genpubkey(const uint8_t *priv_key, uint8_t *pub_key);

#endif /* CRACEN_PSA_MONTGOMERY_H */
