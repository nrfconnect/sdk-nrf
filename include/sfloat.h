/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SFLOAT_H_
#define SFLOAT_H_

#include <stdint.h>
#include <zephyr/types.h>

/**
 * @file
 * @defgroup sfloat Short float (SFLOAT) type.
 * @{
 *
 * @brief API for short float (SFLOAT) type from the IEEE 11073-20601-2008
 *        specification.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SFLOAT special values. */
enum sfloat_special {
	/** Not a number. */
	SFLOAT_NAN = 0x07FF,

	/** Not at this resolution. */
	SFLOAT_NRES = 0x0800,

	/** Positive infinity. */
	SFLOAT_POS_INFINITY = 0x07FE,

	/** Negative infinity. */
	SFLOAT_NEG_INFINITY = 0x0802,

	/** Reserved for future use. */
	SFLOAT_RESERVED = 0x0801,
};

/** @brief SFLOAT type. */
struct sfloat {
	/* SFLOAT type encoded as 16-bit word that consists of the following fields:
	 * 1. Exponent, encoded as 4-bit integer in two's-complement form.
	 * 2. Mantissa, encoded as 12-bit integer in two's-complement form.
	 */
	uint16_t val;
};

/**
 * @brief Convert the standard float type into SFLOAT type.
 *
 * @param[in] float_num Number encoded as a standard float type
 *                      (binary32 notation from the IEEE 754-2008 specification).
 *
 * @return SFLOAT type (from the IEEE 11073-20601-2008 specification).
 */
struct sfloat sfloat_from_float(float float_num);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SFLOAT_H_ */
