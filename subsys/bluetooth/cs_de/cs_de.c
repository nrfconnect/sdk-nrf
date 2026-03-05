/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <dsp/transform_functions.h>
#include <dsp/fast_math_functions.h>
#include <dsp/statistics_functions.h>
#include <arm_const_structs.h>
#include <bluetooth/cs_de.h>

#if defined(CONFIG_BT_RAS)
#include <bluetooth/services/ras.h>
#endif

LOG_MODULE_REGISTER(cs_de, CONFIG_BT_CS_DE_LOG_LEVEL);

#define PI		       3.14159265358979f
#define SPEED_OF_LIGHT_M_PER_S (299792458.0f)

#define CHANNEL_INDEX_OFFSET (2)

#define TONE_QI_OK_TONE_COUNT_THRESHOLD (15)

#define PHASE_INVALID (2 * PI)

#define CHANNEL_SPACING_HZ (1e6f)
#define NORMAL_PEAK_TO_NULL                                                                        \
	((CONFIG_BT_CS_DE_NFFT_SIZE + CS_DE_NUM_CHANNELS - 1) / (CS_DE_NUM_CHANNELS))

static float m_iq_scratch_mem[2 * CONFIG_BT_CS_DE_NFFT_SIZE];

#if defined(CONFIG_BT_RAS)
static uint16_t m_n_iqs[CONFIG_BT_RAS_MAX_ANTENNA_PATHS][CS_DE_NUM_CHANNELS];
static cs_de_tone_quality_t m_tone_quality_indicators[CONFIG_BT_RAS_MAX_ANTENNA_PATHS]
						     [CS_DE_NUM_CHANNELS];

static bool m_is_tone_quality_ok(cs_de_tone_quality_t *p_tone_qi, uint8_t channel_map[10])
{
	uint8_t ok_tones_count = 0;

	for (uint8_t i = 0; i < CS_DE_NUM_CHANNELS; ++i) {
		if (BT_LE_CS_CHANNEL_BIT_GET(channel_map, i + CHANNEL_INDEX_OFFSET) &&
		    p_tone_qi[i] == CS_DE_TONE_QUALITY_OK) {
			ok_tones_count += 1;
		}
	}
	return (ok_tones_count >= TONE_QI_OK_TONE_COUNT_THRESHOLD);
}

static void cumulate_mean(float *avg, float new_value, uint16_t *N)
{
	float a = 1.0f / (*N);
	float b = 1.0f - a;
	*avg = a * new_value + b * (*avg);
}

static void extract_pcts(cs_de_report_t *p_report, uint8_t channel_index,
			 uint8_t antenna_permutation_index,
			 struct bt_hci_le_cs_step_data_tone_info *local_tone_info,
			 struct bt_hci_le_cs_step_data_tone_info *remote_tone_info)
{

	for (uint8_t tone_index = 0; tone_index < p_report->n_ap; tone_index++) {
		int antenna_path = bt_le_cs_get_antenna_path(p_report->n_ap,
							     antenna_permutation_index, tone_index);
		if (antenna_path < 0) {
			LOG_WRN("Invalid antenna path.");
			return;
		}

		if (local_tone_info[tone_index].quality_indicator !=
			    BT_HCI_LE_CS_TONE_QUALITY_HIGH ||
		    remote_tone_info[tone_index].quality_indicator !=
			    BT_HCI_LE_CS_TONE_QUALITY_HIGH) {
			return;
		}

		struct bt_le_cs_iq_sample local_iq =
			bt_le_cs_parse_pct(local_tone_info[tone_index].phase_correction_term);
		struct bt_le_cs_iq_sample remote_iq =
			bt_le_cs_parse_pct(remote_tone_info[tone_index].phase_correction_term);

		m_n_iqs[antenna_path][channel_index]++;
		m_tone_quality_indicators[antenna_path][channel_index] = CS_DE_TONE_QUALITY_OK;

		if (m_n_iqs[antenna_path][channel_index] == 1) {
			p_report->iq_tones[antenna_path].i_local[channel_index] = local_iq.i;
			p_report->iq_tones[antenna_path].q_local[channel_index] = local_iq.q;
			p_report->iq_tones[antenna_path].i_remote[channel_index] = remote_iq.i;
			p_report->iq_tones[antenna_path].q_remote[channel_index] = remote_iq.q;
		} else {
			cumulate_mean(&p_report->iq_tones[antenna_path].i_local[channel_index],
				      local_iq.i, &m_n_iqs[antenna_path][channel_index]);
			cumulate_mean(&p_report->iq_tones[antenna_path].q_local[channel_index],
				      local_iq.q, &m_n_iqs[antenna_path][channel_index]);
			cumulate_mean(&p_report->iq_tones[antenna_path].i_remote[channel_index],
				      remote_iq.i, &m_n_iqs[antenna_path][channel_index]);
			cumulate_mean(&p_report->iq_tones[antenna_path].q_remote[channel_index],
				      remote_iq.q, &m_n_iqs[antenna_path][channel_index]);
		}
	}
}

