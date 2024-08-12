/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* A minimalistic PKCS #15 Elementary File (EF) decoder.
 *
 * Supported objects:
 *   EF(ODF) (Object Directory File) - Path element
 *   EF(DODF) (Data Object Directory File) - Path element
 */

#ifndef PKCS15_DECODE_H_
#define PKCS15_DECODE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum Path size supported in this decoder. */
#define PATH_SIZE 16

/** PKCS#15 object with values of interest. */
typedef struct {
	uint8_t path[PATH_SIZE];
} pkcs15_object_t;

/**
 * @brief Decode PKCS #15 EF(ODF) Path.
 *
 * @param[in]  bytes  Array of bytes containing ASN.1.
 * @param[in]  len    Length of bytes array.
 * @param[out] object PKCS #15 object with decoded values.
 *
 * @return true on success.
 */
bool pkcs15_ef_odf_path_decode(const uint8_t *bytes, size_t len, pkcs15_object_t *object);

/**
 * @brief Decode PKCS #15 EF(DODF) Path.
 *
 * @param[in]  bytes  Array of bytes containing ASN.1.
 * @param[in]  len    Length of bytes array.
 * @param[out] object PKCS #15 object with decoded values.
 *
 * @return true on success.
 */
bool pkcs15_ef_dodf_path_decode(const uint8_t *bytes, size_t len, pkcs15_object_t *object);

#ifdef __cplusplus
}
#endif

#endif /* PKCS15_DECODE_H_ */
