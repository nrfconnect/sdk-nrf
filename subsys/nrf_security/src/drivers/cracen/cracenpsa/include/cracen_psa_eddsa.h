/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_EDDSA_H
#define CRACEN_PSA_EDDSA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

int cracen_ed25519_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

int cracen_ed25519_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

int cracen_ed25519ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

int cracen_ed25519ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

int cracen_ed25519_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

int cracen_ed448_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

int cracen_ed448_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

int cracen_ed448ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

int cracen_ed448ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

int cracen_ed448_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

#endif /* CRACEN_PSA_EDDSA_H */
