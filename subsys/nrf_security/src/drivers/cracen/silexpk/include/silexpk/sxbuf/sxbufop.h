/** Basic "sxops" operand definition
 *
 * @file
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ADAPTER_TYPES_HEADER
#define ADAPTER_TYPES_HEADER

#include <stddef.h>
#include <stdint.h>

/** Basic operand representation **/
struct sx_buf {
	/** Size in bytes of operand **/
	size_t sz;
	/** Memory of operand bytes in big endian **/
	uint8_t *bytes;
};

struct sx_const_buf {
	/** Size in bytes of operand **/
	size_t sz;
	/** Memory of operand bytes in big endian **/
	const uint8_t *bytes;
};
/** Simple "sxops" implementation based on sx_buf and sx_const_buf**/
typedef struct sx_buf sx_op;
typedef struct sx_const_buf sx_const_op;

/** Simple Elliptic Curve operand implementation based on sx_buf**/
typedef struct sx_buf sx_ecop;
typedef struct sx_const_buf sx_const_ecop;

/** Affine point for ECC **/
typedef struct sx_pk_affine_point {
	/** x-coordinate of affine point */
	sx_ecop x;
	/** y-coordinate of affine point */
	sx_ecop y;
} sx_pk_affine_point;

typedef struct sx_pk_const_affine_point {
	/** x-coordinate of const affine point */
	sx_const_ecop x;
	/** y-coordinate of const affine point */
	sx_const_ecop y;
} sx_pk_const_affine_point;

/** Initializes const_sx_op with values from provided sx_op
 *
 * @param[in] source_op Source sx operand
 * @param[out] result_op Result operand initialized from source
 */
static inline void sx_get_const_op(const sx_op *source_op, sx_const_op *result_op)
{
	result_op->sz = source_op->sz;
	result_op->bytes = source_op->bytes;
}

/** Initializes sx_pk_const_affine_point with values from provided sx_pk_affine_point
 *
 * @param[in] source_point Source affine point
 * @param[out] result_point Result affine point initialized from source
 */
static inline void sx_get_const_affine_point(const sx_pk_affine_point *source_point,
					     sx_pk_const_affine_point *result_point)
{
	result_point->x.sz = source_point->x.sz;
	result_point->x.bytes = source_point->x.bytes;

	result_point->y.sz = source_point->y.sz;
	result_point->y.bytes = source_point->y.bytes;
}

#endif
