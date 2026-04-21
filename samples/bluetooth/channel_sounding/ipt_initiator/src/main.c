/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Channel Sounding IPT Initiator sample
 */

#include <math.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/scan.h>
#include <bluetooth/cs_de.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

#define CON_STATUS_LED DK_LED1
#define CS_CONFIG_ID 0
#define NUM_MODE_0_STEPS 3
#define REFLECTOR_NAME "Nordic CS IPT Reflector"

#define CHANNEL_INDEX_OFFSET (2)

static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_security, 0, 1);
static K_SEM_DEFINE(sem_read_all_remote_feat_complete, 0, 1);
static K_SEM_DEFINE(sem_subevent_results_parsed, 0, 1);
static K_SEM_DEFINE(sem_distance_estimate_updated, 1, 1);

static struct bt_conn *connection;
static struct bt_conn_le_cs_config cs_config;

/* Store local initiator IQs in this union.
  The size is based on requirements of cs_de_ifft.
  The scratch memory is used for the ifft operation. */
static union {
	struct {
		float i;
		float q;
	} values[CS_DE_NUM_CHANNELS];
	float scratch_mem[2 * CONFIG_BT_CS_DE_NFFT_SIZE];
} iq;

/* --- Simple constant-velocity Kalman filter for distance tracking --- *
 *
 * State vector: x = [d, v]^T  (distance [m], velocity [m/s])
 * Measurement : z = d         (distance from cs_de_ifft, in meters)
 *
 * Transition   F = [[1, dt],
 *                   [0,  1]]
 * Measurement  H = [1, 0]
 *
 * Process noise is modeled as continuous white-noise acceleration with
 * spectral density sigma_a^2, giving
 *   Q = sigma_a^2 * [[dt^4/4, dt^3/2],
 *                    [dt^3/2, dt^2  ]]
 *
 * R is the scalar measurement-noise variance.
 *
 * Tune KF_SIGMA_A and KF_R_MEAS for the expected motion / sensor noise.
 */
#define KF_SIGMA_A   1.0f   /* [m/s^2] expected acceleration std-dev */
#define KF_R_MEAS    0.25f  /* [m^2] distance measurement variance (sigma = 0.5 m) */
#define KF_P_INIT    10.0f  /* initial state covariance (large = trust first measurement) */

static bool  kf_initialized;
static float kf_x[2];       /* state: [distance, velocity] */
static float kf_P[2][2];    /* covariance */

/* Run one predict + update cycle.
 * z  : new distance measurement [m]
 * dt : time since previous measurement [s] (ignored on first call)
 * Returns the filtered distance estimate.
 */
static float kalman_update(float z, float dt)
{
	if (!kf_initialized || dt <= 0.0f) {
		/* Initialize state directly from the first measurement. */
		kf_x[0] = z;
		kf_x[1] = 0.0f;
		kf_P[0][0] = KF_P_INIT; kf_P[0][1] = 0.0f;
		kf_P[1][0] = 0.0f;      kf_P[1][1] = KF_P_INIT;
		kf_initialized = true;
		return kf_x[0];
	}

	/* ---------------- Predict ---------------- */
	/* x = F x */
	kf_x[0] += kf_x[1] * dt;
	/* x[1] unchanged */

	/* P = F P F^T + Q */
	const float p00 = kf_P[0][0];
	const float p01 = kf_P[0][1];
	const float p10 = kf_P[1][0];
	const float p11 = kf_P[1][1];

	const float dt2 = dt * dt;
	const float dt3 = dt2 * dt;
	const float dt4 = dt2 * dt2;
	const float sa2 = KF_SIGMA_A * KF_SIGMA_A;

	const float q00 = sa2 * dt4 * 0.25f;
	const float q01 = sa2 * dt3 * 0.5f;
	const float q11 = sa2 * dt2;

	/* F P F^T for F = [[1,dt],[0,1]] */
	kf_P[0][0] = p00 + dt * (p10 + p01) + dt2 * p11 + q00;
	kf_P[0][1] = p01 + dt * p11 + q01;
	kf_P[1][0] = p10 + dt * p11 + q01;
	kf_P[1][1] = p11 + q11;

	/* ---------------- Update (H = [1, 0]) ---------------- */
	const float y = z - kf_x[0];                       /* innovation */
	const float S = kf_P[0][0] + KF_R_MEAS;            /* innovation covariance */
	const float K0 = kf_P[0][0] / S;                   /* Kalman gain */
	const float K1 = kf_P[1][0] / S;

	kf_x[0] += K0 * y;
	kf_x[1] += K1 * y;

	/* P = (I - K H) P */
	const float np00 = (1.0f - K0) * kf_P[0][0];
	const float np01 = (1.0f - K0) * kf_P[0][1];
	const float np10 = kf_P[1][0] - K1 * kf_P[0][0];
	const float np11 = kf_P[1][1] - K1 * kf_P[0][1];

	kf_P[0][0] = np00;
	kf_P[0][1] = np01;
	kf_P[1][0] = np10;
	kf_P[1][1] = np11;

	return kf_x[0];
}

