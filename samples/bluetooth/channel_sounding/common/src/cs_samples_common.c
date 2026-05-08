/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Common source file for shared functionality of Channel Sounding samples
 */

#include <math.h>
#include <stdlib.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>

#include <dk_buttons_and_leds.h>

#if defined(CONFIG_BT_SCAN)
#include <bluetooth/scan.h>
#endif /* CONFIG_BT_SCAN */

LOG_MODULE_REGISTER(cs_samples_common, LOG_LEVEL_INF);

#define CON_STATUS_LED DK_LED1

struct bt_conn *connection;
struct bt_conn_le_cs_config cs_config;
K_SEM_DEFINE(sem_connected, 0, 1);
K_SEM_DEFINE(sem_security, 0, 1);
K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
K_SEM_DEFINE(sem_config_created, 0, 1);


static int float_cmp(const void *a, const void *b)
{
	float fa = *(const float *)a;
	float fb = *(const float *)b;

	return (fa > fb) - (fa < fb);
}

float common_median_inplace(int count, float *values)
{
	if (count == 0) {
		return NAN;
	}

	qsort(values, count, sizeof(float), float_cmp);

	if (count % 2 == 0) {
		return (values[count / 2] + values[count / 2 - 1]) / 2;
	} else {
		return values[count / 2];
	}
}


#if defined(CONFIG_BT_SCAN)
void common_scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

void common_scan_connecting_error(struct bt_scan_device_info *device_info)
{
	int err;

	LOG_INF("Connecting failed, restarting scanning");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Failed to restart scanning (err %i)", err);
		return;
	}
}

void common_scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn)
{
	LOG_INF("Connecting");
}
#endif /* CONFIG_BT_SCAN */

void common_connected_cb(struct bt_conn *conn, uint8_t err)
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

void common_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02X)", reason);

	bt_conn_unref(conn);
	connection = NULL;
	dk_set_led_off(CON_STATUS_LED);

	sys_reboot(SYS_REBOOT_COLD);
}


void common_security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
		return;
	}

	LOG_INF("Security changed: %s level %u", addr, level);
	k_sem_give(&sem_security);
}

void common_cs_security_enable_cb(struct bt_conn *conn, uint8_t status)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("CS security enabled.");
		k_sem_give(&sem_cs_security_enabled);
	} else {
		LOG_WRN("CS security enable failed. (HCI status 0x%02x)", status);
	}
}

void common_remote_capabilities_cb(struct bt_conn *conn, uint8_t status,
				   struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		static const char *const rtt_precision_str[] = {"not supported", "10 ns", "150 ns",
								"invalid"};

		uint8_t aa_idx = MIN((uint8_t)params->rtt_aa_only_precision, 3U);
		uint8_t snd_idx = MIN((uint8_t)params->rtt_sounding_precision, 3U);
		uint8_t rand_idx = MIN((uint8_t)params->rtt_random_payload_precision, 3U);

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
			params->num_config_supported, params->max_consecutive_procedures_supported,
			params->num_antennas_supported, params->max_antenna_paths_supported,
			params->initiator_supported ? "yes" : "no",
			params->reflector_supported ? "yes" : "no",
			params->mode_3_supported ? "yes" : "no", rtt_precision_str[aa_idx],
			(uint8_t)params->rtt_aa_only_precision, rtt_precision_str[snd_idx],
			(uint8_t)params->rtt_sounding_precision, rtt_precision_str[rand_idx],
			(uint8_t)params->rtt_random_payload_precision, params->rtt_aa_only_n,
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

void common_config_create_cb(struct bt_conn *conn, uint8_t status,
			     struct bt_conn_le_cs_config *config)
{
	ARG_UNUSED(conn);

	if (status == BT_HCI_ERR_SUCCESS) {
		static const char *const mode_str[5] = {"Unused", "1 (RTT)", "2 (PBR)",
						       "3 (RTT + PBR)", "Invalid"};
		static const char *const role_str[3] = {"Initiator", "Reflector", "Invalid"};
		static const char *const rtt_type_str[8] = {"AA only", "32-bit sounding",
							    "96-bit sounding", "32-bit random",
							    "64-bit random", "96-bit random",
							    "128-bit random", "Invalid"};
		static const char *const phy_str[4] = {"Invalid", "LE 1M PHY", "LE 2M PHY",
						       "LE 2M 2BT PHY"};
		static const char *const chsel_type_str[3] = {"Algorithm #3b", "Algorithm #3c",
							      "Invalid"};
		static const char *const ch3c_shape_str[3] = {"Hat shape", "X shape", "Invalid"};

		uint8_t mode_idx = (config->mode > 0 && config->mode < 4) ? config->mode : 4;
		uint8_t role_idx = MIN(config->role, 2);
		uint8_t rtt_type_idx = MIN(config->rtt_type, 7);
		uint8_t phy_idx = (config->cs_sync_phy > 0 && config->cs_sync_phy < 4)
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
			config->id, mode_str[mode_idx], config->min_main_mode_steps,
			config->max_main_mode_steps, config->main_mode_repetition,
			config->mode_0_steps, role_str[role_idx], rtt_type_str[rtt_type_idx],
			phy_str[phy_idx], config->channel_map_repetition,
			chsel_type_str[chsel_type_idx], ch3c_shape_str[ch3c_shape_idx],
			config->ch3c_jump, config->t_ip1_time_us, config->t_ip2_time_us,
			config->t_fcs_time_us, config->t_pm_time_us,
			sys_get_le32(&config->channel_map[6]),
			sys_get_le32(&config->channel_map[2]),
			sys_get_le16(&config->channel_map[0]));

			cs_config = *config;
			k_sem_give(&sem_config_created);
	} else {
		LOG_WRN("CS config creation failed. (HCI status 0x%02x)", status);
	}
}

void common_procedure_enable_cb(
	struct bt_conn *conn, uint8_t status,
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