static void extract_rtt_timings(cs_de_report_t *p_report,
				struct bt_hci_le_cs_step_data_mode_1 *local_rtt_data,
				struct bt_hci_le_cs_step_data_mode_1 *peer_rtt_data)
{
	if (local_rtt_data->packet_quality_aa_check !=
		    BT_HCI_LE_CS_PACKET_QUALITY_AA_CHECK_SUCCESSFUL ||
	    local_rtt_data->packet_rssi == BT_HCI_LE_CS_PACKET_RSSI_NOT_AVAILABLE ||
	    local_rtt_data->tod_toa_reflector == BT_HCI_LE_CS_TIME_DIFFERENCE_NOT_AVAILABLE ||
	    peer_rtt_data->packet_quality_aa_check !=
		    BT_HCI_LE_CS_PACKET_QUALITY_AA_CHECK_SUCCESSFUL ||
	    peer_rtt_data->packet_rssi == BT_HCI_LE_CS_PACKET_RSSI_NOT_AVAILABLE ||
	    peer_rtt_data->tod_toa_reflector == BT_HCI_LE_CS_TIME_DIFFERENCE_NOT_AVAILABLE) {
		return;
	}

	if (p_report->role == BT_CONN_LE_CS_ROLE_INITIATOR) {
		p_report->rtt_accumulated_half_ns +=
			local_rtt_data->toa_tod_initiator - peer_rtt_data->tod_toa_reflector;
	} else {
		p_report->rtt_accumulated_half_ns +=
			peer_rtt_data->toa_tod_initiator - local_rtt_data->tod_toa_reflector;
	}

	p_report->rtt_count++;
}

static bool process_ranging_header(struct ras_ranging_header *ranging_header, void *user_data)
{
	cs_de_report_t *p_report = (cs_de_report_t *)user_data;

	p_report->n_ap = ((ranging_header->antenna_paths_mask & BIT(0)) +
			  ((ranging_header->antenna_paths_mask & BIT(1)) >> 1) +
			  ((ranging_header->antenna_paths_mask & BIT(2)) >> 2) +
			  ((ranging_header->antenna_paths_mask & BIT(3)) >> 3));
	return true;
}

static bool process_step_data(struct bt_le_cs_subevent_step *local_step,
			      struct bt_le_cs_subevent_step *peer_step, void *user_data)
{
	cs_de_report_t *p_report = (cs_de_report_t *)user_data;

