/** Predefined and custom elliptic curve definitions
 *
 * @file
 */
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EC_CURVES_HEADER_FILE
#define EC_CURVES_HEADER_FILE

#include <stdint.h>
#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup SX_PK_CURVES
 *
 * @{
 */

/** Declarations for all the defined EC curves. */
extern const struct sx_pk_ecurve sx_curve_brainpoolP192r1;
extern const struct sx_pk_ecurve sx_curve_brainpoolP224r1;
extern const struct sx_pk_ecurve sx_curve_brainpoolP256r1;
extern const struct sx_pk_ecurve sx_curve_brainpoolP320r1;
extern const struct sx_pk_ecurve sx_curve_brainpoolP384r1;
extern const struct sx_pk_ecurve sx_curve_brainpoolP512r1;
extern const struct sx_pk_ecurve sx_curve_ed25519;
extern const struct sx_pk_ecurve sx_curve_ed448;
extern const struct sx_pk_ecurve sx_curve_nistp192;
extern const struct sx_pk_ecurve sx_curve_nistp224;
extern const struct sx_pk_ecurve sx_curve_nistp256;
extern const struct sx_pk_ecurve sx_curve_nistp384;
extern const struct sx_pk_ecurve sx_curve_nistp521;
extern const struct sx_pk_ecurve sx_curve_secp192k1;
extern const struct sx_pk_ecurve sx_curve_secp256k1;
extern const struct sx_pk_ecurve sx_curve_x25519;
extern const struct sx_pk_ecurve sx_curve_x448;

/** Write the generator point of the curve into the slots (internal)
 *
 * Write the parameter gx & gy from curve to px.addr & py.addr respectively
 *
 * @param[in] pk The accelerator request
 * @param[in] curve Initialised curve to get generator point from.
 * @param[in] px x-coordinate slot of generator point. The curve generator
 * (x-coordinate) will be written to this address
 * @param[in] py y-coordinate slot of generator point. The curve generator
 * (y-coordinate) will be written to this address
 */
void sx_pk_write_curve_gen(sx_pk_req *pk, const struct sx_pk_ecurve *curve, struct sx_pk_slot px,
			   struct sx_pk_slot py);

/** Return the operand size in bytes for the given curve
 *
 * @param[in] curve Initialised curve to get operand size from
 * @return Operand size in bytes for the given curve
 */
static inline int sx_pk_curve_opsize(const struct sx_pk_ecurve *curve)
{
	return curve->sz;
}

/** Return a pointer to the field size of the given curve
 *
 * @param[in] curve Initialised curve
 * @return Pointer to the field size of the given curve
 */
static inline const char *sx_pk_field_size(const struct sx_pk_ecurve *curve)
{
	return curve->params;
}

/** Return a pointer to the order of the given curve
 *
 * @param[in] curve Initialised curve
 * @return Pointer to the order of the given curve
 */
static inline const char *sx_pk_curve_order(const struct sx_pk_ecurve *curve)
{
	return &curve->params[curve->sz];
}

/**
 * Return a pointer to the generator point of the given curve.
 *
 * @param[in] curve Initialised curve
 *
 * @return Pointer to the generator point of the given curve.
 */
static inline const char *sx_pk_generator_point(const struct sx_pk_ecurve *curve)
{
	return &curve->params[curve->sz * 2];
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
