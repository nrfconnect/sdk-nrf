/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SAMPLE_RATE_CONVERTER_FILTER_H_
#define _SAMPLE_RATE_CONVERTER_FILTER_H_

#include <dsp/filtering_functions.h>

/**
 * @brief Get the pointer to the filter coefficients.
 *
 * @param[in]	filter_type		Selected filter type.
 * @param[in]	conversion_ratio	Ratio for the conversion the filter will be used for.
 * @param[out]	filter_ptr		Pointer to the filter coefficients.
 * @param[out]	filter_size		Number of filter coefficients.
 *
 * @retval	0	On success.
 * @retval	-EINVAL	Conversion ratio not supported or no filter matching parameters found.
 */
int sample_rate_converter_filter_get(enum sample_rate_converter_filter filter_type,
				     int conversion_ratio, void **filter_ptr, size_t *filter_size);

#endif /* _SAMPLE_RATE_CONVERTER_FILTER_H_ */