static void distance_estimates_update(void)
{
	static int64_t prev_ts = 0;

	int64_t now = k_uptime_get();
	int64_t delta = 0;
	if (prev_ts != 0) {
		delta = now - prev_ts;
	}

	for (uint8_t i = 0; i < CS_DE_NUM_CHANNELS; i++) {
		LOG_DBG("iq[%d]: %.1f + j * %.1f", i, (double)iq.values[i].i, (double)iq.values[i].q);
	}

	float distance_ifft = cs_de_ifft(iq.scratch_mem);

	if (isfinite(distance_ifft)) {
		float dt_s = (float)delta * 1e-3f;
		float distance_kf = kalman_update(distance_ifft, dt_s);

	// LOG_ERR("distance_ifft: %.2f  kf: %.2f  v: %.2f m/s  dt: %lld ms",
	// 	(double)distance_ifft, (double)distance_kf,
	// 	(double)kf_x[1], delta);
		LOG_ERR("kf: %.2f", (double)distance_kf);
		prev_ts = now;
	}
	k_sem_give(&sem_distance_estimate_updated);
}

static void pcts_parse(uint8_t channel_index, struct bt_hci_le_cs_step_data_tone_info *local_tone_info)
{

	// if (local_tone_info[0].quality_indicator != BT_HCI_LE_CS_TONE_QUALITY_HIGH) {
		// LOG_ERR("bad quality");
		// return;
	// }

	struct bt_le_cs_iq_sample local_iq =
		bt_le_cs_parse_pct(local_tone_info[0].phase_correction_term);

	iq.values[channel_index - CHANNEL_INDEX_OFFSET].i = local_iq.i;
	iq.values[channel_index - CHANNEL_INDEX_OFFSET].q = local_iq.q;
}