	if (local_step->mode == BT_HCI_OP_LE_CS_MAIN_MODE_2) {
		struct bt_hci_le_cs_step_data_mode_2 *local_step_data =
			(struct bt_hci_le_cs_step_data_mode_2 *)local_step->data;
		struct bt_hci_le_cs_step_data_mode_2 *peer_step_data =
			(struct bt_hci_le_cs_step_data_mode_2 *)peer_step->data;

		extract_pcts(p_report, local_step->channel - CHANNEL_INDEX_OFFSET,
			     local_step_data->antenna_permutation_index, local_step_data->tone_info,
			     peer_step_data->tone_info);
	} else if (local_step->mode == BT_HCI_OP_LE_CS_MAIN_MODE_1) {
		struct bt_hci_le_cs_step_data_mode_1 *local_step_data =
			(struct bt_hci_le_cs_step_data_mode_1 *)local_step->data;
		struct bt_hci_le_cs_step_data_mode_1 *peer_step_data =
			(struct bt_hci_le_cs_step_data_mode_1 *)peer_step->data;

		extract_rtt_timings(p_report, local_step_data, peer_step_data);
	} else if (local_step->mode == BT_HCI_OP_LE_CS_MAIN_MODE_3) {
		struct bt_hci_le_cs_step_data_mode_3 *local_step_data =
			(struct bt_hci_le_cs_step_data_mode_3 *)local_step->data;
		struct bt_hci_le_cs_step_data_mode_3 *peer_step_data =
			(struct bt_hci_le_cs_step_data_mode_3 *)peer_step->data;

		extract_pcts(p_report, local_step->channel - CHANNEL_INDEX_OFFSET,
			     local_step_data->antenna_permutation_index, local_step_data->tone_info,
			     peer_step_data->tone_info);

		extract_rtt_timings(p_report,
				    (struct bt_hci_le_cs_step_data_mode_1 *)local_step_data,
				    (struct bt_hci_le_cs_step_data_mode_1 *)peer_step_data);
	}

	return true;
}

void cs_de_populate_report(struct net_buf_simple *local_steps, struct net_buf_simple *peer_steps,
			   struct bt_conn_le_cs_config *config, cs_de_report_t *p_report)
{
	memset(p_report, 0x0, sizeof(*p_report));
	memset(m_n_iqs, 0, sizeof(m_n_iqs));
	memset(m_tone_quality_indicators, CS_DE_TONE_QUALITY_BAD,
	       sizeof(m_tone_quality_indicators));

	p_report->role = config->role;

	bt_ras_rreq_rd_subevent_data_parse(peer_steps, local_steps, config->role,
					   process_ranging_header, NULL, process_step_data,
					   p_report);

	for (uint8_t ap = 0; ap < p_report->n_ap; ap++) {
		p_report->distance_estimates[ap].ifft = NAN;
		p_report->distance_estimates[ap].phase_slope = NAN;
		p_report->distance_estimates[ap].rtt = NAN;
		p_report->distance_estimates[ap].best = NAN;

		if (m_is_tone_quality_ok(&m_tone_quality_indicators[ap][0], config->channel_map)) {
			p_report->tone_quality[ap] = CS_DE_TONE_QUALITY_OK;
		} else {
			p_report->tone_quality[ap] = CS_DE_TONE_QUALITY_BAD;
		}
	}
}
#endif

static cs_de_quality_t set_best_estimate(cs_de_dist_estimates_t *p_estimates_public)
{
	cs_de_quality_t data_quality = CS_DE_QUALITY_OK;

	if (isfinite(p_estimates_public->ifft)) {
		p_estimates_public->best = p_estimates_public->ifft;
	} else if (isfinite(p_estimates_public->phase_slope)) {
		p_estimates_public->best = p_estimates_public->phase_slope;
	} else if (isfinite(p_estimates_public->rtt)) {
		p_estimates_public->best = p_estimates_public->rtt;
	} else {
		data_quality = CS_DE_QUALITY_DO_NOT_USE;
		p_estimates_public->best = 0.0f;
	}

	return data_quality;
}

void cs_de_combined_iq_calculate(cs_de_iq_tones_t *cs_de_iq_tones,
				 float iq_tones_comb[CS_DE_NUM_CHANNELS * 2])
{
	float *i_local = cs_de_iq_tones->i_local;
	float *q_local = cs_de_iq_tones->q_local;
	float *i_remote = cs_de_iq_tones->i_remote;
	float *q_remote = cs_de_iq_tones->q_remote;

	for (uint32_t n = 0; n < CS_DE_NUM_CHANNELS; n++) {
		iq_tones_comb[2 * n] = i_local[n] * i_remote[n] - q_local[n] * q_remote[n];
		iq_tones_comb[2 * n + 1] = i_local[n] * q_remote[n] + i_remote[n] * q_local[n];
	}
}

