/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include "pcm_mix.h"

#define ZEQ(a, b) zassert_equal(a, b, "fail")

void verify_array_eq(int16_t *p1, int16_t *p2, uint32_t elements)
{
	while (elements--) {
		ZEQ(*p1++, *p2++);
	}
}

void test_mix_with_nothing(void)
{
	int ret;
	int16_t sample_a[] = { 0, 1, -1 };
	int16_t sample_r[] = { 0, 1, -1 };

	ret = pcm_mix(sample_a, sizeof(sample_a), NULL, 0, B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

void test_mix_with_silence(void)
{
	int ret;
	int16_t sample_a[] = { 0, 1, 0 };
	int16_t sample_b[] = { 0, 0, 2 };
	int16_t sample_r[] = { 0, 1, 2 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

void test_mix_various_legal(void)
{
	int ret;
	int16_t sample_a[] = { 12, 1, 2, 3, -4, 2000, -2000 };
	int16_t sample_b[] = { 12, 1, 2, -3, -4, 2000, -2000 };
	int16_t sample_r[] = { 24, 2, 4, 0, -8, 4000, -4000 };

	ret = pcm_mix(&sample_a, sizeof(sample_a), &sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

void test_illegal_arguments(void)
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

void test_high_values(void)
{
	int ret;
	int16_t sample_a[] = { INT16_MAX, INT16_MIN, INT16_MIN, INT16_MAX };
	int16_t sample_b[] = { 1, -1, 10, -10 };
	int16_t sample_r[] = { INT16_MAX, INT16_MIN, INT16_MIN + 10, INT16_MAX - 10 };

	ret = pcm_mix(sample_a, sizeof(sample_a), sample_b, sizeof(sample_b), B_MONO_INTO_A_MONO);
	ZEQ(ret, 0);

	verify_array_eq(sample_a, sample_r, ARRAY_SIZE(sample_r));
}

void test_mono_into_stereo_lr(void)
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

void test_mono_into_stereo_l(void)
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

void test_mono_into_stereo_r(void)
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

void test_main(void)
{
	ztest_test_suite(test_suite_pcm_mix,
		ztest_unit_test(test_mix_with_nothing),
		ztest_unit_test(test_mix_with_silence),
		ztest_unit_test(test_mix_various_legal),
		ztest_unit_test(test_illegal_arguments),
		ztest_unit_test(test_high_values),
		ztest_unit_test(test_mono_into_stereo_lr),
		ztest_unit_test(test_mono_into_stereo_l),
		ztest_unit_test(test_mono_into_stereo_r)
	);

	ztest_run_test_suite(test_suite_pcm_mix);
}
