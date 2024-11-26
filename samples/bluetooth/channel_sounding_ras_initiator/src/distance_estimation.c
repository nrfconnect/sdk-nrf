/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Channel Sounding distance estimation for Ranging Requestor
 */

#include "zephyr/bluetooth/hci_types.h"
#include <math.h>
#include <zephyr/bluetooth/cs.h>
#include <bluetooth/services/ras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_main, LOG_LEVEL_INF);

#define CS_FREQUENCY_MHZ(ch)	(2402u + 1u * (ch))
#define CS_FREQUENCY_HZ(ch)	(CS_FREQUENCY_MHZ(ch) * 1000000.0f)
#define SPEED_OF_LIGHT_M_PER_S	(299792458.0f)
#define SPEED_OF_LIGHT_NM_PER_S (SPEED_OF_LIGHT_M_PER_S / 1000000000.0f)
#define PI			3.14159265358979323846f
#define MAX_NUM_SAMPLES		256

struct iq_sample_and_channel {
	bool failed;
	uint8_t channel;
	uint8_t antenna_permutation;
	struct bt_le_cs_iq_sample local_iq_sample;
	struct bt_le_cs_iq_sample peer_iq_sample;
};

struct rtt_timing {
	bool failed;
	int16_t toa_tod_initiator;
	int16_t tod_toa_reflector;
};

static struct iq_sample_and_channel mode_2_data[MAX_NUM_SAMPLES];
static struct rtt_timing mode_1_data[MAX_NUM_SAMPLES];

struct processing_context {
	uint8_t mode_1_data_index;
	uint8_t mode_2_data_index;
	uint8_t n_ap;
	enum bt_conn_le_cs_role role;
};

static void calc_complex_product(int32_t z_a_real, int32_t z_a_imag, int32_t z_b_real,
				 int32_t z_b_imag, int32_t *z_out_real, int32_t *z_out_imag)
{
	*z_out_real = z_a_real * z_b_real - z_a_imag * z_b_imag;
	*z_out_imag = z_a_real * z_b_imag + z_a_imag * z_b_real;
}

static float linear_regression(float *x_values, float *y_values, uint8_t n_samples)
{
	if (n_samples == 0) {
		return 0.0;
	}

	/* Estimates b in y = a + b x */

	float y_mean = 0.0;
	float x_mean = 0.0;

	for (uint8_t i = 0; i < n_samples; i++) {
		y_mean += (y_values[i] - y_mean) / (i + 1);
		x_mean += (x_values[i] - x_mean) / (i + 1);
	}

	float b_est_upper = 0.0;
	float b_est_lower = 0.0;

	for (uint8_t i = 0; i < n_samples; i++) {
		b_est_upper += (x_values[i] - x_mean) * (y_values[i] - y_mean);
		b_est_lower += (x_values[i] - x_mean) * (x_values[i] - x_mean);
	}

	return b_est_upper / b_est_lower;
}

static void bubblesort_2(float *array1, float *array2, uint16_t len)
{
	bool swapped;
	float temp;

	for (uint16_t i = 0; i < len - 1; i++) {
		swapped = false;
		for (uint16_t j = 0; j < len - i - 1; j++) {
			if (array1[j] > array1[j + 1]) {
				temp = array1[j];
				array1[j] = array1[j + 1];
				array1[j + 1] = temp;
				temp = array2[j];
				array2[j] = array2[j + 1];
				array2[j + 1] = temp;
				swapped = true;
			}
		}

		if (!swapped) {
			break;
		}
	}
}

