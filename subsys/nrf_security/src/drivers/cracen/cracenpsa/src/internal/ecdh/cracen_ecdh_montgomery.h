/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_ECDH_MONTGOMERY_H
#define CRACEN_ECDH_MONTGOMERY_H

#include <stdint.h>
#include <psa/crypto.h>
#include <silexpk/core.h>

psa_status_t cracen_ecdh_montgmr_calc_secret(const struct sx_pk_ecurve *curve,
					     const uint8_t *priv_key, size_t priv_key_size,
					     const uint8_t *publ_key, size_t publ_key_size,
					     uint8_t *output, size_t output_size,
					     size_t *output_length);

#endif /* CRACEN_ECDH_MONTGOMERY_H */
