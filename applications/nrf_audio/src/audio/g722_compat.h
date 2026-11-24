/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef G722_COMPAT_H_
#define G722_COMPAT_H_

#include <stdint.h>

#if defined(__has_include)
#if __has_include("g722_encoder.h") && __has_include("g722_decoder.h")
#include "g722_encoder.h"
#include "g722_decoder.h"
#else
/* Compile-only fallback used when the private libg722 project is not present. */
typedef void G722_ENC_CTX;
typedef void G722_DEC_CTX;

enum {
	G722_DEFAULT = 0x0000,
	G722_SAMPLE_RATE_8000 = 0x0001,
	G722_PACKED = 0x0002,
};

G722_ENC_CTX *g722_encoder_new(int rate, int options);
int g722_encoder_destroy(G722_ENC_CTX *ctx);
int g722_encode(G722_ENC_CTX *ctx, const int16_t samples[], int num_samples, uint8_t encoded[]);

G722_DEC_CTX *g722_decoder_new(int rate, int options);
int g722_decoder_destroy(G722_DEC_CTX *ctx);
int g722_decode(G722_DEC_CTX *ctx, const uint8_t encoded[], int encoded_size, int16_t samples[]);
#endif /* __has_include("g722_encoder.h") && __has_include("g722_decoder.h") */
#else
#include "g722_encoder.h"
#include "g722_decoder.h"
#endif /* defined(__has_include) */

#endif /* G722_COMPAT_H_ */