static void subevent_steps_parse(struct bt_conn_le_cs_subevent_result *result)
{
	for (uint8_t i = 0; i < result->header.num_steps_reported; i++) {
		if (result->step_data_buf->len < 3) {
			LOG_WRN("Local step data appears malformed.");
			return;
		}
		struct bt_le_cs_subevent_step local_step = {0};
		local_step.mode = net_buf_simple_pull_u8(result->step_data_buf);
		local_step.channel = net_buf_simple_pull_u8(result->step_data_buf);
		local_step.data_len = net_buf_simple_pull_u8(result->step_data_buf);

		if (local_step.data_len == 0) {
			LOG_WRN("Encountered zero-length step data.");
			return;
		}
		if (local_step.data_len > result->step_data_buf->len) {
			LOG_WRN("Local step data appears malformed.");
			return;
		}

		local_step.data = result->step_data_buf->data;

		if (local_step.mode == BT_HCI_OP_LE_CS_MAIN_MODE_2) {
			struct bt_hci_le_cs_step_data_mode_2 *local_step_data =
				(struct bt_hci_le_cs_step_data_mode_2 *)local_step.data;
			pcts_parse(local_step.channel, local_step_data->tone_info);
		}

		net_buf_simple_pull(result->step_data_buf, local_step.data_len);
	}
}

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	static int64_t prev_ts = 0;
	static uint32_t prev_procedure_counter = UINT16_MAX + 1;

	int64_t now = k_uptime_get();
	int64_t delta = 0;
	if (prev_ts != 0) {
		delta = now - prev_ts;
	}
	prev_ts = now;

	LOG_INF("Subevent result callback for procedure counter %u procedure done status "
		"%u subevent done status %u, number of steps reported %u, time delta since "
		"last call: %lld ms",
		result->header.procedure_counter, result->header.procedure_done_status,
		result->header.subevent_done_status, result->header.num_steps_reported,
		delta);

	if (result->header.procedure_counter != prev_procedure_counter) {
		LOG_DBG("Procedure counter %u is the same as the previous one. This means there are multiple"
			       "CS subevents per CS procedure.", result->header.procedure_counter);
		k_sem_take(&sem_distance_estimate_updated, K_FOREVER);
		memset(iq.scratch_mem, 0, sizeof(iq.scratch_mem));
	}
	prev_procedure_counter = result->header.procedure_counter;

	subevent_steps_parse(result);
	/*TODO: How do we handle aborted PROCEDURES / SUBEVENTS ?? */

	/* This means all the steps from the procedure have been parsed and distance estimation can run. */
	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		k_sem_give(&sem_subevent_results_parsed);
	}

	return;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
		return;
	}

	LOG_INF("Security changed: %s level %u", addr, level);
	k_sem_give(&sem_security);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected to %s (err 0x%02X)", addr, err);

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	} else {
		connection = bt_conn_ref(conn);

		k_sem_give(&sem_connected);

		dk_set_led_on(CON_STATUS_LED);
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02X)", reason);

	bt_conn_unref(conn);
	connection = NULL;
	dk_set_led_off(CON_STATUS_LED);

	sys_reboot(SYS_REBOOT_COLD);
}

static void remote_capabilities_cb(struct bt_conn *conn,
				   uint8_t status,
				   struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		static const char *rtt_aa_str[] = {"not supported", "10 ns", "150 ns",
						   "invalid"};
		static const char *rtt_snd_str[] = {"not supported", "10 ns", "150 ns",
						    "invalid"};
		static const char *rtt_rand_str[] = {"not supported", "10 ns", "150 ns",
						     "invalid"};

		unsigned aa_idx = MIN((unsigned)params->rtt_aa_only_precision, 3U);
		unsigned snd_idx = MIN((unsigned)params->rtt_sounding_precision, 3U);
		unsigned rand_idx = MIN((unsigned)params->rtt_random_payload_precision, 3U);

		LOG_INF("CS capability exchange completed.\n"
			"Remote CS capabilities:\n"
			" - num_config_supported: %u\n"
			" - max_consecutive_procedures_supported: %u\n"
			" - num_antennas_supported: %u\n"
			" - max_antenna_paths_supported: %u\n"
			" - initiator_supported: %s\n"
			" - reflector_supported: %s\n"
			" - mode_3_supported: %s\n"
			" - rtt_aa_only_precision: %s (%u)\n"
			" - rtt_sounding_precision: %s (%u)\n"
			" - rtt_random_payload_precision: %s (%u)\n"
			" - rtt_aa_only_n: %u\n"
			" - rtt_sounding_n: %u\n"
			" - rtt_random_payload_n: %u\n"
			" - phase_based_nadm_sounding_supported: %s\n"
			" - phase_based_nadm_random_supported: %s\n"
			" - cs_sync_2m_phy_supported: %s\n"
			" - cs_sync_2m_2bt_phy_supported: %s\n"
			" - cs_without_fae_supported: %s\n"
			" - chsel_alg_3c_supported: %s\n"
			" - pbr_from_rtt_sounding_seq_supported: %s\n"
			" - t_ip1_times_supported: 0x%04x\n"
			" - t_ip2_times_supported: 0x%04x\n"
			" - t_fcs_times_supported: 0x%04x\n"
			" - t_pm_times_supported: 0x%04x\n"
			" - t_sw_time: %u us\n"
			" - tx_snr_capability: 0x%02x\n"
			" - t_ip2_ipt_times_supported: 0x%04x\n"
			" - t_sw_ipt_time_supported: %u us\n",
			params->num_config_supported,
			params->max_consecutive_procedures_supported,
			params->num_antennas_supported, params->max_antenna_paths_supported,
			params->initiator_supported ? "yes" : "no",
			params->reflector_supported ? "yes" : "no",
			params->mode_3_supported ? "yes" : "no", rtt_aa_str[aa_idx],
			(unsigned)params->rtt_aa_only_precision, rtt_snd_str[snd_idx],
			(unsigned)params->rtt_sounding_precision, rtt_rand_str[rand_idx],
			(unsigned)params->rtt_random_payload_precision, params->rtt_aa_only_n,
			params->rtt_sounding_n, params->rtt_random_payload_n,
			params->phase_based_nadm_sounding_supported ? "yes" : "no",
			params->phase_based_nadm_random_supported ? "yes" : "no",
			params->cs_sync_2m_phy_supported ? "yes" : "no",
			params->cs_sync_2m_2bt_phy_supported ? "yes" : "no",
			params->cs_without_fae_supported ? "yes" : "no",
			params->chsel_alg_3c_supported ? "yes" : "no",
			params->pbr_from_rtt_sounding_seq_supported ? "yes" : "no",
			params->t_ip1_times_supported, params->t_ip2_times_supported,
			params->t_fcs_times_supported, params->t_pm_times_supported,
			params->t_sw_time, params->tx_snr_capability,
			params->t_ip2_ipt_times_supported, params->t_sw_ipt_time_supported);

		k_sem_give(&sem_remote_capabilities_obtained);
	} else {
		LOG_WRN("CS capability exchange failed. (HCI status 0x%02x)", status);
	}
}

