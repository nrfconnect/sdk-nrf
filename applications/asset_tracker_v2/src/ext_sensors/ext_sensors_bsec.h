/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

int ext_sensors_bsec_temperature_get(double *temperature);

int ext_sensors_bsec_humidity_get(double *humidity);

int ext_sensors_bsec_pressure_get(double *pressure);

int ext_sensors_bsec_air_quality_get(uint16_t *air_quality);

int ext_sensors_bsec_init(void);
