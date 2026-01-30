/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief PHY init config parameters. These are passed to phy at init.
 */

#ifndef _PHY_RF_PARAMS_COMMON_H_
#define _PHY_RF_PARAMS_COMMON_H_
#include "pack_def.h"

#define NRF_WIFI_RF_PARAMS_SIZE 200
#define NRF_WIFI_RF_PARAMS_CONF_SIZE 42
#define NUM_PCB_LOSS_OFFSET 4

#define NRF_WIFI_PHY_CALIB_FLAG_RXDC 1
#ifdef NRF71_PASSIVE_SCAN_ONLY
	/* Disable TX DC Calibration for passive scan alone mode */
	#define NRF_WIFI_PHY_CALIB_FLAG_TXDC 0
#else
	#define NRF_WIFI_PHY_CALIB_FLAG_TXDC 2
#endif
#define NRF_WIFI_PHY_CALIB_FLAG_TXPOW 0
#define NRF_WIFI_PHY_CALIB_FLAG_TXIQ 8
#define NRF_WIFI_PHY_CALIB_FLAG_RXIQ 16
#define NRF_WIFI_PHY_CALIB_FLAG_DPD 32
#define NRF_WIFI_PHY_CALIB_FLAG_ENHANCED_TXDC 64

#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXDC (1<<16)
#ifdef NRF71_PASSIVE_SCAN_ONLY
	/* Disable TX DC Calibration for passive scan alone mode */
	#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXDC (0<<16)
#else
	#define NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXDC (2<<16)
#endif
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
				NRF_WIFI_PHY_CALIB_FLAG_ENHANCED_TXDC |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXDC |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXDC |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_RXIQ |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXIQ |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_TXPOW |\
				NRF_WIFI_PHY_SCAN_CALIB_FLAG_DPD)

/* Temperature based calibration params */
#define NRF_WIFI_DEF_PHY_TEMP_CALIB (NRF_WIFI_PHY_CALIB_FLAG_RXDC |\
				     NRF_WIFI_PHY_CALIB_FLAG_TXDC |\
				     NRF_WIFI_PHY_CALIB_FLAG_ENHANCED_TXDC |\
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
#define NRF_WIFI_VBAT_LOW (12)  /* Correspond to (2.5+12*0.07)=3.34V */
#define NRF_WIFI_VBAT_HIGH (14) /* Correspond to (2.5+14*0.07)=3.48V */

/** Package type information written to the OTP memory */
#define QFN_PACKAGE_INFO 0x5146
#define CSP_PACKAGE_INFO 0x4345

/* Package independent params */

/** Power detector adjustment value. */
#define PD_ADJUST_VAL 0

#ifdef NRF71_PASSIVE_SCAN_ONLY
	/** The byte is repurposed and bit-0 is used to enable TX power optimizations */
	#define CTRL_PWR_OPTIMIZATIONS 1
#else
	#define CTRL_PWR_OPTIMIZATIONS 0
#endif
/** RX gain adjustment value for both 2.4GHz and 5 GHz bands */
#define RX_GAIN_OFFSET_HB_LOW_CHAN 0
#define RX_GAIN_OFFSET_HB_MID_CHAN 0
#define RX_GAIN_OFFSET_HB_HIGH_CHAN 0

/** End of package independent params */


/** QFN Package dependent params */

/** XO adjustment value */
#define QFN_XO_VAL 0x2A


/** Systematic error between set power and measured power in dBm */
#define QFN_DSSS_BKF 3
#define QFN_OFDM_BKF 3
#define QFN_SYSTEM_OFFSET_LB  ((QFN_DSSS_BKF << 4) | QFN_OFDM_BKF)
#define QFN_SYSTEM_OFFSET_HB_CHAN_LOW 3
#define QFN_SYSTEM_OFFSET_HB_CHAN_MID 3
#define QFN_SYSTEM_OFFSET_HB_CHAN_HIGH 3

