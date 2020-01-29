/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_PMW3360_H_
#define ZEPHYR_INCLUDE_PMW3360_H_

#include <drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pmw3360_attribute {
	PMW3360_ATTR_CPI = SENSOR_ATTR_PRIV_START,
	PMW3360_ATTR_REST_ENABLE,
	PMW3360_ATTR_RUN_DOWNSHIFT_TIME,
	PMW3360_ATTR_REST1_DOWNSHIFT_TIME,
	PMW3360_ATTR_REST2_DOWNSHIFT_TIME,
	PMW3360_ATTR_REST1_SAMPLE_TIME,
	PMW3360_ATTR_REST2_SAMPLE_TIME,
	PMW3360_ATTR_REST3_SAMPLE_TIME,
};

#define PMW3360_SVALUE_TO_CPI(svalue) ((u32_t)(svalue).val1)
#define PMW3360_SVALUE_TO_TIME(svalue) ((u32_t)(svalue).val1)
#define PMW3360_SVALUE_TO_BOOL(svalue) ((svalue).val1 != 0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PMW3360_H_ */
