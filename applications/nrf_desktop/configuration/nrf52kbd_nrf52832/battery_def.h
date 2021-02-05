/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file is included only once from battery level measuring
 * module and holds information about battery characteristic.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} battery_def_include_once;

#define BATTERY_MEAS_ADC_INPUT		NRF_SAADC_INPUT_VDD
#define BATTERY_MEAS_ADC_GAIN		ADC_GAIN_1_6
#define BATTERY_MEAS_VOLTAGE_GAIN	6

/* Converting measured battery voltage[mV] to State of Charge[%].
 * First element corresponds to CONFIG_DESKTOP_BATTERY_MIN_LEVEL.
 * Each element is CONFIG_DESKTOP_VOLTAGE_TO_SOC_DELTA higher than previous.
 * Defined separately for every configuration.
 */
static const uint8_t battery_voltage_to_soc[] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,
 1,  1,  2,  2,  2,  2,  3,  3,  3,  4,  4,  5,  5,  6,  6,  7,  8,  9,  9, 10,
11, 13, 14, 15, 16, 18, 19, 21, 23, 24, 26, 28, 31, 33, 35, 37, 40, 42, 45, 47,
50, 52, 54, 57, 59, 62, 64, 66, 68, 71, 73, 75, 76, 78, 80, 81, 83, 84, 85, 86,
88, 89, 90, 90, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 97, 97, 98,
98, 98, 98, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 100,
100
};
