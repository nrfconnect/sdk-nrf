/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CS_DE_H__
#define CS_DE_H__

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>

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
	float i_local[75];
	/** Quadrature-phase measurement of tones on this device */
	float q_local[75];
	/** In-phase measurements of tones from remote device */
	float i_remote[75];
	/** Quadrature-phase measurements of tones from remote device */
	float q_remote[75];
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
	cs_de_iq_tones_t iq_tones[CONFIG_BT_RAS_MAX_ANTENNA_PATHS];

	/** Tone quality indicators */
	cs_de_tone_quality_t tone_quality[CONFIG_BT_RAS_MAX_ANTENNA_PATHS];

	/** Distance estimate results */
	cs_de_dist_estimates_t distance_estimates[CONFIG_BT_RAS_MAX_ANTENNA_PATHS];

	/** Total time measured during RTT measurements */
	int32_t rtt_accumulated_half_ns;

	/** Number of RTT measurements taken */
	uint8_t rtt_count;
} cs_de_report_t;

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

/* Takes partially populated report and calculates distance estimates and quality. */
cs_de_quality_t cs_de_calc(cs_de_report_t *p_report);

/**
 * @}
 */

#endif /* CS_DE_H__ */
