/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file is included only once from battery level measuring module
 * and holds information about battery characteristic.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} battery_def_include_once;

#define BATTERY_MEAS_ADC_INPUT		NRF_SAADC_INPUT_AIN3
#define BATTERY_MEAS_ADC_GAIN		ADC_GAIN_1
#define BATTERY_MEAS_VOLTAGE_GAIN	1

/* Converting measured battery voltage[mV] to State of Charge[%].
 * First element corresponds to CONFIG_DESKTOP_BATTERY_MIN_LEVEL.
 * Each element is CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA higher than previous.
 * Defined separately for every configuration.
 */
static const uint8_t battery_voltage_to_soc[] = {
0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,
2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
4,  5,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 11, 12, 13, 13, 14, 15, 16,
18, 19, 22, 25, 28, 32, 36, 40, 44, 47, 51, 53, 56, 58, 60, 62, 64, 66, 67, 69,
71, 72, 74, 76, 77, 79, 81, 82, 84, 85, 85, 86, 86, 86, 87, 88, 88, 89, 90, 91,
91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 100
};
