/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <sfloat.h>

ZTEST_SUITE(sfloat_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(sfloat_suite, test_sfloat_from_float)
{
	struct sfloat res;
	struct {
		float in_float;
		struct sfloat out_sfloat;
	} test_vector[] = {
		/* Basic values: three significant figure guarantee */
		{0.00, {0x0000}}, /* 10^(0) * 0 = 0.000000 */
		{-0.00, {0x0000}}, /* 10^(0) * 0 = 0.000000 */
		{0.000678, {0xA2A6}}, /* 10^(-6) * 678 = 0.000678 */
		{-0.000678, {0xAD5A}}, /* - 10^(-6) * 678 = -0.000678 */
		{0.00123, {0xA4CE}}, /* 10^(-6) * 1230 = 0.001230 */
		{-0.00123, {0xAB32}}, /* - 10^(-6) * 1230 = -0.001230 */
		{0.0475, {0xC1DB}}, /* 10^(-4) * 475 = 0.047500 */
		{-0.0475, {0xCE25}}, /* - 10^(-4) * 475 = -0.047500 */
		{0.827, {0xD33B}}, /* 10^(-3) * 827 = 0.827000 */
		{-0.827, {0xDCC5}}, /* - 10^(-3) * 827 = -0.827000 */
		{2.75, {0xE113}}, /* 10^(-2) * 275 = 2.750000 */
		{-2.75, {0xEEED}}, /* - 10^(-2) * 275 = -2.750000 */
		{10.3, {0xE406}}, /* 10^(-2) * 1030 = 10.300000 */
		{-10.3, {0xEBFA}}, /* - 10^(-2) * 1030 = -10.300000 */
		{391.0, {0x0187}}, /* 10^(0) * 391 = 391.00000 */
		{-391.0, {0x0E79}}, /* - 10^(0) * 391 = -391.000000 */
		{5070.0, {0x11FB}}, /* 10^(1) * 507 = 5070.000000 */
		{-5070.0, {0x1E05}}, /* - 10^(1) * 507 = -5070.000000 */
		{96100.0, {0x23C1}}, /* 10^(2) * 961 = 96100.000000 */
		{-96100.0, {0x2C3F}}, /* - 10^(2) * 961 = -96100.000000 */
		{278000.0, {0x3116}}, /* 10^(3) * 278 = 278000.000000 */
		{-278000.0, {0x3EEA}}, /* - 10^(3) * 278 = -278000.00000 */
		{9510000.0, {0x43B7}}, /* 10^(4) * 951 = 9510000.000000 */
		{-9510000.0, {0x4C49}}, /* - 10^(4) * 951 = -9510000.000000 */

		/* Rounding up or down */
		{105.899, {0xF423}}, /* 10^(-1) * 1059 = 105.900000 */
		{-105.899, {0xFBDD}}, /* - 10^(-1) * 1059 = -105.900000 */
		{197.850, {0xF7BB}}, /* 10^(-1) * 1979 = 197.9000000 */
		{-197.850, {0xF845}}, /* - 10^(-1) * 1979 = -197.900000 */
		{153.249, {0xF5FC}}, /* 10^(-1) * 1532 = 153.200000 */
		{-153.249, {0xFA04}}, /* - 10^(-1) * 1532 = -153.200000 */
		{171.601, {0xF6B4}}, /* 10^(-1) * 1716 = 171.600000 */
		{-171.601, {0xF94C}}, /* - 10^(-1) * 1716 = -171.600000 */

		/* Low values */
		{0.000001, {0x8064}}, /* 10^(-8) * 100 = 0.000001 */
		{-0.000001, {0x8F9C}}, /* - 10^(-8) * 100 = 0.000001 */
		{0.00000101, {0x8065}}, /* 10^(-8) * 101 = 0.00000101 */
		{-0.00000101, {0x8F9B}}, /* - 10^(-8) * 101 = -0.00000101 */
		{0.000001001, {0x8064}}, /* 10^(-8) * 100 = 0.000001 */
		{-0.000001001, {0x8F9C}}, /* - 10^(-8) * 100 = 0.000001 */
		{0.00000100000001, {0x8064}}, /* 10^(-8) * 100 = 0.000001 */
		{-0.00000100000001, {0x8F9C}}, /* - 10^(-8) * 100 = 0.000001 */

		/* High values */
		{20470000000.0, {0x77FF}}, /* 10^(7) * 2047 = 20470000000.000000 */
		{20469000000.0, {0x77FF}}, /* 10^(7) * 2047 = 20470000000.000000 */
		{-20480000000.0, {0x7800}}, /* - 10^(7) * 2048 = -20480000000.00000 */
		{-20479000000.0, {0x7800}}, /* - 10^(7) * 2048 = -20480000000.00000 */

		/* Mantisssa - edge cases */
		{2047.0, {0x07FF}}, /* 10^(0) * 2047 = 2047.00000 */
		{2048.0, {0x10CD}}, /* 10^(1) * 205 = 2050.00000 */
		{-2047.0, {0x0801}}, /* - 10^(0) * 2047 = -2047.00000 */
		{-2048.0, {0x0800}}, /* - 10^(0) * 2048 = -2048.00000 */
		{2047.5, {0x10CD}}, /* 10^(1) * 205 = 2050.00000 */
		{2048.5, {0x10CD}}, /* 10^(1) * 205 = 2050.00000 */
		{-2047.5, {0x0800}}, /* - 10^(0) * 2048 = -2048.00000 */
		{-2048.5, {0x1F33}}, /* - 10^(1) * 205 = -2050.00000 */

		/* Special values */
		{(0.0 / 0.0), {SFLOAT_NAN}}, /* SFLOAT NaN */
		{(1.0 / 0.0), {SFLOAT_POS_INFINITY}}, /* SFLOAT Positive Infinity */
		{(-1.0 / 0.0), {SFLOAT_NEG_INFINITY}}, /* SFLOAT Negative Infinity */
		{ 0.0000009999999, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution */
		{-0.0000009999999, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution */
		{ 0.0000000000001, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution */
		{-0.0000000000001, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution */
		{ 20470010000.0, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution */
		{-20480010000.0, {SFLOAT_NRES}}, /* SFLOAT Not At This Resolution0 */
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct sfloat out_sfloat = test_vector[i].out_sfloat;
		float in_float = test_vector[i].in_float;

		res = sfloat_from_float(in_float);
		zassume_true(res.val == out_sfloat.val,
			"comparison failed for the sample[%d] equal to %f: 0x%04X != 0x%04X",
			i, in_float, res.val, out_sfloat.val);
	}
}
