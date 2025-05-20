/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <sfloat.h>

#include <zephyr/sys/byteorder.h>

/* Maximum mantissa value: 2047. */
#define SFLOAT_MANTISSA_MAX ((1 << 11) - 1)
/* Minimum mantissa value: -2048. */
#define SFLOAT_MANTISSA_MIN (-(1 << 11))
/* Mantissa bit position in the SFLOAT encoding. */
#define SFLOAT_MANTISSA_BIT_POS (0)
/* Mantissa mask in the SFLOAT encoding. */
#define SFLOAT_MANTISSA_MASK (0x0FFF)

/* Minimum exponent value. */
#define SFLOAT_EXP_MIN (-8)
/* Maximum exponent value. */
#define SFLOAT_EXP_MAX (7)
/* Maximum exponentiation (exponent function) value: 10^7. */
#define SFLOAT_EXP_FUNC_MAX (10000000)
/* Exponent bit position in the SFLOAT encoding. */
#define SFLOAT_EXP_BIT_POS (12)
/* Exponent mask in the SFLOAT encoding. */
#define SFLOAT_EXP_MASK (0xF000)
/* Value of the exponent that is in SFLOAT special values. */
#define SFLOAT_SPECIAL_EXP 0
/* Minimum value of mantissa in SFLOAT special values. */
#define SFLOAT_SPECIAL_MANTISSA_MIN 2046

/* Size in bits of the float sign encoding. */
#define FLOAT_SIGN_BIT_SIZE 1
/* Size in bits of the float exponent encoding. */
#define FLOAT_EXP_BIT_SIZE 8
/* Size in bits of the float mantissa encoding. */
#define FLOAT_MANTISSA_BIT_SIZE 23

/* Absolute minimum float resolution. */
#define FLOAT_ABS_MIN (0.000001f)
/* Minimum negative float resolution. */
#define FLOAT_NEG_MIN (SFLOAT_EXP_FUNC_MAX * 1.0f * SFLOAT_MANTISSA_MIN)
/* Maximum positive float resolution. */
#define FLOAT_POS_MAX (SFLOAT_EXP_FUNC_MAX * 1.0f * SFLOAT_MANTISSA_MAX)

/* Helper macro for getting absolute value. */
#define ABS(a) ((a < 0) ? (-a) : (a))
/* Helper macro for dividing two integers with rounding upwards (ceiling). */
#define DIV_AND_CEIL(a, b) (((a) + ((b) - 1)) / (b))

/* Float type should use binary32 notation from the IEEE 754-2008 specification. */
BUILD_ASSERT(sizeof(float) == sizeof(uint32_t));

/* SFLOAT descriptor. */
struct sfloat_desc {
	uint8_t exponent;
	uint16_t mantissa;
};

/* FLOAT encoding descriptor. */
union float_enc {
	uint32_t val;
	struct {
		uint32_t mantissa: FLOAT_MANTISSA_BIT_SIZE;
		uint32_t exp: FLOAT_EXP_BIT_SIZE;
		uint32_t sign: FLOAT_SIGN_BIT_SIZE;
	};
};

static struct sfloat_desc sfloat_desc_from_float(float float_num)
{
	struct sfloat_desc sfloat = {0};
	union float_enc float_enc;
	uint16_t mantissa_max;
	uint16_t mantissa;
	float float_abs;
	bool inc_exp;
	int8_t exp = 0;

	float_enc.val = sys_get_le32((uint8_t *) &float_num);

	/* Handle zero float values. */
	if ((float_enc.exp == 0) && (float_enc.mantissa == 0)) {
		return sfloat;
	}

	/* Handle special float values. */
	if (float_enc.exp == UINT8_MAX) {
		if (float_enc.mantissa == 0) {
			sfloat.mantissa = float_enc.sign ?
					  SFLOAT_NEG_INFINITY : SFLOAT_POS_INFINITY;
		} else {
			sfloat.mantissa = SFLOAT_NAN;
		}

		return sfloat;
	}

	/* Verify if the SFLOAT has a proper resolution for the conversion. */
	float_abs = float_enc.sign ? (-float_num) : float_num;
	if ((float_abs < FLOAT_ABS_MIN) ||
	    (float_num < FLOAT_NEG_MIN) ||
	    (float_num > FLOAT_POS_MAX)) {
		sfloat.mantissa = SFLOAT_NRES;

		return sfloat;
	}

	/* Find mantissa and exponent for the SFLOAT type. */
	mantissa_max = float_enc.sign ? ABS(SFLOAT_MANTISSA_MIN) : SFLOAT_MANTISSA_MAX;
	inc_exp = float_abs > mantissa_max;
	while (exp > SFLOAT_EXP_MIN && exp < SFLOAT_EXP_MAX) {
		if (inc_exp) {
			if (float_abs <= mantissa_max) {
				break;
			}

			float_abs /= 10;
			exp++;
		} else {
			if ((float_abs * 10) > mantissa_max) {
				break;
			}

			float_abs *= 10;
			exp--;
		}
	}

	/* Round up the mantisssa. */
	mantissa = (uint16_t) float_abs;
	if (((float_abs - mantissa) * 10 >= 5) && (mantissa + 1 <= mantissa_max)) {
		mantissa++;
	}

	/* Encode mantissa and exponent in the two's-complement form. */
	if (exp >= 0) {
		sfloat.exponent = ((uint8_t) exp) & 0x0F;
	} else {
		sfloat.exponent = ((uint8_t) -exp) & 0x0F;
		sfloat.exponent = (~sfloat.exponent & 0x0F) + 1;
	}

	/* Make sure that special sfloat values are not returned */
	if ((sfloat.exponent == SFLOAT_SPECIAL_EXP) && (mantissa >= SFLOAT_SPECIAL_MANTISSA_MIN)) {
		sfloat.exponent++;
		mantissa = DIV_AND_CEIL(SFLOAT_SPECIAL_MANTISSA_MIN, 10);
	}

	sfloat.mantissa = mantissa & SFLOAT_MANTISSA_MASK;
	if (float_enc.sign) {
		sfloat.mantissa = (~sfloat.mantissa & SFLOAT_MANTISSA_MASK) + 1;
	}

	return sfloat;
}

static struct sfloat sfloat_encode(const struct sfloat_desc *sfloat_desc)
{
	struct sfloat sfloat;

	sfloat.val = ((sfloat_desc->exponent << SFLOAT_EXP_BIT_POS) & SFLOAT_EXP_MASK);
	sfloat.val |= ((sfloat_desc->mantissa << SFLOAT_MANTISSA_BIT_POS) & SFLOAT_MANTISSA_MASK);

	return sfloat;
}

struct sfloat sfloat_from_float(float float_num)
{
	struct sfloat sfloat;
	struct sfloat_desc sfloat_desc;

	sfloat_desc = sfloat_desc_from_float(float_num);
	sfloat = sfloat_encode(&sfloat_desc);

	return sfloat;
}
