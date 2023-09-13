/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief PHY init config parameters. These are passed to phy at init.
 */

#ifndef _PHY_RF_PARAMS_H_
#define _PHY_RF_PARAMS_H_
#include "pack_def.h"

#define NRF_WIFI_RF_PARAMS_SIZE 200
#define NRF_WIFI_RF_PARAMS_CONF_SIZE 42

#ifdef CONFIG_NRF700X_RADIO_TEST
#define NRF_WIFI_DEF_RF_PARAMS "0000000000002A0000000003030303544040383838383838000000003C00FC00F8FCFCFC00FC00000000007077003F032424001000002800323500000C0008087D8105010071630300EED501001F6F00003B350100F52E0000E35E0000B7B6000066EFFEFFB5F60000896200007A840200E28FFCFF080808080408120100000000A1A10178000000080050003B020726181818181A120A140E0600"
#define MAX_TX_PWR_SYS_TEST 30
#define MAX_TX_PWR_RADIO_TEST 24
#else
#define NRF_WIFI_DEF_RF_PARAMS "0000000000002A0000000003030303544040383838383838000000003C00FC00F8FCFCFC00FC00000000007077003F032424001000002800323500000CF008087D8105010071630300EED501001F6F00003B350100F52E0000E35E0000B7B6000066EFFEFFB5F60000896200007A840200E28FFCFF080808080408120100000000A1A10178000000080050003B020726181818181A120A140E0600"
#endif

#define NRF_WIFI_RF_PARAMS_OFF_RESV_1 0
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_X0 6
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PDADJM7 7
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PDADJM0 11
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR2G 15
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR2GM0M7 16
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM7 18
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_PWR5GM0 21
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_RXGNOFF 24
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_MAX_TEMP 28
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_MIN_TEMP 29
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_TXP_BOFF_2GH 30
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_TXP_BOFF_2GL 31
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_TXP_BOFF_5GH 32
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_TXP_BOFF_5GL 33
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_TXP_BOFF_V 34
#define NRF_WIFI_RF_PARAMS_OFF_CALIB_RESV_2 37


#define NRF_WIFI_PHY_CALIB_FLAG_RXDC 1
#define NRF_WIFI_PHY_CALIB_FLAG_TXDC 2
#define NRF_WIFI_PHY_CALIB_FLAG_TXPOW 0
#define NRF_WIFI_PHY_CALIB_FLAG_TXIQ 8
#define NRF_WIFI_PHY_CALIB_FLAG_RXIQ 16
#define NRF_WIFI_PHY_CALIB_FLAG_DPD  32

#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXDC (1<<16)
#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXDC (2<<16)
#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXPOW (0<<16)
#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXIQ (0<<16)
#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXIQ (0<<16)
#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_DPD (0<<16)

#define NRF_WIFI_DEF_PHY_CALIB (NRF_WIFI_PHY_CALIB_FLAG_RXDC |\
				NRF_WIFI_PHY_CALIB_FLAG_TXDC |\
				NRF_WIFI_PHY_CALIB_FLAG_RXIQ |\
				NRF_WIFI_PHY_CALIB_FLAG_TXIQ |\
				NRF_WIFI_PHY_CALIB_FLAG_TXPOW |\
				NRF_WIFI_PHY_CALIB_FLAG_DPD |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXDC |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXDC |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXIQ |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXIQ |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXPOW |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_DPD)

/* Temperature based calibration params */
#define NRF_WIFI_DEF_PHY_TEMP_CALIB (NRF_WIFI_PHY_CALIB_FLAG_RXDC |\
				     NRF_WIFI_PHY_CALIB_FLAG_TXDC |\
				     NRF_WIFI_PHY_CALIB_FLAG_RXIQ |\
				     NRF_WIFI_PHY_CALIB_FLAG_TXIQ |\
				     NRF_WIFI_PHY_CALIB_FLAG_TXPOW |\
				     NRF_WIFI_PHY_CALIB_FLAG_DPD)


#define NRF_WIFI_TEMP_CALIB_PERIOD (1024 * 1024) /* micro seconds */
#define NRF_WIFI_TEMP_CALIB_THRESHOLD (40)
#define NRF_WIFI_TEMP_CALIB_ENABLE 1

