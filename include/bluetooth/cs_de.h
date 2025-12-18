/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CS_DE_H__
#define CS_DE_H__

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>

#define CS_DE_NUM_CHANNELS (75)

#if defined(CONFIG_BT_RAS)
#define CS_DE_NUM_ANTENNA_PATHS CONFIG_BT_RAS_MAX_ANTENNA_PATHS
#else
#define CS_DE_NUM_ANTENNA_PATHS CONFIG_BT_CTLR_SDC_CS_MAX_ANTENNA_PATHS
#endif

/** @file
 *  @defgroup bt_cs_de Channel Sounding Distance Estimation API
 *  @{
 *  @brief API for the Channel Sounding Distance Estimation toolkit.
 */

/**
 * @brief Container of IQ values for local and remote measured tones
 *
 * The values are indexed by channel from 2 (2404 MHz) to 76 (2479 Mhz).
 * Channels 23 to 25 are reserved for advertising and do not have IQ values.
 */
typedef struct {
	/** In-phase measurements of tones on this device */
	float i_local[CS_DE_NUM_CHANNELS];
	/** Quadrature-phase measurement of tones on this device */
	float q_local[CS_DE_NUM_CHANNELS];
	/** In-phase measurements of tones from remote device */
	float i_remote[CS_DE_NUM_CHANNELS];
	/** Quadrature-phase measurements of tones from remote device */
	float q_remote[CS_DE_NUM_CHANNELS];
} cs_de_iq_tones_t;

typedef enum {
	CS_DE_TONE_QUALITY_OK,
	CS_DE_TONE_QUALITY_BAD,
} cs_de_tone_quality_t;

/**
 * @brief Container of distance estimate results for a number of different
 * methods, in meters.
 */
typedef struct {
	/** Distance estimate based on inverse fourier transform. */
	float ifft;
	/** Distance estimate based on average phase slope. */
	float phase_slope;
	/** Distance estimate based on RTT. */
	float rtt;
	/** Best effort distance estimate.
	 *
	 *  This is a convenience value which is automatically set to the most
	 *  accurate of the estimation methods.
	 */
	float best;
} cs_de_dist_estimates_t;

/**
 * @brief Quality of the procedure
 */
typedef enum {
	CS_DE_QUALITY_OK,
	CS_DE_QUALITY_DO_NOT_USE,
} cs_de_quality_t;

/**
 * @brief Output data for distance estimation
 */
typedef struct {
	/** CS Role. */
	enum bt_conn_le_cs_role role;

	/** Number of antenna paths present in data. */
	uint8_t n_ap;

	/** IQ values for local and remote measured tones. */
	cs_de_iq_tones_t iq_tones[CS_DE_NUM_ANTENNA_PATHS];

	/** Tone quality indicators */
	cs_de_tone_quality_t tone_quality[CS_DE_NUM_ANTENNA_PATHS];

	/** Distance estimate results */
	cs_de_dist_estimates_t distance_estimates[CS_DE_NUM_ANTENNA_PATHS];

	/** Total time measured during RTT measurements */
	int32_t rtt_accumulated_half_ns;

	/** Number of RTT measurements taken */
	uint8_t rtt_count;
} cs_de_report_t;

#if defined(CONFIG_BT_RAS)
/**
 * @brief Partially populate the report.
 * This populates the report but does not set the distance estimates and the quality.
 * @param[in] local_steps Buffer to the local step data to parse.
 * @param[in] peer_steps Buffer to the peer ranging data to parse.
 * @param[in] config CS config of the local controller.
 * @param[out] p_report Report populated with the raw data from the last ranging.
 */
void cs_de_populate_report(struct net_buf_simple *local_steps, struct net_buf_simple *peer_steps,
			   struct bt_conn_le_cs_config *config, cs_de_report_t *p_report);
#endif

/**
 * @brief Combine the local and remote IQ values in a cs_de_iq_tones_t to one array.
 * This is done through an element wise complex multiplication of the local and remote IQ values.
 * @param[in] cs_de_iq_tones to IQ values to combine
 * @param[out] iq_tones_comb pointer to allocated memory for combined IQ values. In the combined
 * memory even indexes are the imaginary component of the combined IQ value and odd indexes are the
 * real component.
 */
void cs_de_combined_iq_calculate(cs_de_iq_tones_t *cs_de_iq_tones,
				 float iq_tones_comb[CS_DE_NUM_CHANNELS * 2]);

/**
 * @brief Calculate distance estimates and quality for a given report
 * This function assumes the report has been populated in all fields except the distance estimates
 * and quality fields.
 * @param[inout] p_report The partially populated report to calculate distance with
 * @return Quality of the distance estimates, if at least one estimate is valid this will be
 * CS_DE_QUALITY_OK. Otherwise it is CS_DE_QUALITY_DO_NOT_USE and estimates should be ignored.
 */
cs_de_quality_t cs_de_calc(cs_de_report_t *p_report);

/**
 * @brief Calculates a distance estimate based on the IFFT magnitude of the input IQ values.
 * Note! After calling this function, the input IQ values in iq_tones_comb are overwritten with the
 * IFFT magnitude.
 * @param[inout] iq_tones_comb combined IQ values from two devices. The first CS_DE_NUM_CHANNELS * 2
 * elements should match the format described in @ref cs_de_combined_iq_calculate
 * @return Distance estimate between the two devices in meters
 */
float cs_de_ifft(float iq_tones_comb[2 * CONFIG_BT_CS_DE_NFFT_SIZE]);

/**
 * @brief Calculate a distance estimate based on the accumulated RTT
 * To do this, average time of flight is calculated and multiplied with the speed of light.
 * @param[in] rtt_accumulated_half_ns the accumulated total of RTT values over a CS procedure in
 * units of 0.5 ns.
 * @param[in] rtt_count the total number of rtt measurements in the procedure
 * @return Distance estimate between the two devices in meters
 */
float cs_de_rtt(int32_t rtt_accumulated_half_ns, uint8_t rtt_count);

/**
 * @brief Calculate distance estimate based on the phase slope of the input IQ values
 * @param[in] iq_tones_comb combined IQ values from two devices. The first CS_DE_NUM_CHANNELS * 2
 * elements should match the format described in @ref cs_de_combined_iq_calculate
 * @return Distance estimate between the two devices in meters
 */
float cs_de_phase_slope(float iq_tones_comb[CS_DE_NUM_CHANNELS * 2]);

/**
 * @}
 */

#endif /* CS_DE_H__ */
