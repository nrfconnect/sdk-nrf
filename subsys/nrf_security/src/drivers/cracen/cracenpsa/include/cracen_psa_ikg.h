/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_IKG_H
#define CRACEN_PSA_IKG_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

int cracen_ikg_sign_message(int identity_key_index, const struct sxhashalg *hashalg,
			    const struct sx_pk_ecurve *curve, const uint8_t *message,
			    size_t message_length, uint8_t *signature);

int cracen_ikg_sign_digest(int identity_key_index, const struct sxhashalg *hashalg,
			   const struct sx_pk_ecurve *curve, const uint8_t *digest,
			   size_t digest_length, uint8_t *signature);

int cracen_ikg_create_pub_key(int identity_key_index, uint8_t *pub_key);

#endif /* CRACEN_PSA_IKG_H */