/** Max TX power allowed for DSSS and OFDM in 2.4GHz band */
#define QFN_MAX_TX_PWR_DSSS 0x54
#define QFN_MAX_TX_PWR_LB_MCS7 0x40
#define QFN_MAX_TX_PWR_LB_MCS0 0x40

/** Max TX power allowed for MCS7 for channels in the range,
 * 36 to 64, 96 to 132 and 136 to 177
 */
#define QFN_MAX_TX_PWR_HB_LOW_CHAN_MCS7 0x34
#define QFN_MAX_TX_PWR_HB_MID_CHAN_MCS7 0x34
#define QFN_MAX_TX_PWR_HB_HIGH_CHAN_MCS7 0x30

/** Max TX power allowed for MCS0 for channels in the range,
 * 36 to 64, 96 to 132 and 136 to 177
 */
#define QFN_MAX_TX_PWR_HB_LOW_CHAN_MCS0 0x38
#define QFN_MAX_TX_PWR_HB_MID_CHAN_MCS0 0x34
#define QFN_MAX_TX_PWR_HB_HIGH_CHAN_MCS0 0x30

/** Max chip temperature at which the TX power backoff to be applied. */
#define QFN_MAX_CHIP_TEMP 0x43

/** Min chip temperature at which the TX power backoff to be applied. */
#define QFN_MIN_CHIP_TEMP 0x07

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * chip temperature crosses MAX_CHIP_TEMP. The resolution is in 0.25dB.
 * To get 1 dB backoff configure -4(0xFC)
 */
#define QFN_LB_MAX_PWR_BKF_HI_TEMP 0xFC
#define QFN_LB_MAX_PWR_BKF_LOW_TEMP 0x00
#define QFN_HB_MAX_PWR_BKF_HI_TEMP 0xF8
#define QFN_HB_MAX_PWR_BKF_LOW_TEMP 0xFC

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * the voltage is less than NRF_WIFI_VBAT_VERYLOW
 */
#define QFN_LB_VBT_LT_VLOW 0xFC
#define QFN_HB_VBT_LT_VLOW 0xF8

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * the voltage is less than NRF_WIFI_VBAT_LOW
 */
#define QFN_LB_VBT_LT_LOW 0x00
#define QFN_HB_VBT_LT_LOW 0xFC


/** CSP Package dependent params */

/** XO adjustment value */
#define CSP_XO_VAL 0x2A


/** Systematic error between set power and measured power in dBm in 2.4G band */
#define CSP_DSSS_BKF 4
#define CSP_OFDM_BKF 5
#define CSP_SYSTEM_OFFSET_LB ((CSP_DSSS_BKF << 4) | CSP_OFDM_BKF)

/** Systematic error between set power and measured power in dBm in 5G band */
#define CSP_SYSTEM_OFFSET_HB_CHAN_LOW 5
#define CSP_SYSTEM_OFFSET_HB_CHAN_MID 5
#define CSP_SYSTEM_OFFSET_HB_CHAN_HIGH 5

/** Max TX power allowed for DSSS and OFDM in 2.4GHz band */
#define CSP_MAX_TX_PWR_DSSS 0x44
#define CSP_MAX_TX_PWR_LB_MCS7 0x3C
#define CSP_MAX_TX_PWR_LB_MCS0 0x3C

/** Max TX power allowed for MCS7 for channels in the range,
 * 36 to 64, 96 to 132 and 136 to 177
 */
#define CSP_MAX_TX_PWR_HB_LOW_CHAN_MCS7 0x34
#define CSP_MAX_TX_PWR_HB_MID_CHAN_MCS7 0x34
#define CSP_MAX_TX_PWR_HB_HIGH_CHAN_MCS7 0x34

/** Max TX power allowed for MCS0 for channels in the range,
 * 36 to 64, 96 to 132 and 136 to 177
 */
#define CSP_MAX_TX_PWR_HB_LOW_CHAN_MCS0 0x34
#define CSP_MAX_TX_PWR_HB_MID_CHAN_MCS0 0x34
#define CSP_MAX_TX_PWR_HB_HIGH_CHAN_MCS0 0x34