static void config_create_cb(struct bt_conn *conn, uint8_t status,
			     struct bt_conn_le_cs_config *config)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		cs_config = *config;

		const char *mode_str[5] = {"Unused", "1 (RTT)", "2 (PBR)", "3 (RTT + PBR)",
					   "Invalid"};
		const char *role_str[3] = {"Initiator", "Reflector", "Invalid"};
		const char *rtt_type_str[8] = {
			"AA only",	 "32-bit sounding", "96-bit sounding", "32-bit random",
			"64-bit random", "96-bit random",   "128-bit random",  "Invalid"};
		const char *phy_str[4] = {"Invalid", "LE 1M PHY", "LE 2M PHY", "LE 2M 2BT PHY"};
		const char *chsel_type_str[3] = {"Algorithm #3b", "Algorithm #3c", "Invalid"};
		const char *ch3c_shape_str[3] = {"Hat shape", "X shape", "Invalid"};

		uint8_t mode_idx = config->mode > 0 && config->mode < 4 ? config->mode : 4;
		uint8_t role_idx = MIN(config->role, 2);
		uint8_t rtt_type_idx = MIN(config->rtt_type, 7);
		uint8_t phy_idx = config->cs_sync_phy > 0 && config->cs_sync_phy < 4
					  ? config->cs_sync_phy
					  : 0;
		uint8_t chsel_type_idx = MIN(config->channel_selection_type, 2);
		uint8_t ch3c_shape_idx = MIN(config->ch3c_shape, 2);

		LOG_INF("CS config creation complete.\n"
			" - id: %u\n"
			" - mode: %s\n"
			" - min_main_mode_steps: %u\n"
			" - max_main_mode_steps: %u\n"
			" - main_mode_repetition: %u\n"
			" - mode_0_steps: %u\n"
			" - role: %s\n"
			" - rtt_type: %s\n"
			" - cs_sync_phy: %s\n"
			" - channel_map_repetition: %u\n"
			" - channel_selection_type: %s\n"
			" - ch3c_shape: %s\n"
			" - ch3c_jump: %u\n"
			" - t_ip1_time_us: %u\n"
			" - t_ip2_time_us: %u\n"
			" - t_fcs_time_us: %u\n"
			" - t_pm_time_us: %u\n"
			" - channel_map: 0x%08X%08X%04X\n",
			config->id, mode_str[mode_idx],
			config->min_main_mode_steps, config->max_main_mode_steps,
			config->main_mode_repetition, config->mode_0_steps, role_str[role_idx],
			rtt_type_str[rtt_type_idx], phy_str[phy_idx],
			config->channel_map_repetition, chsel_type_str[chsel_type_idx],
			ch3c_shape_str[ch3c_shape_idx], config->ch3c_jump, config->t_ip1_time_us,
			config->t_ip2_time_us, config->t_fcs_time_us, config->t_pm_time_us,
			sys_get_le32(&config->channel_map[6]),
			sys_get_le32(&config->channel_map[2]),
			sys_get_le16(&config->channel_map[0]));

		k_sem_give(&sem_config_created);
	} else {
		LOG_WRN("CS config creation failed. (HCI status 0x%02x)", status);
	}
}