/* Battery voltage changes base calibrations and voltage thresholds */
#define NRF_WIFI_DEF_PHY_VBAT_CALIB (NRF_WIFI_PHY_CALIB_FLAG_DPD)
#define NRF_WIFI_VBAT_VERYLOW (8) /* Corresponds to (2.5+8*0.07)=3.06V */
#define NRF_WIFI_VBAT_LOW  (12)  /* Correspond to (2.5+12*0.07)=3.34V */
#define NRF_WIFI_VBAT_HIGH (14) /* Correspond to (2.5+14*0.07)=3.48V */



#ifdef CONFIG_NRF700X_RADIO_TEST
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
	NRF_WIFI_RF_TEST_EVENT_MAX,
};



/* Holds the RX capture related info */
struct nrf_wifi_rf_test_capture_params {
	unsigned char test;

	/* Number of samples to be captured. */
	unsigned short int cap_len;

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

	/*  Mean of I samples. Format: Q.11 */
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

	/* Desired tone frequency in MHz in steps of 1 MHz from -10 MHz to +10 MHz.*/
	signed char tone_freq;

	/* Desired TX power in the range -16 dBm to +24 dBm.
	 * in steps of 2 dBm
	 */
	signed char tx_pow;

	/* Set 1 for staring tone transmission.*/
	unsigned char enabled;
} __NRF_WIFI_PKD;

struct nrf_wifi_rf_test_dpd_params {
	unsigned char test;
	unsigned char enabled;

} __NRF_WIFI_PKD;

struct nrf_wifi_temperature_params {
	unsigned char test;

	/** current measured temperature */
	signed int temperature;

	/** Temperature measurment status.
	 *0: Reading successful
	 *1: Reading failed
	 */
	unsigned int readTemperatureStatus;
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

#endif /* CONFIG_NRF700X_RADIO_TEST */

/**
 * @brief This structure defines the parameters used to control the max transmit (TX) power
 * in both frequency bands for different data rates.
 *
 */

struct nrf_wifi_tx_pwr_ceil_params {
	/** Maximum power permitted while transmitting DSSS rates in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_dsss;
	/** Maximum power permitted while transmitting MCS0 rate in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 2.4G band.
	 *  Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_2g_mcs7;
	/** Maximum power permitted while transmitting MCS0 rate in 5G lowband.
	 * Low band corresponds to ch: 36 to 64 Resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_low_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G lowband.
	 * Low band corresponds to ch: 36 to 64, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_low_mcs7;
	/** Maximum power permitted while transmitting MCS0 rate in 5G midband.
	 * Mid band corresponds to ch: 96 to 132, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_mid_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G midband.
	 * Mid band corresponds to ch: 96 to 132, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_mid_mcs7;
	/** Maximum power permitted while transmitting MCS0 rate in 5G highband.
	 * High band corresponds to ch: 136 to 177, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_high_mcs0;
	/** Maximum power permitted while transmitting MCS7 rate in 5G highband.
	 * High band corresponds to ch: 136 to 177, resolution is 0.25dBm.
	 */
	unsigned char max_pwr_5g_high_mcs7;
	/** Flag to determine presence of overriding, default parameters present
	 * in RF parameters string.
	 */
	unsigned char rf_tx_pwr_ceil_params_override;
} __NRF_WIFI_PKD;

/* FT Prog version info */
enum ft_prog_ver {
	FT_PROG_VER1 = 1,
	FT_PROG_VER2 = 2,
	FT_PROG_VER3 = 3
};

/* FT Prog version dependent back off values
 * from Max transmit power
 */
#define FT_PROG_VER1_2G_DSSS_TXCEIL_BKOFF 0
#define FT_PROG_VER1_2G_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER1_5G_LOW_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER1_5G_MID_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER1_5G_HIGH_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER2_2G_DSSS_TXCEIL_BKOFF 0
#define FT_PROG_VER2_2G_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER2_5G_LOW_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER2_5G_MID_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER2_5G_HIGH_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER3_2G_DSSS_TXCEIL_BKOFF 0
#define FT_PROG_VER3_2G_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER3_5G_LOW_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER3_5G_MID_OFDM_TXCEIL_BKOFF 0
#define FT_PROG_VER3_5G_HIGH_OFDM_TXCEIL_BKOFF 0

#endif
