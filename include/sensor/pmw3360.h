/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PMW3360_H_ */
