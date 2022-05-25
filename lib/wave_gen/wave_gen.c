/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>

#include <math.h>
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wave_gen, CONFIG_WAVE_GEN_LIB_LOG_LEVEL);

#include <wave_gen.h>

/**
 * @brief Generates a pseudo-random number between -1 and 1.
 *
 * @return Pseudo-random number.
 */
static double generate_pseudo_random(void)
{
	return rand() / (RAND_MAX / 2.0) - 1.0;
}

/**
 * @brief Calculate sine wave value.
 *
 * @param[in]	time	Time for generated value (lower than the sine wave period).
 * @param[in]	period	Sine wave period.
 *
 * @return Sine wave value for given time.
 */
static double sine_val(uint32_t time, uint32_t period)
{
	double angle = 2 * M_PI * time / period;

	return sin(angle);
}

/**
 * @brief Calculate triangle wave value.
 *
 * @param[in]	time	Time for generated value (lower than the triangle wave period).
 * @param[in]	period	Triangle wave period.
 *
 * @return Triangle wave value for given time.
 */
static double triangle_val(uint32_t time, uint32_t period)
{
	static const double amplitude = 1.0;
	uint32_t line_time = period / 2;
	double res;
	double change;

	if (time < line_time) {
		change = 2 * amplitude * time / line_time;
		res = -amplitude + change;
	} else {
		time -= line_time;
		change = 2 * amplitude * time / line_time;
		res = amplitude - change;
	}

	return res;
}

/**
 * @brief Calculate square wave value.
 *
 * @param[in]	time	Time for generated value (lower than the square wave period).
 * @param[in]	period	Square wave period.
 *
 * @return Square wave value for given time.
 */
static double square_val(uint32_t time, uint32_t period)
{
	static const double amplitude = 1.0;

	return ((time < (period / 2)) ? (-amplitude) : (amplitude));
}

int wave_gen_generate_value(uint32_t time, const struct wave_gen_param *params, double *out_val)
{
	double res;

	if (params->period_ms == 0) {
		if (params->type != WAVE_GEN_TYPE_NONE) {
			return -EINVAL;
		}
	} else {
		time = time % params->period_ms;
	}

	switch (params->type) {
	case WAVE_GEN_TYPE_SINE:
		res = sine_val(time, params->period_ms);
		break;

	case WAVE_GEN_TYPE_TRIANGLE:
		res = triangle_val(time, params->period_ms);
		break;

	case WAVE_GEN_TYPE_SQUARE:
		res = square_val(time, params->period_ms);
		break;

	case WAVE_GEN_TYPE_NONE:
		res = 0.0;
		break;

	default:
		return -EINVAL;
	}

	res *= params->amplitude;
	res += params->offset;
	res += params->noise * generate_pseudo_random();

	*out_val = res;

	return 0;
}
