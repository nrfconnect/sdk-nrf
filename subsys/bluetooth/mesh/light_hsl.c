/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/light_hsl.h>
#include "lightness_internal.h"

static uint16_t hue_to_rgb_ch(uint16_t hue, enum bt_mesh_rgb_ch ch,
			      uint16_t min, uint16_t max)
{
	/* Split the hue spectrum into 6 equal segments, as show in
	 * https://en.wikipedia.org/wiki/Hue:
	 * Each color channel follows this sequence:
	 * 1. MAX
	 * 2. Linear transition from MAX to MIN
	 * 3. MIN
	 * 4. MIN
	 * 5. Linear transition from MIN to MAX
	 * 6. MAX
	 *
	 * Generating a 6 segment waveform like this:
	 *  MAX |____                ____|
	 *      |    \              /    |
	 *      |     \            /     |
	 *      |      \          /      |
	 *  MIN |       \________/       |
	 *      |.   .   .   .   .   .   |
	 *      |1   2   3   4   5   6   |
	 *
	 * The three color channels are spread evenly across the spectrum, with
	 * a 2 segment offset per channel (ie red starts at segment 0, green at
	 * segment 4, blue at segment 2). This way, there'll always be one
	 * channel transitioning and two channels resting for every segment.
	 */
	const uint16_t len = (BT_MESH_LIGHT_HSL_MAX + 1) / 6;
	const uint16_t offset = hue + (4 * len * ch); /* may roll over */
	const uint8_t segment = offset / len;
	const uint16_t seg_start = segment * len;

	switch (segment) {
	case 0:
		return max;
	case 1:
		return max - ((max - min) * (offset - seg_start)) / len;
	case 2:
		return min;
	case 3:
		return min;
	case 4:
		return min + ((max - min) * (offset - seg_start)) / len;
	case 5:
		return max;
	}

	return 0;
}

uint16_t bt_mesh_light_hsl_to_rgb(const struct bt_mesh_light_hsl *hsl,
				  enum bt_mesh_rgb_ch ch)
{
	uint16_t lightness, min, max;

	lightness = to_actual(hsl->lightness);

	/* When the color is monochromatic, it's equivalent to lightness: */
	if (!hsl->saturation) {
		return lightness;
	}

	/* The hue waveform should use the following min and max values:
	 *
	 *  MAX = V
	 *  MIN = V * (1 - S')
	 *
	 * Where V and S' is the Value and Saturation in the HSV format.
	 *
	 * Convert first from HSL to HS'V, where:
	 *
	 *  V  = L + S * min(L, 1 - L)
	 *  S' = 2 * (1 - L / V)
	 *
	 * Note how the saturation (S') in HSV is not the same as the
	 * saturation in HSL.
	 *
	 * As the values range from 0 to BT_MESH_LIGHT_HSL_MAX, each instance of
	 * H, S and L must be divided by BT_MESH_LIGHT_HSL_MAX to end up on a
	 * scale from 0 to 1.
	 */
	if (lightness < BT_MESH_LIGHT_HSL_MAX / 2) {
		/* MAX = V
		 *     = L + S * L
		 *     = L * (1 + S)
		 */
		max = ((lightness * (BT_MESH_LIGHT_HSL_MAX + hsl->saturation)) /
		       BT_MESH_LIGHT_HSL_MAX);
	} else {
		/* MAX = V
		 *     = L + S * (1 - L)
		 *     = L + S - S * L
		 *     = L * (1 - S) + S
		 */
		max = ((lightness * (BT_MESH_LIGHT_HSL_MAX - hsl->saturation)) /
		       BT_MESH_LIGHT_HSL_MAX) +
		      hsl->saturation;
	}

	/* MIN = V * (1 - S')
	 *     = V * (1 - (2 * (1 - L / V)))
	 *     = V * (1 - 2 + 2 * L / V)
	 *     = V * (2 * L / V - 1)
	 *     = 2 * L - V
	 *     = 2 * L - max:
	 */
	min = (2 * lightness) - max;

	return hue_to_rgb_ch(hsl->hue, ch, min, max);
}
