/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_PAW3212_H_
#define ZEPHYR_INCLUDE_PAW3212_H_

#include <sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum paw3212_attribute {
	PAW3212_ATTR_CPI = SENSOR_ATTR_PRIV_START,
	PAW3212_ATTR_SLEEP1_TIMEOUT,
	PAW3212_ATTR_SLEEP2_TIMEOUT,
	PAW3212_ATTR_SLEEP3_TIMEOUT,
};

/* CPI */
#define PAW3212_CPI_STEP		38u
#define PAW3212_CPI_MIN			(0x01 * PAW3212_CPI_STEP)
#define PAW3212_CPI_MAX			(0x3F * PAW3212_CPI_STEP)

#define PAW3212_SVALUE_TO_CPI(svalue) ((u32_t)(svalue).val1)
#define PAW3212_CPI_TO_SVALUE(cpi) ((struct sensor_value) {.val1 = cpi})

#define PAW3212_SVALUE_TO_TIME(svalue) ((u32_t)(svalue).val1)
#define PAW3212_TIME_TO_SVALUE(time) ((struct sensor_value) {.val1 = time})

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PAW3212_H_ */
