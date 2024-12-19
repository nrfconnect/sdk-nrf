/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>

int cracen_ed25519_sign(const uint8_t *priv_key, char *signature, const uint8_t *message,
			size_t message_length);

int cracen_ed25519_verify(const uint8_t *pub_key, const char *message, size_t message_length,
			  const char *signature);

int cracen_ed25519ph_sign(const uint8_t *priv_key, char *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

int cracen_ed25519ph_verify(const uint8_t *pub_key, const char *message, size_t message_length,
			    const char *signature, bool is_message);

int cracen_ed25519_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);