static float estimate_distance_using_phase_slope(struct iq_sample_and_channel *data, uint8_t len)
{
	int32_t combined_i;
	int32_t combined_q;
	uint16_t num_angles = 0;
	static float theta[MAX_NUM_SAMPLES];
	static float frequencies[MAX_NUM_SAMPLES];

	for (uint8_t i = 0; i < len; i++) {
		if (!data[i].failed) {
			calc_complex_product(data[i].local_iq_sample.i, data[i].local_iq_sample.q,
					     data[i].peer_iq_sample.i, data[i].peer_iq_sample.q,
					     &combined_i, &combined_q);

			theta[num_angles] = atan2(1.0 * combined_q, 1.0 * combined_i);
			frequencies[num_angles] = 1.0 * CS_FREQUENCY_MHZ(data[i].channel);
			num_angles++;
		}
	}

	if (num_angles < 2) {
		return 0.0;
	}

	/* Sort phases by tone frequency */
	bubblesort_2(frequencies, theta, num_angles);

	/* One-dimensional phase unwrapping */
	for (uint8_t i = 1; i < num_angles; i++) {
		float difference = theta[i] - theta[i - 1];

		if (difference > PI) {
			for (uint8_t j = i; j < num_angles; j++) {
				theta[j] -= 2.0f * PI;
			}
		} else if (difference < -PI) {
			for (uint8_t j = i; j < num_angles; j++) {
				theta[j] += 2.0f * PI;
			}
		}
	}

	float phase_slope = linear_regression(frequencies, theta, num_angles);

	float distance = -phase_slope * (SPEED_OF_LIGHT_M_PER_S / (4 * PI));

	return distance / 1000000.0f; /* Scale to meters. */
}

static float estimate_distance_using_time_of_flight(uint8_t n_samples)
{
	float tof;
	float tof_mean = 0.0;

	/* Cumulative Moving Average */
	for (uint8_t i = 0; i < n_samples; i++) {
		if (!mode_1_data[i].failed) {
			tof = (mode_1_data[i].toa_tod_initiator -
			       mode_1_data[i].tod_toa_reflector) /
			      2;
			tof_mean += (tof - tof_mean) / (i + 1);
		}
	}

	float tof_mean_ns = tof_mean / 2.0f;

	return tof_mean_ns * SPEED_OF_LIGHT_NM_PER_S;
}

static bool process_step_data(struct bt_le_cs_subevent_step *local_step,
			      struct bt_le_cs_subevent_step *peer_step, void *user_data)
{
	struct processing_context *context = (struct processing_context *)user_data;

