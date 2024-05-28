/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <pcm_mix.h>

#define ZEQ(a, b) zassert_equal(a, b, "fail")

void verify_array_eq(int16_t *p1, int16_t *p2, uint32_t elements)
{
	while (elements--) {
		ZEQ(*p1++, *p2++);
	}
}

ZTEST(suite_pcm_mix, test_mix_with_nothing)
{
	int ret;
	int16_t sample_a[] = { 0, 1, -1 };
	int16_t sample_r[] = { 0, 1, -1 };

	ret = pcm_mix(sample_a, sizeof(sample_a), NULL, 0, B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_mix_with_silence)
{
	int ret;
	int16_t sample_a[] = { 0, 1, 0 };
	int16_t sample_b[] = { 0, 0, 2 };
	int16_t sample_r[] = { 0, 1, 2 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_mix_various_legal)
{
	int ret;
	int16_t sample_a[] = { 12, 1, 2, 3, -4, 2000, -2000 };
	int16_t sample_b[] = { 12, 1, 2, -3, -4, 2000, -2000 };
	int16_t sample_r[] = { 24, 2, 4, 0, -8, 4000, -4000 };

	ret = pcm_mix(&sample_a, sizeof(sample_a), &sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_illegal_arguments)
{
	int ret;
	int16_t sample_a[] = { 0, 1, 2 };
	int16_t sample_r[] = { 0, 1, 2 };

	/* NULL pointer */
	ret = pcm_mix(NULL, sizeof(sample_a), sample_a, sizeof(sample_a), B_MONO_INTO_A_MONO);
	zassert_not_equal(ret, 0, "Returned zero with illegal argument");
	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));

	/* Zero length */
	ret = pcm_mix(sample_a, 0, sample_a, sizeof(sample_a), B_MONO_INTO_A_MONO);
	zassert_not_equal(ret, 0, "Returned zero with illegal argument");
	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_high_values)
{
	int ret;
	int16_t sample_a[] = { INT16_MAX, INT16_MIN, INT16_MIN, INT16_MAX };
	int16_t sample_b[] = { 1, -1, 10, -10 };
	int16_t sample_r[] = { INT16_MAX, INT16_MIN, INT16_MIN + 10, INT16_MAX - 10 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_mono_into_stereo_lr)
{
	int ret;
	int16_t sample_a[] = { 10, 10, 10, 10 };
	int16_t sample_b[] = { -5, 5 };
	int16_t sample_r[] = { 5, 5, 15, 15 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b),
		      B_MONO_INTO_A_STEREO_LR);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_mono_into_stereo_l)
{
	int ret;
	int16_t sample_a[] = { 10, 10, 10, 10 };
	int16_t sample_b[] = { -5, 5 };
	int16_t sample_r[] = { 5, 10, 15, 10 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b),
		      B_MONO_INTO_A_STEREO_L);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST(suite_pcm_mix, test_mono_into_stereo_r)
{
	int ret;
	int16_t sample_a[] = { 10, 10, 10, 10 };
	int16_t sample_b[] = { -5, 5 };
	int16_t sample_r[] = { 10, 5, 10, 15 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b),
		      B_MONO_INTO_A_STEREO_R);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

ZTEST_SUITE(suite_pcm_mix, NULL, NULL, NULL, NULL, NULL);
