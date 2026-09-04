/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <psa/psa_ext_ecc.h>

#include "vectors.h"

#define SCALAR_SIZE PSA_EXT_ECC_SECP160R1_SCALAR_SIZE
#define COORD_SIZE  PSA_EXT_ECC_SECP160R1_COORD_SIZE

/* Reduce, then multiply the base point, and check both results. This is the
 * sequence Google Find Hub Network uses to derive an Ephemeral ID.
 */
static void check_vector(const char *name, const uint8_t *input, size_t input_length,
			 const uint8_t *expected_scalar, const uint8_t *expected_x)
{
	uint8_t scalar[SCALAR_SIZE];
	uint8_t x[COORD_SIZE];

	zassert_equal(PSA_SUCCESS,
		      psa_ext_ecc_secp160r1_scalar_reduce(input, input_length, scalar,
							  sizeof(scalar)),
		      "%s: reduce failed", name);
	zassert_mem_equal(scalar, expected_scalar, SCALAR_SIZE,
			  "%s: wrong reduction modulo the group order", name);

	zassert_equal(PSA_SUCCESS,
		      psa_ext_ecc_secp160r1_scalar_mult_base(scalar, sizeof(scalar), x,
							     sizeof(x)),
		      "%s: scalar multiplication failed", name);
	zassert_mem_equal(x, expected_x, COORD_SIZE, "%s: wrong x-coordinate", name);
}

#define CHECK(name) \
	check_vector(#name, input_##name, sizeof(input_##name), scalar_##name, x_##name)

ZTEST(ecc_secp160r1, test_base_point)
{
	CHECK(one);
}

ZTEST(ecc_secp160r1, test_doubling_and_addition)
{
	CHECK(two);
	CHECK(seven);
}

/* The scalar here is 161 bits wide, because the secp160r1 group order is wider
 * than the field. Truncating it to the 160-bit curve size yields a different
 * point, so this vector is what proves the operand size is 21 bytes and not 20.
 */
ZTEST(ecc_secp160r1, test_161_bit_scalar)
{
	CHECK(order_minus_one);
}

ZTEST(ecc_secp160r1, test_reduction_wraps_at_the_order)
{
	CHECK(order_plus_one);

	/* And the two must agree, since (n + 1) mod n == 1 mod n. */
	zassert_mem_equal(x_order_plus_one, x_one, COORD_SIZE,
			  "vectors disagree, so one of them is wrong");
}

ZTEST(ecc_secp160r1, test_scalar_zero_is_rejected)
{
	uint8_t scalar[SCALAR_SIZE];
	uint8_t x[COORD_SIZE];
	static const uint8_t order[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf4, 0xc8,
		0xf9, 0x27, 0xae, 0xd3, 0xca, 0x75, 0x22, 0x57,
	};
	uint8_t zeros[SCALAR_SIZE] = {0};

	/* n mod n is zero, and 0 * G is the point at infinity, which has no
	 * affine x-coordinate. It must be reported, not returned as an
	 * all-zero point.
	 */
	zassert_equal(PSA_SUCCESS,
		      psa_ext_ecc_secp160r1_scalar_reduce(order, sizeof(order), scalar,
							  sizeof(scalar)),
		      "reduce failed");
	zassert_mem_equal(scalar, zeros, SCALAR_SIZE, "n mod n is not zero");

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT,
		      psa_ext_ecc_secp160r1_scalar_mult_base(scalar, sizeof(scalar), x,
							     sizeof(x)),
		      "a zero scalar was accepted");
}

ZTEST(ecc_secp160r1, test_argument_validation)
{
	uint8_t scalar[SCALAR_SIZE];
	uint8_t x[COORD_SIZE];

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT,
		      psa_ext_ecc_secp160r1_scalar_reduce(input_one, 0, scalar, sizeof(scalar)),
		      "empty input was accepted");
	zassert_equal(PSA_ERROR_INVALID_ARGUMENT,
		      psa_ext_ecc_secp160r1_scalar_reduce(
			      input_one, PSA_EXT_ECC_SECP160R1_MAX_INPUT_SIZE + 1, scalar,
			      sizeof(scalar)),
		      "oversized input was accepted");
	zassert_equal(PSA_ERROR_BUFFER_TOO_SMALL,
		      psa_ext_ecc_secp160r1_scalar_reduce(input_one, sizeof(input_one), scalar,
							  SCALAR_SIZE - 1),
		      "undersized reduction output was accepted");

	zassert_equal(PSA_ERROR_INVALID_ARGUMENT,
		      psa_ext_ecc_secp160r1_scalar_mult_base(scalar_one, 0, x, sizeof(x)),
		      "empty scalar was accepted");
	zassert_equal(PSA_ERROR_INVALID_ARGUMENT,
		      psa_ext_ecc_secp160r1_scalar_mult_base(scalar_one, SCALAR_SIZE + 1, x,
							     sizeof(x)),
		      "oversized scalar was accepted");
	zassert_equal(PSA_ERROR_BUFFER_TOO_SMALL,
		      psa_ext_ecc_secp160r1_scalar_mult_base(scalar_one, SCALAR_SIZE, x,
							     COORD_SIZE - 1),
		      "undersized coordinate output was accepted");
}

/* A shorter scalar must be accepted and right-aligned, not misread. */
ZTEST(ecc_secp160r1, test_short_scalar)
{
	static const uint8_t two[] = {0x02};
	uint8_t x[COORD_SIZE];

	zassert_equal(PSA_SUCCESS,
		      psa_ext_ecc_secp160r1_scalar_mult_base(two, sizeof(two), x, sizeof(x)),
		      "single-byte scalar failed");
	zassert_mem_equal(x, x_two, COORD_SIZE, "single-byte scalar gave the wrong point");
}

ZTEST_SUITE(ecc_secp160r1, NULL, NULL, NULL, NULL, NULL);
