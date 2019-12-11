/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @brief Light Lightness model internal defines
 */

#ifndef LIGHTNESS_INTERNAL_H__
#define LIGHTNESS_INTERNAL_H__

#include <bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MESH_LIGHTNESS_LINEAR)
#define LIGHT_USER_REPR LINEAR
#else
#define LIGHT_USER_REPR ACTUAL
#endif /* CONFIG_BT_MESH_LIGHTNESS_LINEAR */

/** The lightness server's value is > 0 */
#define LIGHTNESS_SRV_FLAG_IS_ON BIT(0)
/** Flag for preventing startup behavior on the server */
#define LIGHTNESS_SRV_FLAG_NO_START BIT(1)

enum light_repr {
	ACTUAL,
	LINEAR,
};

static inline u32_t lightness_sqrt32(u32_t val)
{
	/* Shortcut out of this for the very common case of 0: */
	if (val == 0) {
		return 0;
	}

	/* Square root by binary search from the highest bit: */
	u32_t factor = 0;

	/* sqrt(UINT32_MAX) < (1 << 16), so the highest bit will be bit 15: */
	for (int i = 15; i >= 0; --i) {
		factor |= BIT(i);
		if (factor * factor > val) {
			factor &= ~BIT(i);
		}
	}

	return factor;
}

static inline u16_t linear_to_actual(u16_t linear)
{
	/* Conversion: actual = 65535 * sqrt(linear / 65535) */
	return lightness_sqrt32(65535UL * linear);
}

static inline u16_t actual_to_linear(u16_t actual)
{
	/* Conversion:
	 * linear = CEIL(65535 * (actual * actual) / (65535 * 65535)))
	 */
	return ceiling_fraction((u32_t) actual * (u32_t) actual, 65535UL);
}

/** @brief Convert light from the specified representation to the configured.
 *
 *  @param val  Value to convert.
 *  @param repr Representation the value is in.
 *
 *  @return The light value in the configured representation.
 */
static inline u16_t repr_to_light(u16_t val, enum light_repr repr)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR) && repr == ACTUAL) {
		return actual_to_linear(val);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL) && repr == LINEAR) {
		return linear_to_actual(val);
	}

	return val;
}

/** @brief Convert light from the configured representation to the specified.
 *
 *  @param light Light value in the configured representation.
 *  @param repr  Representation to convert to
 *
 *  @return The light value in the representation specified in @c repr.
 */
static inline u16_t light_to_repr(u16_t light, enum light_repr repr)
{
	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_LINEAR) && repr == ACTUAL) {
		return linear_to_actual(light);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHTNESS_ACTUAL) && repr == LINEAR) {
		return actual_to_linear(light);
	}

	return light;
}

#ifdef __cplusplus
}
#endif

#endif /* LIGHTNESS_INTERNAL_H__ */
