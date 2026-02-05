/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief PHY init config parameters. These are passed to phy at init.
 */

#ifndef _PHY_RF_PARAMS_RT_H_
#define _PHY_RF_PARAMS_RT_H_
#include "common/phy_rf_params_common.h"

#define NRF_WIFI_RT_DEF_RF_PARAMS "007077003F032424001000002800323500000C0008087D8105010071630300EED501001F6F00003B350100F52E0000E35E0000B7B6000066EFFEFFB5F60000896200007A840200E28FFCFF080808080408120100000000A1A10178000000080050003B020726181818181A120A140E0600"
#define MAX_TX_PWR_SYS_TEST 30
#define MAX_TX_PWR_RADIO_TEST 24

#define MAX_CAPTURE_LEN 16383
#define MIN_CAPTURE_LEN 0
#define RX_CAPTURE_TIMEOUT_CONST 11
#define CAPTURE_DURATION_IN_SEC 600

#define VBAT_OFFSET_MILLIVOLT (2500)
#define VBAT_SCALING_FACTOR (70)

enum nrf_wifi_rf_test {
	NRF_WIFI_RF_TEST_RX_ADC_CAP,
	NRF_WIFI_RF_TEST_RX_STAT_PKT_CAP,
	NRF_WIFI_RF_TEST_RX_DYN_PKT_CAP,
	NRF_WIFI_RF_TEST_TX_TONE,
	NRF_WIFI_RF_TEST_DPD,
	NRF_WIFI_RF_TEST_RF_RSSI,
	NRF_WIFI_RF_TEST_SLEEP,
	NRF_WIFI_RF_TEST_GET_TEMPERATURE,
	NRF_WIFI_RF_TEST_XO_CALIB,
	NRF_WIFI_RF_TEST_XO_TUNE,
	NRF_WIFI_RF_TEST_GET_BAT_VOLT,
	NRF_WIFI_RF_TEST_MAX,
};

enum nrf_wifi_rf_test_event {
	NRF_WIFI_RF_TEST_EVENT_RX_ADC_CAP,
	NRF_WIFI_RF_TEST_EVENT_RX_STAT_PKT_CAP,
	NRF_WIFI_RF_TEST_EVENT_RX_DYN_PKT_CAP,
	NRF_WIFI_RF_TEST_EVENT_TX_TONE_START,
	NRF_WIFI_RF_TEST_EVENT_DPD_ENABLE,
	NRF_WIFI_RF_TEST_EVENT_RF_RSSI,
	NRF_WIFI_RF_TEST_EVENT_SLEEP,
	NRF_WIFI_RF_TEST_EVENT_TEMP_MEAS,
	NRF_WIFI_RF_TEST_EVENT_XO_CALIB,
	NRF_WIFI_RF_TEST_EVENT_XO_TUNE,
	NRF_WIFI_RF_TEST_EVENT_GET_BAT_VOLT,
	NRF_WIFI_RF_TEST_EVENT_MAX,
};

/* Holds the RX capture related info */
struct nrf_wifi_rf_test_capture_params {
	unsigned char test;

	/* Number of samples to be captured. */
	unsigned short int cap_len;

	/* Capture timeout in seconds. */
	unsigned short int cap_time;

	/* Capture status codes:
	 *0: Capture successful after WLAN packet detection
	 *1: Capture failed after WLAN packet detection
	 *2: Capture timedout as no WLAN packets are detected
	 */
	unsigned char capture_status;

	/* LNA Gain to be configured. It is a 3 bit value. The mapping is,
	 * '0' = 24dB
	 * '1' = 18dB
	 * '2' = 12dB
	 * '3' = 0dB
	 * '4' = -12dB
	 */
	unsigned char lna_gain;

	/* Baseband Gain to be configured. It is a 5 bit value.
	 * It supports 64dB range.The increment happens lineraly 2dB/step
	 */
	unsigned char bb_gain;
} __NRF_WIFI_PKD;

/* Struct to hold the events from RF test SW. */
struct nrf_wifi_rf_test_capture_meas {
	unsigned char test;

	/* Mean of I samples. Format: Q.11 */
	signed short mean_I;

	/* Mean of Q samples. Format: Q.11 */
	signed short mean_Q;

	/* RMS of I samples */
	unsigned int rms_I;

	/* RMS of Q samples */
	unsigned int rms_Q;
} __NRF_WIFI_PKD;

/* Holds the transmit related info */
struct nrf_wifi_rf_test_tx_params {
	unsigned char test;

	/* Desired tone frequency in MHz in steps of 1 MHz from -10 MHz to +10 MHz. */
	signed char tone_freq;

	/* Desired TX power in the range -16 dBm to +24 dBm.
	 * in steps of 2 dBm
	 */
	signed char tx_pow;

	/* Set 1 for staring tone transmission. */
	unsigned char enabled;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_test_dpd_params {
	unsigned char test;
	unsigned char enabled;

} __NRF_WIFI_PKD;

struct nrf_wifi_temperature_params {
	unsigned char test;

	/** Current measured temperature */
	signed int temperature;

	/** Temperature measurment status.
	 * 0: Reading successful
	 * 1: Reading failed
	 */
	unsigned int readTemperatureStatus;
} __NRF_WIFI_PKD;

struct nrf_wifi_bat_volt_params {
	unsigned char test;
	/** Measured battery voltage in milliVolts. */
	unsigned char voltage;
	/** Status of the voltage measurement command.
	 * 0: Reading successful
	 * 1: Reading failed
	 */
	unsigned int cmd_status;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_get_rf_rssi {
	unsigned char test;
	unsigned char lna_gain;
	unsigned char bb_gain;
	unsigned char agc_status_val;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_test_xo_calib {
	unsigned char test;

	/* XO value in the range between 0 to 127 */
	unsigned char xo_val;

} __NRF_WIFI_PKD;

struct nrf_wifi_rf_get_xo_value {
	unsigned char test;

	/* Optimal XO value computed. */
	unsigned char xo_value;
} __NRF_WIFI_PKD;

#endif /* _PHY_RF_PARAMS_RT_H_ */