static void security_enable_cb(struct bt_conn *conn, uint8_t status)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("CS security enabled.");
		k_sem_give(&sem_cs_security_enabled);
	} else {
		LOG_WRN("CS security enable failed. (HCI status 0x%02x)", status);
	}
}

static void procedure_enable_cb(struct bt_conn *conn,
				uint8_t status,
				struct bt_conn_le_cs_procedure_enable_complete *params)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		if (params->state == 1) {
			LOG_INF("CS procedures enabled:\n"
				" - config ID: %u\n"
				" - antenna configuration index: %u\n"
				" - TX power: %d dbm\n"
				" - subevent length: %u us\n"
				" - subevents per event: %u\n"
				" - subevent interval: %u\n"
				" - event interval: %u\n"
				" - procedure interval: %u\n"
				" - procedure count: %u\n"
				" - maximum procedure length: %u",
				params->config_id, params->tone_antenna_config_selection,
				params->selected_tx_power, params->subevent_len,
				params->subevents_per_event, params->subevent_interval,
				params->event_interval, params->procedure_interval,
				params->procedure_count, params->max_procedure_len);
		} else {
			LOG_INF("CS procedures disabled.");
		}
	} else {
		LOG_WRN("CS procedures enable failed. (HCI status 0x%02x)", status);
	}
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	int err;

	LOG_INF("Connecting failed, restarting scanning");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Failed to restart scanning (err %i)", err);
		return;
	}
}

static void scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn)
{
	LOG_INF("Connecting");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

static int scan_init(struct bt_scan_init_param *p_param)
{
	int err;

	bt_scan_init(p_param);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, REFLECTOR_NAME);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	return 0;
}

