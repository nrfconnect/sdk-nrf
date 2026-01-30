/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "cracen_psa.h"
#include "sxsymcrypt/internal.h"

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/hashdefs.h>
#include <silexpk/core.h>

#define CRACEN_PSA_IS_KEY_FLAG(flag, attr) ((flag) == (psa_get_key_usage_flags((attr)) & (flag)))
#define CRACEN_PSA_IS_KEY_TYPE(flag, attr) ((flag) == (psa_get_key_type((attr)) & (flag)))

typedef struct {
	uint8_t slot_number;
	uint8_t owner_id;
} ikg_opaque_key;

__must_check psa_status_t silex_statuscodes_to_psa(int sx_status);

__must_check psa_status_t
hash_get_algo(psa_algorithm_t alg, const struct sxhashalg **sx_hash_algo);

/**
 * @brief Use cracen_get_random up to generate a random number in the range [1, upperlimit).
 *
 * @param[out] n           Output number.
 * @param[in]  sz          Size of number in bytes.
 * @param[in]  upperlimit  Upper limit for generated number.
 * @param[in]  retrylimit  Maximum number of attempts to generate a number in the range.
 *
 * @note Output number and upper limit must be big endian numbers of size @ref sz.
 *
 * @return PSA status code.
 */
psa_status_t cracen_rnd_in_range(uint8_t *n, size_t sz, const uint8_t *upperlimit,
				 size_t retrylimit);

/**
 * @brief XOR two byte buffers and store result in first buffer
 *
 * @param[in,out] a First buffer of size sz
 * @param[in] b Second buffer of size sz
 * @param[in] sz Size of the buffers
 */
void cracen_xorbytes(uint8_t *a, const uint8_t *b, size_t sz);

/**
 * @brief Loads key buffer and attributes.
 *
 * @return PSA status code.
 */
psa_status_t cracen_load_keyref(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, struct sxkeyref *k);

/**
 * @brief Prepare ik key.
 *
 * @param user_data    Owner ID.
 *
 * @return sxsymcrypt status code.
 */
int cracen_prepare_ik_key(const uint8_t *user_data);

/**
 * @brief Compute v = v + summand.
 * \note  v is an unsigned integer stored as a big endian byte array of sz bytes.
 *        Summand must be less than or equal to the maximum value of a size_t minus 255.
 *        The final carry is discarded: addition is modulo 2^(sz*8).
 *
 * @param v		Big-endian Value
 * @param v_size	size of v
 * @param summand	Summand.
 *
 */
void cracen_be_add(uint8_t *v, size_t v_size, size_t summand);

/**
 * @brief Compute c = a - b.
 *
 * @param a	Big-endian value a.
 * @param b	Big-endian value b.
 * @param c	The substraction result.
 * @param sz	Size of the buffers.
 *
 * @retval 0 Success.
 * @retval -1 For a < b.
 */
int cracen_be_sub(const uint8_t *a, const uint8_t *b, uint8_t *c, size_t sz);

/**
 * @brief Compute r = a >> n.
 *
 * @param a	Big-endian value a.
 * @param n	Number of bits.
 * @param c	Bitshift result.
 * @param sz	Size of the buffers.
 *
 */
void cracen_be_rshift(const uint8_t *a, int n, uint8_t *r, size_t sz);

/**
 * @brief Big-Endian compare with carry.
 * \note
 *
 * @param a		First value to be compared
 * @param b		Second value to be compared
 * @param size		Size of a and b.
 * @param carry		Value of the carry.
 *
 * \retval 0 if equal.
 * \retval 1 if a > b.
 * \retval -1 if a < b.
 */
int cracen_be_cmp(const uint8_t *a, const uint8_t *b, size_t sz, int carry);

/**
 * @brief Compute buf = buf ^ xor_val.
 *
 * @param buf		Big-endian value.
 * @param buf_sz	size of buf.
 * @param xor_val	A value to XOR buf with.
 */
void cracen_be_xor(uint8_t *buf, size_t buf_sz, size_t xor_val);

/**
 * @brief Hash several elements at different locations in memory
 *
 * @param inputs[in]		Array of pointers to elements that will be hashed.
 * @param input_lengths[in]	Array of lengths of elements to be hashed.
 * @param input_count[in]	Number of elements to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_all_inputs(const uint8_t *inputs[], const size_t input_lengths[],
			   size_t input_count, const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash several elements at different locations in memory with a previously created hash
 * context(sxhash)
 *
 * @param sxhashopctx[in]	Pointer to the sxhash context.
 * @param inputs[in]		Array of pointers to elements that will be hashed.
 * @param input_lengths[in]	Array of lengths of elements to be hashed.
 * @param input_count[in]	Number of elements to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_all_inputs_with_context(struct sxhash *sxhashopctx, const uint8_t *inputs[],
					const size_t input_lengths[], size_t input_count,
					const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash a single element
 *
 * @param input[in]		Pointer to the element that will be hashed.
 * @param input_length[in]	Length of the element to be hashed.
 * @param hashalg[in]		Hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_input(const uint8_t *input, const size_t input_length,
		      const struct sxhashalg *hashalg, uint8_t *digest);

/**
 * @brief Hash a single element with a previously created hash context(sxhash)
 *
 * @param sxhashopctx[in]	Pointer to the sxhash context.
 * @param input[in]		Pointer to element that will be hashed.
 * @param input_length[in]	Length of element to be hashed.
 * @param hashalg[in]		hash algorithm to be used in sxhashalg format.
 * @param digest[out]		Buffer of at least sx_hash_get_alg_digestsz(hashalg) bytes.
 *
 * @return sxsymcrypt status code.
 */
int cracen_hash_input_with_context(struct sxhash *hashopctx, const uint8_t *input,
				   const size_t input_length, const struct sxhashalg *hashalg,
				   uint8_t *digest);

/**
 * @brief Generate a random number within the specified range.
 *
 * \note  This function generates a random number strictly less than the given upper limit (`n`).
 *        The generated random number is evenly distributed over the range [0, n-1]. If the range
 *        is invalid (e.g., `n` is zero, less than three, or even), an error code is returned.
 *
 * @param n[in]         Pointer to a buffer holding the upper limit of the random range.
 *                      The upper limit must be a non-zero odd number.
 * @param nsz[in]       Size of the upper limit buffer in bytes.
 * @param out[out]      Buffer to store the generated random number.
 *                      The size of `out` should be at least `nsz`.
 *
 * @return sxsymcrypt status code.
 */
int cracen_get_rnd_in_range(const uint8_t *n, size_t nsz, uint8_t *out);
