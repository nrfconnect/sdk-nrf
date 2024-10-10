/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* A minimalistic ASN.1 BER/DER decoder (X.690).
 *
 * Supported types:
 *   OCTET STRING
 *   SEQUENCE / SEQUENCE OF
 */

#ifndef ASN1_DECODE_H_
#define ASN1_DECODE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Commonly used ASN.1 tags. */
#define UP4  0x04
#define UP6  0x06
#define UC16 0x30
#define AP15 0x4F
#define AP16 0x50
#define CC1  0xA1
#define CC7  0xA7

/** ASN.1 context values */
typedef struct {
	const uint8_t *asnbuf;	/**< ASN.1 BER/DER encoded buffer */
	size_t length;		/**< Length of the data buffer */
	uint32_t offset;	/**< Current offset into asnbuf */
	bool error;		/**< Error detected in ASN.1 syntax */
} asn1_ctx_t;

/** Function called to handle elements in a SEQUENCE */
typedef void (*asn1_sequence_func_t)(asn1_ctx_t *ctx, void *data);

/**
 * @brief Decode ASN.1 header.
 *
 * @param[in]  ctx ASN.1 context values.
 * @param[out] tag ASN.1 TAG.
 * @param[out] len Length of ASN.1 value.
 *
 * @return true when valid ASN.1 and a value exists
 */
bool asn1_dec_head(asn1_ctx_t *ctx, uint8_t *tag, size_t *len);

/**
 * @brief Decode ASN.1 OCTET STRING.
 *
 * @param[in]  ctx     ASN.1 context values.
 * @param[in]  len     Length of octet string to decode.
 * @param[out] value   Decoded octet string.
 * @param[in]  max_len Maximum length of octet string to decode.
 */
void asn1_dec_octet_string(asn1_ctx_t *ctx, size_t len, uint8_t *value, size_t max_len);

/**
 * @brief Decode ASN.1 SEQUENCE.
 *
 * @param[in]    ctx           ASN.1 context values.
 * @param[in]    len           Length of sequence to decode.
 * @param[inout] data          Pointer to application specific values.
 * @param[in]    sequence_func Function to be called to handle sequence elements.
 */
void asn1_dec_sequence(asn1_ctx_t *ctx, size_t len, void *data, asn1_sequence_func_t sequence_func);

/**
 * @brief Skip a subset of ASN.1 content.
 *        This is used to skip parts of the content which is not of interest.
 *
 * @param[in] ctx ASN.1 context values.
 * @param[in] len Length of data to skip.
 */
void asn1_dec_skip(asn1_ctx_t *ctx, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ASN1_DECODE_H_ */
