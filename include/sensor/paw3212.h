/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_PAW3212_H_
#define ZEPHYR_INCLUDE_PAW3212_H_

#include <drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum paw3212_attribute {
	PAW3212_ATTR_CPI = SENSOR_ATTR_PRIV_START,
	PAW3212_ATTR_SLEEP_ENABLE,
	PAW3212_ATTR_SLEEP1_TIMEOUT,
	PAW3212_ATTR_SLEEP2_TIMEOUT,
	PAW3212_ATTR_SLEEP3_TIMEOUT,
	PAW3212_ATTR_SLEEP1_SAMPLE_TIME,
	PAW3212_ATTR_SLEEP2_SAMPLE_TIME,
	PAW3212_ATTR_SLEEP3_SAMPLE_TIME,
};

#define PAW3212_SVALUE_TO_CPI(svalue) ((u32_t)(svalue).val1)
#define PAW3212_SVALUE_TO_TIME(svalue) ((u32_t)(svalue).val1)
#define PAW3212_SVALUE_TO_BOOL(svalue) ((svalue).val1 != 0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PAW3212_H_ */
