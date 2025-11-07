/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <bluetooth/cs_de.h>

#define NUM_CHANNELS (75)
#define CHANNEL_SPACING_HZ  (1e6f)
#define PI (3.14159265358979f)
#define SPEED_OF_LIGHT_M_PER_S (299792458.0f)

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

/* Generate ideal IQ data for a given distance in meters.*/
static void generate_ideal_iq_data(float distance, cs_de_iq_tones_t *iq_tones)
{
	float rotation_per_channel =
		2 * PI * CHANNEL_SPACING_HZ * distance / SPEED_OF_LIGHT_M_PER_S;
	for (int i = 0; i < NUM_CHANNELS; i++) {
		iq_tones->i_local[i] = 100 * cosf(-rotation_per_channel * i);
		iq_tones->q_local[i] = 100 * sinf(-rotation_per_channel * i);
		iq_tones->i_remote[i] = 100 * cosf(-rotation_per_channel * i);
		iq_tones->q_remote[i] = 100 * sinf(-rotation_per_channel * i);
	}
}

void test_cs_de_calc_empty_report(void)
{
	cs_de_report_t test_report;
	cs_de_quality_t result;

	/* Setup: Initialize report with no antenna paths */
	test_report.n_ap = 0;

	/* Execute */
	result = cs_de_calc(&test_report);

	/* Verify: Should return DO_NOT_USE quality */
	TEST_ASSERT_EQUAL(CS_DE_QUALITY_DO_NOT_USE, result);
}

void test_cs_de_calc_bad_tone_quality(void)
{
	cs_de_report_t test_report;
	cs_de_quality_t result;
	/* Setup: Initialize report with 1 antenna path but bad tone quality */
	test_report.n_ap = 1;
	test_report.tone_quality[0] = CS_DE_TONE_QUALITY_BAD;

	/* Execute */
	result = cs_de_calc(&test_report);

	/* Verify: Should return DO_NOT_USE quality due to bad tone quality */
	TEST_ASSERT_EQUAL(CS_DE_QUALITY_DO_NOT_USE, result);
}

void test_cs_de_calc_with_ideal_iq_data(void)
{
	for (uint8_t n_ap = 1; n_ap <= CONFIG_BT_RAS_MAX_ANTENNA_PATHS; n_ap++) {
		for (float distance = 0.0f; distance < 74.5f; distance += 0.1f) {
			cs_de_report_t test_report;
			cs_de_quality_t result;

			test_report.n_ap = n_ap;

			for (uint8_t ap = 0; ap < n_ap; ap++) {
				test_report.tone_quality[ap] = CS_DE_TONE_QUALITY_OK;
				generate_ideal_iq_data(distance, &test_report.iq_tones[ap]);
			}

			result = cs_de_calc(&test_report);
			TEST_ASSERT_TRUE(result == CS_DE_QUALITY_OK);

			for (uint8_t ap = 0; ap < n_ap; ap++) {
				/* Verify that the estimated distance is within 1 cm of the distance
				 * used to generate the ideal IQ data.
				 */
				TEST_ASSERT_FLOAT_WITHIN(0.01f,
					distance,
					test_report.distance_estimates[ap].ifft);
				TEST_ASSERT_FLOAT_WITHIN(0.01f,
					distance,
					test_report.distance_estimates[ap].phase_slope);
				TEST_ASSERT_EQUAL_FLOAT(test_report.distance_estimates[ap].ifft,
							test_report.distance_estimates[ap].best);
			}
		}
	}
}

/* Main test entry point */
int main(void)
{
	(void)unity_main();

	return 0;
}