	if (local_step->mode == BT_CONN_LE_CS_MAIN_MODE_2) {
		struct bt_hci_le_cs_step_data_mode_2 *local_step_data =
			(struct bt_hci_le_cs_step_data_mode_2 *)local_step->data;
		struct bt_hci_le_cs_step_data_mode_2 *peer_step_data =
			(struct bt_hci_le_cs_step_data_mode_2 *)peer_step->data;

		for (uint8_t i = 0; i < (context->n_ap + 1); i++) {
			if (local_step_data->tone_info[i].extension_indicator !=
				    BT_HCI_LE_CS_NOT_TONE_EXT_SLOT ||
			    peer_step_data->tone_info[i].extension_indicator !=
				    BT_HCI_LE_CS_NOT_TONE_EXT_SLOT) {
				continue;
			}

			mode_2_data[context->mode_2_data_index].channel = local_step->channel;
			mode_2_data[context->mode_2_data_index].antenna_permutation =
				local_step_data->antenna_permutation_index;
			mode_2_data[context->mode_2_data_index].local_iq_sample =
				bt_le_cs_parse_pct(
					local_step_data->tone_info[i].phase_correction_term);
			mode_2_data[context->mode_2_data_index].peer_iq_sample = bt_le_cs_parse_pct(
				peer_step_data->tone_info[i].phase_correction_term);

			if (local_step_data->tone_info[i].quality_indicator ==
				    BT_HCI_LE_CS_TONE_QUALITY_LOW ||
			    local_step_data->tone_info[i].quality_indicator ==
				    BT_HCI_LE_CS_TONE_QUALITY_UNAVAILABLE ||
			    peer_step_data->tone_info[i].quality_indicator ==
				    BT_HCI_LE_CS_TONE_QUALITY_LOW ||
			    peer_step_data->tone_info[i].quality_indicator ==
				    BT_HCI_LE_CS_TONE_QUALITY_UNAVAILABLE) {
				mode_2_data[context->mode_2_data_index].failed = true;
			}

			context->mode_2_data_index++;
		}
	} else if (local_step->mode == BT_HCI_OP_LE_CS_MAIN_MODE_1) {
		struct bt_hci_le_cs_step_data_mode_1 *local_step_data =
			(struct bt_hci_le_cs_step_data_mode_1 *)local_step->data;
		struct bt_hci_le_cs_step_data_mode_1 *peer_step_data =
			(struct bt_hci_le_cs_step_data_mode_1 *)peer_step->data;

		if (local_step_data->packet_quality_aa_check !=
			    BT_HCI_LE_CS_PACKET_QUALITY_AA_CHECK_SUCCESSFUL ||
		    local_step_data->packet_rssi == BT_HCI_LE_CS_PACKET_RSSI_NOT_AVAILABLE ||
		    local_step_data->tod_toa_reflector ==
			    BT_HCI_LE_CS_TIME_DIFFERENCE_NOT_AVAILABLE ||
		    peer_step_data->packet_quality_aa_check !=
			    BT_HCI_LE_CS_PACKET_QUALITY_AA_CHECK_SUCCESSFUL ||
		    peer_step_data->packet_rssi == BT_HCI_LE_CS_PACKET_RSSI_NOT_AVAILABLE ||
		    peer_step_data->tod_toa_reflector ==
			    BT_HCI_LE_CS_TIME_DIFFERENCE_NOT_AVAILABLE) {
			mode_1_data[context->mode_1_data_index].failed = true;
		}

		if (context->role == BT_CONN_LE_CS_ROLE_INITIATOR) {
			mode_1_data[context->mode_1_data_index].toa_tod_initiator =
				local_step_data->toa_tod_initiator;
			mode_1_data[context->mode_1_data_index].tod_toa_reflector =
				peer_step_data->tod_toa_reflector;
		} else if (context->role == BT_CONN_LE_CS_ROLE_REFLECTOR) {
			mode_1_data[context->mode_1_data_index].tod_toa_reflector =
				local_step_data->tod_toa_reflector;
			mode_1_data[context->mode_1_data_index].toa_tod_initiator =
				peer_step_data->toa_tod_initiator;
		}

		context->mode_1_data_index++;
	}

	return true;
}

void estimate_distance(struct net_buf_simple *local_steps, struct net_buf_simple *peer_steps,
		       uint8_t n_ap, enum bt_conn_le_cs_role role)
{
	struct processing_context context = {
		.mode_1_data_index = 0,
		.mode_2_data_index = 0,
		.n_ap = n_ap,
		.role = role,
	};

	memset(mode_1_data, 0, sizeof(mode_1_data));
	memset(mode_2_data, 0, sizeof(mode_2_data));

	bt_ras_rreq_rd_subevent_data_parse(peer_steps, local_steps, context.role, NULL,
					   process_step_data, &context);

	float phase_slope_based_distance =
		estimate_distance_using_phase_slope(mode_2_data, context.mode_2_data_index);

	float rtt_based_distance =
		estimate_distance_using_time_of_flight(context.mode_1_data_index);

	if (rtt_based_distance == 0.0f && phase_slope_based_distance == 0.0f) {
		LOG_INF("A reliable distance estimate could not be computed.");
	} else {
		LOG_INF("Estimated distance to reflector:");
	}

	if (rtt_based_distance != 0.0f) {
		LOG_INF("- Round-Trip Timing method: %f meters (derived from %d samples)",
			(double)rtt_based_distance, context.mode_1_data_index);
	}
	if (phase_slope_based_distance != 0.0f) {
		LOG_INF("- Phase-Based Ranging method: %f meters (derived from %d samples)",
			(double)phase_slope_based_distance, context.mode_2_data_index);
	}
}