/** Max chip temperature at which the TX power backoff to be applied. */
#define CSP_MAX_CHIP_TEMP 0x43

/** Min chip temperature at which the TX power backoff to be applied. */
#define CSP_MIN_CHIP_TEMP 0x07

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * chip temperature crosses MAX_CHIP_TEMP. The resolution is in 0.25dB.
 * To get 1 dB backoff configure -4(0xFC)
 */
#define CSP_LB_MAX_PWR_BKF_HI_TEMP 0xEC
#define CSP_LB_MAX_PWR_BKF_LOW_TEMP 0x00
#define CSP_HB_MAX_PWR_BKF_HI_TEMP 0xFC
#define CSP_HB_MAX_PWR_BKF_LOW_TEMP 0xF4

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * the voltage is less than NRF_WIFI_VBAT_VERYLOW
 */
#define CSP_LB_VBT_LT_VLOW 0xFC
#define CSP_HB_VBT_LT_VLOW 0xEC

/** TX power backoff values to be applied in 2.4GHz and 5GHz band when
 * the voltage is less than NRF_WIFI_VBAT_LOW
 */
#define CSP_LB_VBT_LT_LOW 0x00
#define CSP_HB_VBT_LT_LOW 0xF8


/** XO adjustment value */
struct nrf_wifi_xo_freq_offset {
	unsigned char xo_freq_offset;
} __NRF_WIFI_PKD;

/** Power detector adjustment factor for MCS7 */
struct nrf_wifi_pd_adst_val {
	/** PD adjustment value corresponding to Channel 7 */
	signed char pd_adjt_lb_chan;
	/** PD adjustment value corresponding to Channel 36 */
	signed char pd_adjt_hb_low_chan;
	/** PD adjustment value corresponding to Channel 100 */
	signed char pd_adjt_hb_mid_chan;
	/** PD adjustment value corresponding to Channel 165 */
	signed char pd_adjt_hb_high_chan;
} __NRF_WIFI_PKD;

/** TX power systematic offset is the difference between set power
 *  and the measured power
 */
struct nrf_wifi_tx_pwr_systm_offset {
	/** Systematic adjustment value corresponding to Channel 7 */
	signed char syst_off_lb_chan;
	/** Systematic adjustment value corresponding to Channel 36 */
	signed char syst_off_hb_low_chan;
	/** Systematic adjustment value corresponding to Channel 100 */
	signed char syst_off_hb_mid_chan;
	/** Systematic adjustment value corresponding to Channel 165 */
	signed char syst_off_hb_high_chan;
} __NRF_WIFI_PKD;

/** Max TX power value for which both EVM and SEM pass */
struct nrf_wifi_tx_pwr_ceil {
	/** Max output power for 11b for channel 7 */
	signed char max_dsss_pwr;
	/** Max output power for MCS7 for channel 7 */
	signed char max_lb_mcs7_pwr;
	/** Max output power for MCS0 for channel 7 */
	signed char max_lb_mcs0_pwr;
	/** Max output power for MCS7 for channel 36 */
	signed char max_hb_low_chan_mcs7_pwr;
	/** Max output power for MCS7 for channel 100 */
	signed char max_hb_mid_chan_mcs7_pwr;
	/** Max output power for MCS7 for channel 165 */
	signed char max_hb_high_chan_mcs7_pwr;
	/** Max output power for MCS0 for channel 36 */
	signed char max_hb_low_chan_mcs0_pwr;
	/** Max output power for MCS0 for channel 100 */
	signed char max_hb_mid_chan_mcs0_pwr;
	/** Max output power for MCS0 for channel 165 */
	signed char max_hb_high_chan_mcs0_pwr;
} __NRF_WIFI_PKD;

