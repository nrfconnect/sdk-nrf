/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef TMP_RNG_H
#define TMP_RNG_H

#include <psa/crypto.h>

psa_status_t tmp_rng(uint8_t *out, size_t outsize);

#endif
