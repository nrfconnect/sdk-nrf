/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wave generator header.
 */

#ifndef _WAVE_GEN_H
#define _WAVE_GEN_H

/**
 * @defgroup wave_gen Wave generator
 * @{
 * @brief Library for generating wave signals.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/** @brief Available generated wave types.
 */
enum wave_gen_type {
	WAVE_GEN_TYPE_SINE,
	WAVE_GEN_TYPE_TRIANGLE,
	WAVE_GEN_TYPE_SQUARE,
	WAVE_GEN_TYPE_NONE,

	WAVE_GEN_TYPE_COUNT,
};

/** @brief Generated wave parameters.
 */
struct wave_gen_param {
	/** Type of the wave signal. */
	enum wave_gen_type type;

	/** Period of the wave signal [ms]. */
	uint32_t period_ms;

	/** Offset of the wave signal. */
	double offset;

	/** Amplitude of the wave signal. */
	double amplitude;

	/** Amplitude of the added noise signal. */
	double noise;
};

/**
 * @brief Generate wave value.
 *
 * @param[in]	time	Time for generated value.
 * @param[in]	params	Parameters describing generated wave signal.
 * @param[out]	out_val	Pointer to the variable that is used to store generated value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int wave_gen_generate_value(uint32_t time, const struct wave_gen_param *params, double *out_val);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _WAVE_GEN_H */