/** RX gain adjustment offsets */
struct nrf_wifi_rx_gain_offset {
	/** Channel 7 */
	signed char rx_gain_lb_chan;
	/** Channel 36 */
	signed char rx_gain_hb_low_chan;
	/** Channel 100 */
	signed char rx_gain_hb_mid_chan;
	/** Channel 165 */
	signed char rx_gain_hb_high_chan;
} __NRF_WIFI_PKD;

/** Voltage and temperature dependent backoffs */
struct nrf_wifi_temp_volt_depend_params {
	/** Maximum chip temperature in centigrade */
	signed char max_chip_temp;
	/** Minimum chip temperature in centigrade */
	signed char min_chip_temp;
	/** TX power backoff at high temperature in 2.4GHz */
	signed char lb_max_pwr_bkf_hi_temp;
	/** TX power backoff at low temperature in 2.4GHz */
	signed char lb_max_pwr_bkf_low_temp;
	/** TX power backoff at high temperature in 5GHz */
	signed char hb_max_pwr_bkf_hi_temp;
	/** TX power backoff at low temperature in 5GHz */
	signed char hb_max_pwr_bkf_low_temp;
	/** Voltage back off value in LowBand when VBAT< VBAT_VERYLOW */
	signed char lb_vbt_lt_vlow;
	/** Voltage back off value in HighBand when VBAT< VBAT_VERYLOW */
	signed char hb_vbt_lt_vlow;
	/** Voltage back off value in LowBand when VBAT< VBAT_LOW */
	signed char lb_vbt_lt_low;
	/** Voltage back off value in HighBand when VBAT< VBAT_LOW */
	signed char hb_vbt_lt_low;
	/** Reserved bytes */
	signed char reserved[4];
} __NRF_WIFI_PKD;

/** The top-level structure holds substructures,
 * each containing information related to the
 * first 42 bytes of RF parameters.
 */
struct nrf_wifi_phy_rf_params {
	unsigned char reserved[6];
	struct nrf_wifi_xo_freq_offset xo_offset;
	struct nrf_wifi_pd_adst_val pd_adjust_val;
	struct nrf_wifi_tx_pwr_systm_offset syst_tx_pwr_offset;
	struct nrf_wifi_tx_pwr_ceil max_pwr_ceil;
	struct nrf_wifi_rx_gain_offset rx_gain_offset;
	struct nrf_wifi_temp_volt_depend_params temp_volt_backoff;
	unsigned char phy_params[NRF_WIFI_RF_PARAMS_SIZE - NRF_WIFI_RF_PARAMS_CONF_SIZE];
} __NRF_WIFI_PKD;

/** The byte offsets of RF parameters indicate the start offset
 * of various RF parameters, such as XO, power detector adjust
 * parameters, power ceiling parameters, RX gain adjustment parameters,
 * and temperature and voltage-based power backoff values.
 */
enum RF_PARAMS_OFFSETS {
	NRF_WIFI_XO_FREQ_BYTE_OFFSET = 6,
	NRF_WIFI_PD_ADST_VAL_BYTE_OFFSET = 7,
	NRF_WIFI_TX_PWR_SYSTM_BYTE_OFFSET = 11,
	NRF_WIFI_TX_PWR_CEIL_BYTE_OFFSET = 15,
	NRF_WIFI_RX_GAIN_BYTE_OFFSET = 24,
	NRF_WIFI_VT_DEPEND_PARAMS_BYTE_OFFSET = 28
};

/** RF Params from byte starting with offset
 * NRF_WIFI_TX_PWR_CEIL_BYTE_OFFSET contains the
 * TX power celings based on DSSS, OFDM , Frequency
 * band and MCS.
 * In 5GHz band we have three subbands based on
 * channel frequency, we have divided them in to
 * LOW BAND, MID BAND and HIGH BAND
 */