void read_all_remote_feat_complete(struct bt_conn *conn,
				   const struct bt_conn_le_read_all_remote_feat_complete *params)
{
	if (params->status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("Read all remote features complete");
		k_sem_give(&sem_read_all_remote_feat_complete);
	} else {
		LOG_WRN("Read all remote features failed (HCI status 0x%02x)", params->status);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed,
	.le_cs_read_remote_capabilities_complete = remote_capabilities_cb,
	.le_cs_config_complete = config_create_cb,
	.le_cs_security_enable_complete = security_enable_cb,
	.le_cs_procedure_enable_complete = procedure_enable_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
	.read_all_remote_feat_complete = read_all_remote_feat_complete,
};

static void cs_config_get(struct bt_le_cs_create_config_params *config_params)
{
	config_params->id = CS_CONFIG_ID;
	config_params->mode = BT_CONN_LE_CS_MAIN_MODE_2_NO_SUB_MODE;
	config_params->min_main_mode_steps = 2;
	config_params->max_main_mode_steps = 5;
	config_params->main_mode_repetition = 0;
	config_params->mode_0_steps = NUM_MODE_0_STEPS;
	config_params->role = BT_CONN_LE_CS_ROLE_INITIATOR;
	config_params->rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
	config_params->cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
	config_params->channel_map_repetition = 1;
	config_params->channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
	config_params->ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
	config_params->ch3c_jump = 2;
	config_params->cs_enhancements_1 = 1;
}

int main(void)
{
	int err;

	LOG_INF("Starting Channel Sounding IPT Initiator Sample");

	dk_leds_init();

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	struct bt_scan_init_param scan_params = {
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM(6, 6, 0, BT_GAP_MS_TO_CONN_TIMEOUT(4000)),
		.connect_if_match = 1
	};

	err = scan_init(&scan_params);
	if (err) {
		LOG_ERR("Scan init failed (err %d)", err);
		return 0;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %i)", err);
		return 0;
	}

	k_sem_take(&sem_connected, K_FOREVER);

	err = bt_conn_set_security(connection, BT_SECURITY_L2);
	if (err) {
		LOG_ERR("Failed to encrypt connection (err %d)", err);
		return 0;
	}
	k_sem_take(&sem_security, K_FOREVER);

	/* Needed for IPT. */
	const uint8_t pages_requested = 10;
	err = bt_conn_le_read_all_remote_features(connection, pages_requested);

	if (err != 0) {
		LOG_ERR("bt_conn_le_read_all_remote_features returned error %d", err);
		return 0;
	}
	k_sem_take(&sem_read_all_remote_feat_complete, K_FOREVER);

	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = true,
		.enable_reflector_role = false,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	err = bt_le_cs_set_default_settings(connection, &default_settings);
	if (err) {
		LOG_ERR("Failed to configure default CS settings (err %d)", err);
		return 0;
	}

	err = bt_le_cs_read_remote_supported_capabilities(connection);
	if (err) {
		LOG_ERR("Failed to exchange CS capabilities (err %d)", err);
		return 0;
	}

	k_sem_take(&sem_remote_capabilities_obtained, K_FOREVER);

	struct bt_le_cs_create_config_params config_params;

	cs_config_get(&config_params);

	bt_le_cs_set_valid_chmap_bits(config_params.channel_map);

	err = bt_le_cs_create_config(connection, &config_params,
				     BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
	if (err) {
		LOG_ERR("Failed to create CS config (err %d)", err);
		return 0;
	}
	k_sem_take(&sem_config_created, K_FOREVER);

	err = bt_le_cs_security_enable(connection);
	if (err) {
		LOG_ERR("Failed to start CS Security (err %d)", err);
		return 0;
	}

	k_sem_take(&sem_cs_security_enabled, K_FOREVER);

	/* scale factor of conn_interval units to proc_interval units is 1.25/0.625 = 2 */
	const uint16_t acl_interval_in_proc_interval_units =
		scan_params.conn_param->interval_max * 2;
	uint16_t desired_procedure_interval = 200;
	uint16_t desired_max_procedure_length =
		acl_interval_in_proc_interval_units * (desired_procedure_interval - 1);

	const struct bt_le_cs_set_procedure_parameters_param procedure_params = {
		.config_id = CS_CONFIG_ID,
		.max_procedure_len = desired_max_procedure_length,
		.min_procedure_interval = desired_procedure_interval,
		.max_procedure_interval = desired_procedure_interval,
		.max_procedure_count = 0,
		.min_subevent_len = 11000,
		.max_subevent_len = 11000,
		.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A1_B1,
		.phy = BT_LE_CS_PROCEDURE_PHY_2M,
		.tx_power_delta = 0x80,
		.preferred_peer_antenna = BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_1,
		.snr_control_initiator = BT_LE_CS_SNR_CONTROL_NOT_USED,
		.snr_control_reflector = BT_LE_CS_SNR_CONTROL_NOT_USED,
	};

	err = bt_le_cs_set_procedure_parameters(connection, &procedure_params);
	if (err) {
		LOG_ERR("Failed to set procedure parameters (err %d)", err);
		return 0;
	}

	struct bt_le_cs_procedure_enable_param params = {
		.config_id = CS_CONFIG_ID,
		.enable = 1,
	};

	err = bt_le_cs_procedure_enable(connection, &params);
	if (err) {
		LOG_ERR("Failed to enable CS procedures (err %d)", err);
		return 0;
	}

	while (true) {
			k_sem_take(&sem_subevent_results_parsed, K_FOREVER);
			distance_estimates_update();
		}

	return 0;
}
