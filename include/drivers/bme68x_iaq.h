/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BME68X_NCS_H_
#define _BME68X_NCS_H_

/**
 * @file bme68x_iaq.h
 *
 * @brief Public API for the bme68x driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_CHAN_IAQ	       (SENSOR_CHAN_PRIV_START + 1)
#define SENSOR_CHAN_IAQ_ACC    (SENSOR_CHAN_PRIV_START + 2)
#define SENSOR_CHAN_CO2_ACC    (SENSOR_CHAN_PRIV_START + 3)
#define SENSOR_CHAN_VOC_ACC    (SENSOR_CHAN_PRIV_START + 4)
#define SENSOR_CHAN_GAS_RUN_IN (SENSOR_CHAN_PRIV_START + 5)
#define SENSOR_CHAN_GAS_STAB   (SENSOR_CHAN_PRIV_START + 6)

enum bme68x_accuracy {
	BME68X_ACCURACY_UNRELIABLE = 0,
	BME68X_ACCURACY_LOW,
	BME68X_ACCURACY_MEDIUM,
	BME68X_ACCURACY_HIGH,
};

#ifdef __cplusplus
}
#endif

#endif /* _BME68X_NCS_H_ */