cs_de_quality_t cs_de_calc(cs_de_report_t *p_report)
{
	cs_de_quality_t estimation_quality = CS_DE_QUALITY_DO_NOT_USE;

	float rtt_distance_m = cs_de_rtt(p_report->rtt_accumulated_half_ns, p_report->rtt_count);

	for (uint8_t ap = 0; ap < p_report->n_ap; ap++) {

		if (p_report->tone_quality[ap] == CS_DE_TONE_QUALITY_BAD) {
			continue;
		}

		memset(m_iq_scratch_mem, 0, sizeof(m_iq_scratch_mem));

		/* Combine init and refl IQ values and store in scratch mem. */
		cs_de_combined_iq_calculate(&p_report->iq_tones[ap], m_iq_scratch_mem);

		p_report->distance_estimates[ap].phase_slope = cs_de_phase_slope(m_iq_scratch_mem);

		p_report->distance_estimates[ap].ifft = cs_de_ifft(m_iq_scratch_mem);

		p_report->distance_estimates[ap].rtt = rtt_distance_m;

		if (set_best_estimate(&p_report->distance_estimates[ap]) == CS_DE_QUALITY_OK) {
			estimation_quality = CS_DE_QUALITY_OK;
		}
	}

	return estimation_quality;
}

float cs_de_rtt(int32_t rtt_accumulated_half_ns, uint8_t rtt_count)
{
	if (rtt_count > 0) {
		float rtt_avg_measured_ns = (rtt_accumulated_half_ns * 0.5f) / rtt_count;
		float tof_ns = rtt_avg_measured_ns / 2.0f;
		float rtt_distance_m = fmaxf(tof_ns * (SPEED_OF_LIGHT_M_PER_S / 1e9f), 0.0f);

		return rtt_distance_m;
	}

	return NAN;
}

float cs_de_phase_slope(float iq_tones_comb[CS_DE_NUM_CHANNELS * 2])
{
	float sum_i = 0;
	float sum_q = 0;
	float dist = 0;

	for (uint32_t n = 1; n < CS_DE_NUM_CHANNELS; n++) {
		sum_i += (iq_tones_comb[2 * n] * iq_tones_comb[2 * (n - 1)] +
			  iq_tones_comb[2 * n + 1] * iq_tones_comb[2 * (n - 1) + 1]);
		sum_q += (-iq_tones_comb[2 * n] * iq_tones_comb[2 * (n - 1) + 1] +
			  iq_tones_comb[2 * n + 1] * iq_tones_comb[2 * (n - 1)]);
	}

	dist = -(SPEED_OF_LIGHT_M_PER_S * atan2f(sum_q, sum_i)) / (4.0f * PI * CHANNEL_SPACING_HZ);

	if (dist < 0) {
		dist = NAN;
	}

	return dist;
}

static float calculate_ifft_peak_index_to_distance(int32_t peak_index,
						   const float ifft_mag[CONFIG_BT_CS_DE_NFFT_SIZE])
{
	/* Peak interpolation */
	float prompt = ifft_mag[peak_index];

	/* Find early and late magnitudes, if peak_index is at either first or last point in the
	 * IFFT, wrap around since the IFFT is periodic.
	 */
	float early = (peak_index != 0) ? ifft_mag[peak_index - 1]
					: ifft_mag[CONFIG_BT_CS_DE_NFFT_SIZE - 1];
	float late = (peak_index != (CONFIG_BT_CS_DE_NFFT_SIZE - 1)) ? ifft_mag[peak_index + 1]
								     : ifft_mag[0];
	/* Avoid interpolation of early, prompt and late if left null compensation has taken place.
	 */
	float t_hat = (prompt >= early && prompt >= late)
			      ? (late - early) / (4 * prompt - 2 * (early + late))
			      : 0.0f;

	float distance = ((peak_index + t_hat) * SPEED_OF_LIGHT_M_PER_S) /
			 (2.0f * CONFIG_BT_CS_DE_NFFT_SIZE * CHANNEL_SPACING_HZ);

	if (peak_index >= (CONFIG_BT_CS_DE_NFFT_SIZE - 2) || distance < 0.0f) {
		distance = NAN;
	}
	return distance;
}

