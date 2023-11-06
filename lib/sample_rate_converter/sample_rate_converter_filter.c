/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sample_rate_converter.h"
#include "sample_rate_converter_filter.h"

#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_rate_converter_filter, CONFIG_SAMPLE_RATE_CONVERTER_LOG_LEVEL);

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
static q15_t filter_48khz_to_24khz_16bit_test[] = {0x3fff, 0x3fff};

static q15_t filter_24khz_to_48khz_16bit_test[] = {0x7fff, 0x7fff};

static q15_t filter_48khz_to_16khz_16bit_test[] = {0x2aaa, 0x2aaa, 0x2aaa};

static q15_t filter_16khz_to_48khz_16bit_test[] = {0x7fff, 0x7fff, 0x7fff};
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32

static q31_t filter_48khz_to_24khz_32bit_test[] = {0x3fffffff, 0x3fffffff};

static q31_t filter_24khz_to_48khz_32bit_test[] = {0x7fffffff, 0x7fffffff};

static q31_t filter_48khz_to_16khz_32bit_test[] = {0x2aaaaaaa, 0x2aaaaaaa, 0x2aaaaaaa};

static q31_t filter_16khz_to_48khz_32bit_test[] = {0x7fffffff, 0x7fffffff, 0x7fffffff};
#endif
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */

int sample_rate_converter_filter_get(enum sample_rate_converter_filter filter_type,
				     int conversion_ratio, void **filter_ptr, size_t *filter_size)
{

	__ASSERT(filter_ptr != NULL, "Filter pointer cannot be NULL");
	__ASSERT(filter_size != NULL, "Filter size pointer cannot be NULL");

	/* Filters only support the following conversions:
	 * - 16 kHz <-> 48 kHz
	 * - 24 kHz <-> 48kHz
	 * which means that the supported ratio is limited to 2 and 3.
	 */
	if ((abs(conversion_ratio) != 2) && (abs(conversion_ratio) != 3)) {
		LOG_ERR("Invalid conversion ratio: %d", conversion_ratio);
		return -EINVAL;
	}

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	switch (filter_type) {
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
	case SAMPLE_RATE_FILTER_TEST:
		LOG_WRN("Test filter for the sample rate converter has been selected, do NOT use "
			"in final implementation.");

		if (conversion_ratio == -2) {
			*filter_ptr = filter_48khz_to_24khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_16bit_test);
		} else if (conversion_ratio == 2) {
			*filter_ptr = filter_24khz_to_48khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_16bit_test);
		} else if (conversion_ratio == -3) {
			*filter_ptr = filter_48khz_to_16khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_16bit_test);
		} else if (conversion_ratio == 3) {
			*filter_ptr = filter_16khz_to_48khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_16bit_test);
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */
	default:
		LOG_ERR("No matching filter found");
		return -EINVAL;
	}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16 */

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	switch (filter_type) {
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
	case SAMPLE_RATE_FILTER_TEST:
		if (conversion_ratio == -2) {
			*filter_ptr = filter_48khz_to_24khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_32bit_test);
		} else if (conversion_ratio == 2) {
			*filter_ptr = filter_24khz_to_48khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_32bit_test);
		} else if (conversion_ratio == -3) {
			*filter_ptr = filter_48khz_to_16khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_32bit_test);
		} else if (conversion_ratio == 3) {
			*filter_ptr = filter_16khz_to_48khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_32bit_test);
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */
	default:
		LOG_ERR("No matching filter found");
		return -EINVAL;
	}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16 */
	return 0;
}
