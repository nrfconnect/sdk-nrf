/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <stdint.h>

/**
 * @brief Decode a byte array containing a coordinate in place.
 *
 * @note Corresponds to the decodeUCoordinate function in RFC 7748 for Curve25519.
 *
 * @param[in,out] u Byte array containing a coordinate of size 32 bytes.
 */
void decode_u_coordinate_25519(uint8_t *u);

/**
 * @brief Decode a byte array containing a scalar in place.
 *
 * @note Corresponds to the decodeScalar25519 function in RFC 7748.
 *
 * @param[in,out] k Byte array containing a scalar of size 32.
 */
void decode_scalar_25519(uint8_t *k);

/**
 * @brief Decode a byte array containing a scalar in place.
 *
 * @note Corresponds to the decodeScalar448 function in RFC 7748.
 *
 * @param[in,out] k Byte array containing a scalar of size 56.
 */
void decode_scalar_448(uint8_t *k);