static int32_t calculate_ifft_find_left_null(int32_t peak_index,
					     float ifft_mag[CONFIG_BT_CS_DE_NFFT_SIZE])
{
	int32_t left_null_index = peak_index;
	bool found_left_null = false;

	while (!found_left_null) {
		int32_t next_left_null_index =
			left_null_index == 0 ? CONFIG_BT_CS_DE_NFFT_SIZE - 1 : left_null_index - 1;
		/* This is a heuristic, probably non-optimal definition of a null. */
		if ((ifft_mag[left_null_index] * 2 > ifft_mag[peak_index] ||
		     ifft_mag[left_null_index] > 1.10f * ifft_mag[next_left_null_index]) &&
		    ifft_mag[left_null_index] * 10 > ifft_mag[peak_index] &&
		    next_left_null_index != peak_index) {
			left_null_index = next_left_null_index--;
		} else {
			found_left_null = true;
		}
	}
	return left_null_index;
}

static uint32_t calculate_distance_to_left_null(uint32_t peak_index, uint32_t left_null_index)
{
	return left_null_index > peak_index
		       ? (CONFIG_BT_CS_DE_NFFT_SIZE + peak_index - left_null_index)
		       : (peak_index - left_null_index);
}

static int32_t calculate_left_null_compensation_of_peak(int32_t peak_index,
							float ifft_mag[CONFIG_BT_CS_DE_NFFT_SIZE])
{
	int32_t compensated_peak_index = peak_index;
	int32_t left_null_index = calculate_ifft_find_left_null(peak_index, ifft_mag);
	uint32_t peak_to_null_distance =
		calculate_distance_to_left_null(peak_index, left_null_index);
	if (peak_to_null_distance > NORMAL_PEAK_TO_NULL) {
		if (left_null_index > peak_index) {
			compensated_peak_index = (left_null_index + NORMAL_PEAK_TO_NULL -
						  CONFIG_BT_CS_DE_NFFT_SIZE) > 0
							 ? (left_null_index + NORMAL_PEAK_TO_NULL -
							    CONFIG_BT_CS_DE_NFFT_SIZE)
							 : peak_index;
		} else {
			compensated_peak_index = left_null_index + NORMAL_PEAK_TO_NULL;
		}
	}
	return compensated_peak_index;
}

static void calculate_ifft_mag(float iq_tones_comb[2 * CONFIG_BT_CS_DE_NFFT_SIZE])
{
	/* This function calculates the magnitude of the IFFT of the input IQ values.
	 * Note that the result is written back to the input array.
	 * Also note that the input array is a complex array of size CONFIG_BT_CS_DE_NFFT_SIZE
	 * Odd indexes contain the real part and even indexes contain the imaginary part.
	 *
	 * To find the IFFT this the function uses FFT functions provided by the CMSIS-DSP library.
	 * To calculate the IFFT using FFT the following steps can be used:
	 *  1. Complex conjugate the input.
	 *  2. Perform the FFT.
	 *  3. Complex conjugate the output.
	 * Since we are interested in the magnitude of the IFFT, we can skip step 3.
	 * and directly calculate the magnitude of the output of step 2.
	 */

	/* Complex conjugate the input. */
	for (uint32_t i = 0; i < CS_DE_NUM_CHANNELS; i++) {
		iq_tones_comb[i * 2 + 1] = -iq_tones_comb[i * 2 + 1];
	}

	/* Perform the FFT. */
	#if CONFIG_BT_CS_DE_NFFT_SIZE == 512
		arm_cfft_f32(&arm_cfft_sR_f32_len512, iq_tones_comb, 0, 1);
	#elif CONFIG_BT_CS_DE_NFFT_SIZE == 1024
		arm_cfft_f32(&arm_cfft_sR_f32_len1024, iq_tones_comb, 0, 1);
	#elif CONFIG_BT_CS_DE_NFFT_SIZE == 2048
		arm_cfft_f32(&arm_cfft_sR_f32_len2048, iq_tones_comb, 0, 1);
	#else
	#error
	#endif

	/* Compute the magnitude of complex values in
	 * iq_tones_comb[0:2*CONFIG_BT_CS_DE_NFFT_SIZE - 1]
	 * and scale by 1/CONFIG_BT_CS_DE_NFFT_SIZE.
	 * Store output in iq_tones_comb[0:CONFIG_BT_CS_DE_NFFT_SIZE - 1]
	 */
	for (uint32_t n = 0; n < CONFIG_BT_CS_DE_NFFT_SIZE; n++) {
		float realIn = iq_tones_comb[2 * n] / CONFIG_BT_CS_DE_NFFT_SIZE;
		float imagIn = iq_tones_comb[(2 * n) + 1] / CONFIG_BT_CS_DE_NFFT_SIZE;

		arm_sqrt_f32((realIn * realIn) + (imagIn * imagIn), &iq_tones_comb[n]);
	}
}