enum MAX_POWER_OFFSETS {
	NRF_WIFI_MAX_OP_PWR_DSSS_OFST,
	NRF_WIFI_MAX_OP_PWR_2PT4GHZ_OFDM_MCS7,
	NRF_WIFI_MAX_OP_PWR_2PT4GHZ_OFDM_MCS0,
	NRF_WIFI_MAX_OP_PWR_5GHZ_LB_MCS7,
	NRF_WIFI_MAX_OP_PWR_5GHZ_MID_MCS7,
	NRF_WIFI_MAX_OP_PWR_5GHZ_HI_MCS7,
	NRF_WIFI_MAX_OP_PWR_5GHZ_LB_MCS0,
	NRF_WIFI_MAX_OP_PWR_5GHZ_MID_MCS0,
	NRF_WIFI_MAX_OP_PWR_5GHZ_HI_MCS0
};

/** The byte offsets of RF parameters indicate the start offset
 * of PCB loss for 2.4 GHz and 5 GHz bands.
 */
enum PCB_LOSS_BYTE_OFFSETS {
	PCB_LOSS_BYTE_2G_OFST = 185,
	PCB_LOSS_BYTE_5G_BAND1_OFST,
	PCB_LOSS_BYTE_5G_BAND2_OFST,
	PCB_LOSS_BYTE_5G_BAND3_OFST
};

/** The byte offsets of RF parameters indicate the start offset
 * of antenna gain for 2.4 GHz and 5 GHz bands.
 */
enum ANT_GAIN_OFFSETS {
	ANT_GAIN_2G_OFST = 181,
	ANT_GAIN_5G_BAND1_OFST,
	ANT_GAIN_5G_BAND2_OFST,
	ANT_GAIN_5G_BAND3_OFST,
	NUM_ANT_GAIN = 4
};

/** The byte offsets of RF parameters indicate the start offset
 * of band edge backoffs for different frame formats and
 * different sub-bands of 2.4 GHz and 5 GHz frequency band.
 */
enum EDGE_BACKOFF_OFFSETS {
	BAND_2G_LW_ED_BKF_DSSS_OFST = 155,
	BAND_2G_LW_ED_BKF_HT_OFST,
	BAND_2G_LW_ED_BKF_HE_OFST,
	BAND_2G_UW_ED_BKF_DSSS_OFST,
	BAND_2G_UW_ED_BKF_HT_OFST,
	BAND_2G_UW_ED_BKF_HE_OFST,
	BAND_UNII_1_LW_ED_BKF_HT_OFST,
	BAND_UNII_1_LW_ED_BKF_HE_OFST,
	BAND_UNII_1_UW_ED_BKF_HT_OFST,
	BAND_UNII_1_UW_ED_BKF_HE_OFST,
	BAND_UNII_2A_LW_ED_BKF_HT_OFST,
	BAND_UNII_2A_LW_ED_BKF_HE_OFST,
	BAND_UNII_2A_UW_ED_BKF_HT_OFST,
	BAND_UNII_2A_UW_ED_BKF_HE_OFST,
	BAND_UNII_2C_LW_ED_BKF_HT_OFST,
	BAND_UNII_2C_LW_ED_BKF_HE_OFST,
	BAND_UNII_2C_UW_ED_BKF_HT_OFST,
	BAND_UNII_2C_UW_ED_BKF_HE_OFST,
	BAND_UNII_3_LW_ED_BKF_HT_OFST,
	BAND_UNII_3_LW_ED_BKF_HE_OFST,
	BAND_UNII_3_UW_ED_BKF_HT_OFST,
	BAND_UNII_3_UW_ED_BKF_HE_OFST,
	BAND_UNII_4_LW_ED_BKF_HT_OFST,
	BAND_UNII_4_LW_ED_BKF_HE_OFST,
	BAND_UNII_4_UW_ED_BKF_HT_OFST,
	BAND_UNII_4_UW_ED_BKF_HE_OFST,
	NUM_EDGE_BACKOFF = 26
};

/**
 * @brief This structure defines the parameters used to control the max transmit (TX) power
 * in both frequency bands for different data rates.
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

#endif /* _PHY_RF_PARAMS_COMMON_H_ */