static uint32_t find_ifft_peak_index(float ifft_mag[2 * CONFIG_BT_CS_DE_NFFT_SIZE])
{
	/* This function tries to find the peak index of the input IFFT magnitude.
	 *
	 * The function uses the following approach:
	 *  1. Find the index of the strongest peak,
	 *     corresponding to the maximum value in the IFFT magnitude.
	 *  2. Search for strong peaks closer than the max peak.
	 *  3. When applicable: Compensate peak based on left null location.
	 */
	uint32_t ifft_mag_max_index;
	float ifft_mag_max;

	arm_max_f32(ifft_mag, CONFIG_BT_CS_DE_NFFT_SIZE, &ifft_mag_max, &ifft_mag_max_index);

	/* Search for strong peaks closer than the max value. */
	uint32_t nw = CONFIG_BT_CS_DE_NFFT_SIZE - 2;
	uint32_t nw_next = CONFIG_BT_CS_DE_NFFT_SIZE - 1;
	uint32_t max_search_index = ifft_mag_max_index;
	bool short_path_found = false;
	bool first_rise_found = false;
	uint32_t shortest_path_idx = ifft_mag_max_index;

	while (nw != max_search_index && !short_path_found) {
		if (ifft_mag[nw_next] < ifft_mag[nw]) {
			/* Peak found */
			if (2.5f * ifft_mag[nw] > ifft_mag_max && first_rise_found) {
				/* New peak found */
				shortest_path_idx = nw;
				short_path_found = true;
			}
		} else {
			first_rise_found = true;
		}
		nw = nw_next;
		nw_next = (nw_next + 1) % CONFIG_BT_CS_DE_NFFT_SIZE;
	}

	uint32_t compensated_peak_index = shortest_path_idx;

	if (compensated_peak_index < CONFIG_BT_CS_DE_NFFT_SIZE - 2) {
		compensated_peak_index =
			calculate_left_null_compensation_of_peak(shortest_path_idx, ifft_mag);
	}

	return compensated_peak_index;
}

float cs_de_ifft(float iq_tones_comb[2 * CONFIG_BT_CS_DE_NFFT_SIZE])
{
	/* This function calculates a distance estimate
	 * based on the IFFT magnitude of the input IQ values
	 *
	 * To do this the function uses the following steps:
	 *  1. Calculate the IFFT magnitude of the input IQ values.
	 *  2. Find index of the peak in the IFFT magnitude which is believed
	 *     to correspond to the path with the shortest propagattion time.
	 *  3. Convert the peak index to a distance estimate.
	 */
	calculate_ifft_mag(iq_tones_comb);

	/* The input IQ values are overwritten with the IFFT magnitude. */
	float *ifft_mag = iq_tones_comb;

	uint32_t ifft_peak_index = find_ifft_peak_index(ifft_mag);

	return calculate_ifft_peak_index_to_distance(ifft_peak_index, ifft_mag);
}
