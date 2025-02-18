/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <nrf_modem_dect_phy.h>

#include "desh_print.h"

#include "dect_common_utils.h"
#include "dect_common_settings.h"
#include "dect_phy_common_rx.h"
#include "dect_phy_shell.h"
#include "dect_phy_api_scheduler_integration.h"

#include "dect_phy_rx.h"
#include "dect_phy_scan.h"
#include "dect_phy_perf.h"
#include "dect_phy_ping.h"
#include "dect_phy_rf_tool.h"
#include "dect_phy_mac.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_ctrl.h"

#define DECT_PHY_CTRL_STACK_SIZE 4096
#define DECT_PHY_CTRL_PRIORITY	 K_PRIO_COOP(9) /* -7 */
K_THREAD_STACK_DEFINE(dect_phy_ctrl_th_stack, DECT_PHY_CTRL_STACK_SIZE);

struct k_work_q dect_phy_ctrl_work_q;

/* States of all commands */
static struct dect_phy_ctrl_data {
	bool phy_api_initialized;

	int band_info_count;
	struct nrf_modem_dect_phy_band band_info[DESH_DECT_PHY_SUPPORTED_BAND_COUNT];
	bool band_info_band4_supported;

	bool mdm_latency_info_valid;
	struct nrf_modem_dect_phy_latency_info mdm_latency_info;

	struct nrf_modem_dect_phy_config_params dect_phy_init_params;

	bool last_set_radio_mode_successful;
	enum nrf_modem_dect_phy_radio_mode last_set_radio_mode;

	uint16_t phy_api_init_count;
	bool phy_api_capabilities_printed;

	bool scheduler_suspended;

	bool perf_ongoing;
	bool ping_ongoing;
	bool cert_ongoing;
	bool ext_command_running;

	bool rx_cmd_on_going;
	uint16_t rx_cmd_current_channel;
	struct dect_phy_rx_cmd_params rx_cmd_params;

	bool time_cmd;

	bool debug;

	int16_t last_received_pcc_pwr_dbm;
	int8_t last_received_pcc_mcs;
	uint64_t last_received_stf_start_time;
	uint8_t last_received_pcc_short_nw_id;
	uint16_t last_received_pcc_transmitter_short_rd_id;

	int16_t last_valid_temperature;

	uint8_t last_received_pcc_phy_len;
	enum dect_phy_packet_length_type last_received_pcc_phy_len_type;

	bool rssi_scan_ongoing;
	bool rssi_scan_cmd_running;
	struct dect_phy_rssi_scan_params rssi_scan_params;
	dect_phy_ctrl_rssi_scan_completed_callback_t rssi_scan_complete_cb;

	/* Registered callbacks for ext command usage */
	struct dect_phy_ctrl_ext_callbacks ext_cmd;

	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t default_pcc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t default_pdc_rcv_cb;
} ctrl_data;

K_SEM_DEFINE(rssi_scan_sema, 0, 1);

K_SEM_DEFINE(dect_phy_ctrl_mdm_api_init_sema, 0, 1);
K_SEM_DEFINE(dect_phy_ctrl_mdm_api_op_cancel_sema, 0, 1);
K_SEM_DEFINE(dect_phy_ctrl_mdm_api_radio_mode_config_sema, 0, 1);
K_MUTEX_DEFINE(dect_phy_ctrl_mdm_api_op_cancel_all_mutex);

/**************************************************************************************************/

static void dect_phy_rssi_channel_scan_completed_cb(enum nrf_modem_dect_phy_err phy_status);
static void dect_phy_ctrl_on_modem_lib_init(int ret, void *ctx);
static int dect_phy_ctrl_phy_handlers_init(void);

/**************************************************************************************************/

K_MSGQ_DEFINE(dect_phy_ctrl_msgq, sizeof(struct dect_phy_common_op_event_msgq_item), 1000, 4);

int dect_phy_ctrl_msgq_non_data_op_add(uint16_t event_id)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.id = event_id;
	event.data = NULL;
	ret = k_msgq_put(&dect_phy_ctrl_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

int dect_phy_ctrl_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);
	event.id = event_id;

	ret = k_msgq_put(&dect_phy_ctrl_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

static void dect_phy_ctrl_phy_configure_from_settings(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int ret;

	/* Configure modem */
	ctrl_data.dect_phy_init_params.harq_rx_expiry_time_us =
		current_settings->harq.mdm_init_harq_expiry_time_us;
	ctrl_data.dect_phy_init_params.harq_rx_process_count =
		current_settings->harq.mdm_init_harq_process_count;
	ctrl_data.dect_phy_init_params.band_group_index =
		((current_settings->common.band_nbr == 4) ? 1 : 0);

	ret = nrf_modem_dect_phy_configure(&ctrl_data.dect_phy_init_params);
	if (ret) {
		desh_error("%s: nrf_modem_dect_phy_configure returned: %i",
			(__func__), ret);
	}

	/* Wait that configure is done */
	ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_configure() timeout.", (__func__));
	} else {
		desh_print("DECT modem configured to band #%d.",
		current_settings->common.band_nbr);
	}
}

static void dect_phy_ctrl_radio_activate(enum nrf_modem_dect_phy_radio_mode radio_mode)
{
	int ret = nrf_modem_dect_phy_activate(radio_mode);
	char tmp_str[128] = {0};

	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_activate() failed: %d",
			(__func__), ret);
		return;
	}

	/* Wait that activate is done */
	ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(5));
	if (ret) {
		ctrl_data.last_set_radio_mode_successful = false;
		desh_error("(%s): nrf_modem_dect_phy_activate() timeout.",
			(__func__));
	} else {
		ctrl_data.last_set_radio_mode = radio_mode;
		ctrl_data.last_set_radio_mode_successful = true;
		desh_print("DECT modem activated to radio mode: %s.",
			dect_common_utils_radio_mode_to_string(
				radio_mode, tmp_str));
	}
}

/**************************************************************************************************/
void dect_phy_ctrl_utils_mdm_latency_info_print(void)
{
	char tmp_str[128] = {0};

	if (!ctrl_data.mdm_latency_info_valid) {
		desh_error("DECT modem latency info not valid.");
		return;
	}
	desh_print("DECT modem latency info:");
	desh_print(" Radio mode specific latencies:");

	for (int i = 0; i < NRF_MODEM_DECT_PHY_RADIO_MODE_COUNT; i++) {
		desh_print("  Radio mode: %s",
			dect_common_utils_radio_mode_to_string(i, tmp_str));
		desh_print("    scheduled_operation_transition: %d",
			ctrl_data.mdm_latency_info.radio_mode[i]
				.scheduled_operation_transition);
		desh_print("    scheduled_operation_startup: %d",
			ctrl_data.mdm_latency_info.radio_mode[i]
				.scheduled_operation_startup);
		for (int j = 0; j < NRF_MODEM_DECT_PHY_RADIO_MODE_COUNT; j++) {
			desh_print("    radio mode transition delay to %s: %d",
				dect_common_utils_radio_mode_to_string(j, tmp_str),
					ctrl_data.mdm_latency_info.radio_mode[i]
						.radio_mode_transition[j]);
		}
	}

	desh_print(" Operation latencies:");
	desh_print("  RX:");
	desh_print("   idle_to_active: %d",
		ctrl_data.mdm_latency_info.operation.receive.idle_to_active);
	desh_print("   active_to_idle_rssi: %d",
		ctrl_data.mdm_latency_info.operation.receive.active_to_idle_rssi);
	desh_print("   active_to_idle_rx: %d",
		ctrl_data.mdm_latency_info.operation.receive.active_to_idle_rx);
	desh_print("   stop_to_rf_off: %d",
		ctrl_data.mdm_latency_info.operation.receive.stop_to_rf_off);
	desh_print("  TX:");
	desh_print("   idle_to_active: %d",
		ctrl_data.mdm_latency_info.operation.transmit.idle_to_active);
	desh_print("   active_to_idle: %d",
		ctrl_data.mdm_latency_info.operation.transmit.active_to_idle);
	desh_print(" Modem radio configuration durations:");
	desh_print("  DECT PHY initialization duration: %d",
		ctrl_data.mdm_latency_info.stack.initialization);
	desh_print("  DECT PHY deinitialization duration: %d",
		ctrl_data.mdm_latency_info.stack.deinitialization);
	desh_print("  DECT PHY configuration duration: %d",
		ctrl_data.mdm_latency_info.stack.configuration);
	desh_print("  DECT PHY activation duration: %d",
		ctrl_data.mdm_latency_info.stack.activation);
	desh_print("  DECT PHY deactivation duration: %d",
		ctrl_data.mdm_latency_info.stack.deactivation);
}

int8_t dect_phy_ctrl_utils_mdm_max_tx_pwr_dbm_get_by_channel(uint16_t channel)
{
	for (int i = 0; i < ctrl_data.band_info_count; i++) {
		if (channel >= ctrl_data.band_info[i].min_carrier &&
		    channel <= ctrl_data.band_info[i].max_carrier) {
			return dect_common_utils_max_tx_pwr_dbm_by_pwr_class(
				ctrl_data.band_info[i].power_class);
		}
	}
	/* Fallback to safest choise, the lowest max */
	return DECT_PHY_CLASS_4_MAX_TX_POWER_DBM;
}

uint8_t dect_phy_ctrl_utils_mdm_max_tx_phy_pwr_get_by_channel(uint16_t channel)
{
	int8_t max_permitted_pwr_dbm =
		dect_phy_ctrl_utils_mdm_max_tx_pwr_dbm_get_by_channel(channel);

	return dect_common_utils_dbm_to_phy_tx_power(max_permitted_pwr_dbm);
}

uint8_t dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(uint8_t phy_power, uint16_t channel)
{
	uint8_t max_phy_pwr = dect_phy_ctrl_utils_mdm_max_tx_phy_pwr_get_by_channel(channel);
	uint8_t next_phy_pwr = dect_common_utils_next_phy_tx_power_get(phy_power);

	if (next_phy_pwr > max_phy_pwr) {
		next_phy_pwr = max_phy_pwr;
	}
	return next_phy_pwr;
}

void dect_phy_ctrl_utils_mdm_band_info_print(void)
{
	desh_print("DECT modem band info:");
	for (int i = 0; i < ctrl_data.band_info_count; i++) {
		desh_print("  Band number:      %d", ctrl_data.band_info[i].band_number);
		desh_print("  Band group index: %d", ctrl_data.band_info[i].band_group_index);
		desh_print("  Min carrier:      %d", ctrl_data.band_info[i].min_carrier);
		desh_print("  Max carrier:      %d", ctrl_data.band_info[i].max_carrier);
		desh_print("  Power class:      %d", ctrl_data.band_info[i].power_class);
		desh_print("    Max power:      %ddBm",
			dect_common_utils_max_tx_pwr_dbm_by_pwr_class(
				ctrl_data.band_info[i].power_class));
		desh_print("  RX gain:          %d\n", ctrl_data.band_info[i].rx_gain);
	}
}

/**************************************************************************************************/

static void dect_phy_ctrl_radio_configure_activate_from_settings(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int ret;

	ret = nrf_modem_dect_phy_band_get();
	if (ret) {
		desh_error("%s: nrf_modem_dect_phy_band_get returned: %i",
			(__func__), ret);
	}

	/* Wait that band_get is done */
	ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_band_get() timeout.", (__func__));
	} else {
		dect_phy_ctrl_utils_mdm_band_info_print();

		if (!ctrl_data.band_info_band4_supported &&
		    current_settings->common.band_nbr == 4) {
			desh_error("Band 4 is not supported in modem - setting default band #1.");
			current_settings->common.band_nbr = 1;
		}
	}

	dect_phy_ctrl_phy_configure_from_settings();
	if (current_settings->common.activate_at_startup) {
		dect_phy_ctrl_radio_activate(current_settings->common.startup_radio_mode);
	} else {
		desh_print("DECT modem not activated.");
		desh_print("  Activation can be done manually by using command \"dect activate\".");
	}
}

static void dect_phy_ctrl_msgq_thread_handler(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_ctrl_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_CTRL_OP_RADIO_ACTIVATED: {
			struct nrf_modem_dect_phy_activate_event *evt =
				(struct nrf_modem_dect_phy_activate_event *)event.data;
			char tmp_str[128] = {0};

			dect_common_utils_modem_phy_err_to_string(
				evt->err, evt->temp, tmp_str);

			if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_error("(%s): radio activation failed: %d (%s)",
					(__func__), evt->err, tmp_str);
				ctrl_data.last_set_radio_mode_successful = false;
			} else {
				ctrl_data.last_set_radio_mode_successful = true;
			}
			break;
		}
		case DECT_PHY_CTRL_OP_RADIO_DEACTIVATED: {
			struct nrf_modem_dect_phy_deactivate_event *evt =
				(struct nrf_modem_dect_phy_deactivate_event *)event.data;
			char tmp_str[128] = {0};

			dect_common_utils_modem_phy_err_to_string(
				evt->err, NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED, tmp_str);

			if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_error("(%s): radio deactivation failed: %d (%s)",
					(__func__), evt->err, tmp_str);
			} else {
				desh_print("Radio deactivated.");
			}
			break;
		}
		case DECT_PHY_CTRL_OP_RADIO_MODE_CONFIGURED: {
			struct nrf_modem_dect_phy_radio_config_event *evt =
				(struct nrf_modem_dect_phy_radio_config_event *)event.data;
			char tmp_str[128] = {0};

			dect_common_utils_modem_phy_err_to_string(
				evt->err, NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED, tmp_str);

			if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_error("(%s): radio mode configuration failed: %d (%s)",
					(__func__), evt->err, tmp_str);
				ctrl_data.last_set_radio_mode_successful = false;
			} else {
				ctrl_data.last_set_radio_mode_successful = true;
				desh_print("Radio mode configured.");
			}
			break;
		}

		case DECT_PHY_CTRL_OP_SCHEDULER_SUSPENDED: {
			ctrl_data.scheduler_suspended = true;
			break;
		}
		case DECT_PHY_CTRL_OP_SCHEDULER_RESUMED: {
			ctrl_data.scheduler_suspended = false;
			break;
		}
		case DECT_PHY_CTRL_OP_PERF_CMD_DONE: {
			ctrl_data.perf_ongoing = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			dect_phy_ctrl_phy_handlers_init();

			desh_print("perf command done.");
			break;
		}
		case DECT_PHY_CTRL_OP_PING_CMD_DONE: {
			ctrl_data.ping_ongoing = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			dect_phy_ctrl_phy_handlers_init();
			desh_print("ping command done.");
			break;
		}
		case DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE: {
			ctrl_data.cert_ongoing = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			dect_phy_ctrl_phy_handlers_init();
			desh_print("rf_tool command done.");
			break;
		}
		case DECT_PHY_CTRL_OP_DEBUG_ON: {
			ctrl_data.debug = true;
			break;
		}
		case DECT_PHY_CTRL_OP_DEBUG_OFF: {
			ctrl_data.debug = false;
			break;
		}
		case DECT_PHY_CTRL_OP_SETTINGS_UPDATED: {
			if (ctrl_data.ext_cmd.sett_changed_cb != NULL) {
				ctrl_data.ext_cmd.sett_changed_cb();
			}
			break;
		}
		case DECT_PHY_CTRL_OP_PHY_API_MDM_INITIALIZED: {
			struct nrf_modem_dect_phy_init_event *params =
				(struct nrf_modem_dect_phy_init_event *)event.data;
			char tmp_str[128] = {0};
			int ret;

			if (params->temp != NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED) {
				ctrl_data.last_valid_temperature = params->temp;
			}

			if (params->err != NRF_MODEM_DECT_PHY_SUCCESS) {
				dect_common_utils_modem_phy_err_to_string(
				params->err, params->temp, tmp_str);

				desh_error("(%s): init failed (temperature %d, "
					   "temp_limit %d): %d (%s)",
					   (__func__), params->temp,
					   params->temperature_limit,
					   params->err, tmp_str);
			} else {
				k_sleep(K_MSEC(100));
				if (ctrl_data.phy_api_init_count <= 1) {
					desh_print("DECT modem initialized:");
					desh_print("  current temperature:       %dC",
						   params->temp);
				}
			}
			k_sleep(K_MSEC(100)); /* Wait that prev print is done */

			/* Capability query */
			ret = nrf_modem_dect_phy_capability_get(); /* asynch: result in callback */
			if (ret) {
				printk("nrf_modem_dect_phy_capability_get returned: %i\n", ret);
			}

			/* Wait that Capability is done */
			ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(5));
			if (ret) {
				desh_error("(%s): nrf_modem_dect_phy_capability_get() timeout.",
					(__func__));
				/* continue */
			}

			k_sleep(K_MSEC(100)); /* Wait that capability print is done */

			ret = nrf_modem_dect_phy_latency_get();
			if (ret) {
				desh_error("nrf_modem_dect_phy_latency_get returned: %i", ret);
			}

			/* Wait that latency query is done */
			ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(5));
			if (ret) {
				desh_error("(%s): nrf_modem_dect_phy_latency_get() timeout.",
					(__func__));
				/* continue */
			} else {
				dect_phy_ctrl_utils_mdm_latency_info_print();
			}

			/* Configure and activate */
			dect_phy_ctrl_radio_configure_activate_from_settings();
			break;
		}

		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC_CRC_ERROR: {
			struct dect_phy_common_op_pcc_crc_fail_params *params =
				(struct dect_phy_common_op_pcc_crc_fail_params *)event.data;

			desh_warn("RX PCC CRC error (time %llu, handle %d): "
				  "SNR %d, RSSI-2 %d (%d dBm)",
				  params->time, params->crc_failure.handle,
				  params->crc_failure.snr, params->crc_failure.rssi_2,
				  (params->crc_failure.rssi_2 / 2));
			break;
		}
		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_CRC_ERROR: {
			struct dect_phy_common_op_pdc_crc_fail_params *params =
				(struct dect_phy_common_op_pdc_crc_fail_params *)event.data;

			desh_warn("RX PDC CRC error (time %llu, handle %d): "
				  "SNR %d, RSSI-2 %d (%d dBm)",
				  params->time, params->crc_failure.handle,
				  params->crc_failure.snr, params->crc_failure.rssi_2,
				  (params->crc_failure.rssi_2 / 2));
			break;
		}

		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC: {
			struct dect_phy_common_op_pcc_rcv_params *params =
				(struct dect_phy_common_op_pcc_rcv_params *)event.data;
			char tmp_str[128] = {0};

			dect_common_utils_modem_phy_header_status_to_string(
				params->pcc_status.header_status, tmp_str);
			if (ctrl_data.debug) {
				int16_t rssi_level = params->pcc_status.rssi_2 / 2;

				desh_print("PCC received (stf start time %llu, handle %d): "
					   "status: \"%s\", snr %d, "
					   "RSSI-2 %d (RSSI %d)",
					   params->stf_start_time, params->pcc_status.handle,
					   tmp_str, params->pcc_status.snr,
					   params->pcc_status.rssi_2, rssi_level);
			}
			if (params->pcc_status.header_status ==
			    NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
				if (ctrl_data.debug) {
					desh_print("  phy header: short nw id %d (0x%02x), "
						   "transmitter id %u",
						   params->short_nw_id, params->short_nw_id,
						   params->transmitter_short_rd_id);
					if (params->pcc_status.phy_type == 1) {
						/* Type 2 header */
						desh_print("  receiver id: %u",
							params->receiver_short_rd_id);
					}
					desh_print("  len %d, MCS %d, TX pwr: %d dBm",
						   params->phy_len, params->mcs, params->pwr_dbm);
				}
			}
			if (ctrl_data.ext_cmd.pcc_rcv_cb != NULL) {
				ctrl_data.ext_cmd.pcc_rcv_cb(params);
			}
			break;
		}

		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_DATA: {
			struct dect_phy_commmon_op_pdc_rcv_params *params =
				(struct dect_phy_commmon_op_pdc_rcv_params *)event.data;
			bool data_handled = false;

			/* 1st try to decode dect mac */
			data_handled = dect_phy_mac_handle(params);

			if (!data_handled && ctrl_data.ext_cmd.pdc_rcv_cb != NULL) {
				data_handled = ctrl_data.ext_cmd.pdc_rcv_cb(params);
			}

			if (!data_handled) {
				unsigned char hex_data[128];
				int i;
				struct nrf_modem_dect_phy_pdc_event *p_rx_status =
					&(params->rx_status);
				int16_t rssi_level = p_rx_status->rssi_2 / 2;

				desh_print("PDC received (time %llu): snr %d, RSSI-2 %d "
					   "(RSSI %d), len %d",
					   params->time, p_rx_status->snr, p_rx_status->rssi_2,
					   rssi_level, params->data_length);
				for (i = 0; i < 128 && i < params->data_length; i++) {
					sprintf(&hex_data[i], "%02x ", params->data[i]);
				}
				hex_data[i + 1] = '\0';
				desh_warn("Received unknown data, len %d, hex data: %s\n",
					  params->data_length, hex_data);
			}

			break;
		}

		case DECT_PHY_CTRL_OP_PHY_API_MDM_COMPLETED: {
			struct dect_phy_common_op_completed_params *params =
				(struct dect_phy_common_op_completed_params *)event.data;

			if (params->temperature != NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED) {
				ctrl_data.last_valid_temperature = params->temperature;
			}

			if (params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				char tmp_str[128] = {0};

				dect_common_utils_modem_phy_err_to_string(
					params->status, params->temperature, tmp_str);
				if (params->handle == DECT_PHY_COMMON_RSSI_SCAN_HANDLE) {
					dect_phy_scan_rssi_finished_handle(params->status);
				} else if (params->handle == ctrl_data.rx_cmd_params.handle) {
					dect_phy_api_scheduler_resume();
					desh_error("%s: cannot start RX: %s", __func__, tmp_str);
					ctrl_data.rx_cmd_on_going = false;
				} else {
					if (ctrl_data.ext_cmd.op_complete_cb != NULL) {
						ctrl_data.ext_cmd.op_complete_cb(params);
					}
				}
			} else {
				if (params->handle == ctrl_data.rx_cmd_params.handle) {
					struct dect_phy_rx_cmd_params copy_params =
						ctrl_data.rx_cmd_params;
					int err;

					err = dect_phy_ctrl_rx_start(&copy_params, true);
					if (err) {
						ctrl_data.rx_cmd_on_going = false;
						dect_phy_api_scheduler_resume();
						desh_print("RX DONE.");
					}
				} else if (params->handle == DECT_PHY_COMMON_RSSI_SCAN_HANDLE) {
					dect_phy_scan_rssi_finished_handle(params->status);
				} else {
					if (ctrl_data.ext_cmd.op_complete_cb != NULL) {
						ctrl_data.ext_cmd.op_complete_cb(params);
					}
				}
			}
			break;
		}

		case DECT_PHY_CTRL_OP_RSSI_SCAN_RUN: {
			int err = 0;

			if (ctrl_data.rssi_scan_ongoing ||
			    (ctrl_data.rssi_scan_cmd_running &&
			     ctrl_data.rssi_scan_params.interval_secs)) {
				if (ctrl_data.rssi_scan_params.suspend_scheduler) {
					dect_phy_ctrl_mdm_op_cancel_all();
					dect_phy_api_scheduler_suspend();
				}
				err = dect_phy_scan_rssi_start(
					&ctrl_data.rssi_scan_params,
					dect_phy_rssi_channel_scan_completed_cb);
				if (err) {
					desh_error("RSSI scan start failed: %d", err);
					dect_phy_ctrl_rssi_scan_stop();
				}
			}
			break;
		}
		case DECT_PHY_CTRL_OP_PHY_API_MDM_RSSI_CB_ON_RX_OP: {
			if (ctrl_data.rx_cmd_on_going || ctrl_data.ext_command_running) {
				dect_phy_scan_rssi_finished_handle(NRF_MODEM_DECT_PHY_SUCCESS);
				if (dect_phy_scan_rssi_data_reinit_with_current_params()) {
					desh_error("Cannot reinit rssi scan data");
				}
			}
			break;
		}

		default:
			desh_warn("DECT CTRL: Unknown event %d received", event.id);
			break;
		}
		k_free(event.data);
	}
}

K_THREAD_DEFINE(dect_phy_ctrl_msgq_th, DECT_PHY_CTRL_STACK_SIZE, dect_phy_ctrl_msgq_thread_handler,
		NULL, NULL, NULL, DECT_PHY_CTRL_PRIORITY, 0, 0);

/**************************************************************************************************/

static void dect_phy_ctrl_mdm_initialize_cb(const struct nrf_modem_dect_phy_init_event *evt)
{

	if (evt->err == NRF_MODEM_DECT_PHY_SUCCESS) {
		ctrl_data.phy_api_init_count++;
		ctrl_data.phy_api_initialized = true;
	}

	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_INITIALIZED,
				       (void *)evt,
				       sizeof(struct nrf_modem_dect_phy_init_event));
}

static void dect_phy_ctrl_mdm_on_deinit_cb(const struct nrf_modem_dect_phy_deinit_event *deinit_evt)
{
	ctrl_data.phy_api_initialized = false;
}

void dect_phy_ctrl_mdm_activate_cb(const struct nrf_modem_dect_phy_activate_event *evt)
{
	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_RADIO_ACTIVATED,
				       (void *)evt,
				       sizeof(struct nrf_modem_dect_phy_activate_event));
}

void dect_phy_ctrl_mdm_deactivate_cb(const struct nrf_modem_dect_phy_deactivate_event *evt)
{
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_RADIO_DEACTIVATED,
				       (void *)evt,
				       sizeof(struct nrf_modem_dect_phy_deactivate_event));
}

void dect_phy_ctrl_mdm_configure_cb(const struct nrf_modem_dect_phy_configure_event *evt)
{
	if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
		printk("PHY configure failed: %d\n", evt->err);
	}
	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
}

void dect_phy_ctrl_mdm_radio_config_cb(
	const struct nrf_modem_dect_phy_radio_config_event *evt)
{
	k_sem_give(&dect_phy_ctrl_mdm_api_radio_mode_config_sema);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_RADIO_MODE_CONFIGURED,
				       (void *)evt,
				       sizeof(struct nrf_modem_dect_phy_radio_config_event));
}

static void dect_phy_ctrl_mdm_operation_complete_cb(
	const struct nrf_modem_dect_phy_op_complete_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_completed_params op_completed_params = {
		.handle = evt->handle,
		.temperature = evt->temp,
		.status = evt->err,
		.time = *time,
	};

	dect_phy_api_scheduler_mdm_op_completed(&op_completed_params);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_COMPLETED,
				       (void *)&op_completed_params,
				       sizeof(struct dect_phy_common_op_completed_params));
}

static void dect_phy_ctrl_mdm_on_rssi_cb(const struct nrf_modem_dect_phy_rssi_event *evt)
{
	if (ctrl_data.rssi_scan_ongoing || ctrl_data.rx_cmd_on_going) {
		dect_phy_scan_rssi_cb_handle(NRF_MODEM_DECT_PHY_SUCCESS, evt);
	} else if (ctrl_data.ext_cmd.direct_rssi_cb != NULL) {
		ctrl_data.ext_cmd.direct_rssi_cb(evt);
	}
}

void dect_phy_ctrl_mdm_cancel_cb(const struct nrf_modem_dect_phy_cancel_event *evt)
{
	k_sem_give(&dect_phy_ctrl_mdm_api_op_cancel_sema);
}

static void
dect_phy_ctrl_mdm_on_rx_pcc_cb(const struct nrf_modem_dect_phy_pcc_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_pcc_rcv_params ctrl_pcc_op_params;

	ctrl_data.last_received_stf_start_time = evt->stf_start_time;

	if (evt->header_status == NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
		struct dect_phy_ctrl_field_common *phy_h = (void *)&evt->hdr;
		const uint8_t *p_ptr = (uint8_t *)&evt->hdr;

		uint8_t short_nw_id;
		int16_t pcc_rx_pwr_dbm =
			dect_common_utils_phy_tx_power_to_dbm(phy_h->transmit_power);
		uint16_t transmitter_id, receiver_id;

		p_ptr++;
		short_nw_id = *p_ptr++;
		transmitter_id = sys_get_be16(p_ptr);
		p_ptr = p_ptr + 3;
		receiver_id = sys_get_be16(p_ptr);

		ctrl_data.last_received_pcc_short_nw_id = short_nw_id;
		ctrl_data.last_received_pcc_transmitter_short_rd_id = transmitter_id;
		ctrl_data.last_received_pcc_phy_len = phy_h->packet_length;
		ctrl_data.last_received_pcc_phy_len_type = phy_h->packet_length_type;
		ctrl_data.last_received_pcc_pwr_dbm = pcc_rx_pwr_dbm;
		ctrl_data.last_received_pcc_mcs = phy_h->df_mcs;

		ctrl_pcc_op_params.short_nw_id = short_nw_id;
		ctrl_pcc_op_params.transmitter_short_rd_id = transmitter_id;
		ctrl_pcc_op_params.receiver_short_rd_id = receiver_id;
		ctrl_pcc_op_params.mcs = phy_h->df_mcs;
		ctrl_pcc_op_params.pwr_dbm = pcc_rx_pwr_dbm;

		ctrl_pcc_op_params.phy_len = phy_h->packet_length;
		ctrl_pcc_op_params.phy_len_type = phy_h->packet_length_type;
	}

	ctrl_pcc_op_params.pcc_status = *evt;
	ctrl_pcc_op_params.phy_header = evt->hdr;
	ctrl_pcc_op_params.time = *time;
	ctrl_pcc_op_params.stf_start_time = evt->stf_start_time;

	if (ctrl_data.ext_cmd.direct_pcc_rcv_cb != NULL) {
		ctrl_data.ext_cmd.direct_pcc_rcv_cb(&ctrl_pcc_op_params);
	}

	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC,
				       (void *)&ctrl_pcc_op_params,
				       sizeof(struct dect_phy_common_op_pcc_rcv_params));
}

static void dect_phy_ctrl_mdm_on_pcc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pcc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pcc_crc_fail_params));
}

static void dect_phy_ctrl_mdm_on_rx_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt)
{
	int16_t rssi_level = evt->rssi_2 / 2;
	uint16_t channel;
	int err;

	struct dect_phy_commmon_op_pdc_rcv_params ctrl_pdc_op_params;
	struct dect_phy_api_scheduler_op_pdc_type_rcvd_params pdc_op_params;

	ctrl_pdc_op_params.rx_status = *evt;

	ctrl_pdc_op_params.data_length = evt->len;
	ctrl_pdc_op_params.time = ctrl_data.last_received_stf_start_time;

	ctrl_pdc_op_params.rx_pwr_dbm = ctrl_data.last_received_pcc_pwr_dbm;
	ctrl_pdc_op_params.rx_mcs = ctrl_data.last_received_pcc_mcs;
	ctrl_pdc_op_params.rx_rssi_level_dbm = rssi_level;
	ctrl_pdc_op_params.rx_channel = 0;

	err =  dect_phy_common_rx_op_handle_to_channel_get(evt->handle, &channel);
	if (!err) {
		ctrl_pdc_op_params.rx_channel = channel;
	}

	ctrl_pdc_op_params.last_received_pcc_short_nw_id = ctrl_data.last_received_pcc_short_nw_id;
	ctrl_pdc_op_params.last_received_pcc_transmitter_short_rd_id =
		ctrl_data.last_received_pcc_transmitter_short_rd_id;
	ctrl_pdc_op_params.last_received_pcc_phy_len_type =
		ctrl_data.last_received_pcc_phy_len_type;
	ctrl_pdc_op_params.last_received_pcc_phy_len = ctrl_data.last_received_pcc_phy_len;

	if (evt->len <= sizeof(ctrl_pdc_op_params.data)) {
		memcpy(ctrl_pdc_op_params.data, evt->data, evt->len);
		if (ctrl_data.ext_cmd.direct_pdc_rcv_cb != NULL) {
			ctrl_data.ext_cmd.direct_pdc_rcv_cb(&ctrl_pdc_op_params);
		}

		dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_DATA,
					       (void *)&ctrl_pdc_op_params,
					       sizeof(struct dect_phy_commmon_op_pdc_rcv_params));
	} else {
		printk("Received data is too long to be received by CTRL TH - discarded (len %d, "
		       "buf size %d)\n",
		       evt->len, sizeof(ctrl_pdc_op_params.data));
	}

	pdc_op_params.data_length = evt->len;
	pdc_op_params.time = ctrl_data.last_received_stf_start_time;
	pdc_op_params.rx_pwr_dbm = ctrl_data.last_received_pcc_pwr_dbm;
	pdc_op_params.rx_rssi_dbm = rssi_level;
	if (evt->len <= sizeof(pdc_op_params.data)) {
		memcpy(pdc_op_params.data, evt->data, evt->len);
		dect_phy_api_scheduler_mdm_pdc_data_recv(&pdc_op_params);
	} else {
		printk("Received data is too long to be received by SCHEDULER TH - discarded (len "
		       "%d, buf size %d)\n",
		       evt->len, sizeof(pdc_op_params.data));
	}
}

static void dect_phy_ctrl_mdm_on_pdc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pdc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pdc_crc_fail_params));
}

void dect_phy_ctrl_mdm_time_query_cb(
	const struct nrf_modem_dect_phy_time_get_event *evt, uint64_t *time)
{
	if (ctrl_data.time_cmd) {
		uint64_t time_now;
		int64_t time_diff;
		static const char *behind_str = "behind";
		static const char *ahead_str = "ahead";

		if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
			printk("\nCannot get modem time, err %d", evt->err);
			return;
		}

		time_now = dect_app_modem_time_now();
		time_diff = *time - time_now;

		if (time_diff >= 0) {
			printk("\nModem time: %llu, app time %lld (%s by %lld ticks)\n", *time,
			       time_now, behind_str, time_diff);
		} else {
			printk("\nModem time: %llu, app time %lld (%s by %lld ticks)\n", *time,
			       time_now, ahead_str, time_diff);
		}
	}
	ctrl_data.time_cmd = false;
}

void dect_phy_ctrl_mdm_on_capability_get_cb(
	const struct nrf_modem_dect_phy_capability_get_event *evt)
{
	if (evt->err != NRF_MODEM_DECT_PHY_SUCCESS) {
		printk("Cannot get modem capabilities, err %d\n", evt->err);
		return;
	}
	if (!ctrl_data.phy_api_capabilities_printed) {
		struct nrf_modem_dect_phy_capability *capability = evt->capability;

		if (capability == NULL) {
			printk("\nModem capabilities:\n");
			printk("  No capabilities available\n");
		} else {
			printk("\nModem capabilities:\n");
			printk("  DECT NR+ version: %d\n", capability->dect_version);
			printk("  DECT NR+ variant count: %d\n", capability->variant_count);
			for (int i = 0; i < capability->variant_count; i++) {
				printk("    rx_spatial_streams[%d]:     %d\n", i,
				capability->variant[i].rx_spatial_streams);
				printk("    rx_tx_diversity[%d]:        %d\n", i,
				capability->variant[i].rx_tx_diversity);
				printk("    mcs_max[%d]:                %d\n", i,
				capability->variant[i].mcs_max);
				printk("    harq_soft_buf_size[%d]:     %d\n", i,
				capability->variant[i].harq_soft_buf_size);
				printk("    harq_process_count_max[%d]: %d\n", i,
				capability->variant[i].harq_process_count_max);
				printk("    harq_feedback_delay[%d]:    %d\n", i,
				capability->variant[i].harq_feedback_delay);
				printk("    mu[%d]:                     %d\n", i,
				capability->variant[i].mu);

				/* desh supports only 1 for mu */
				__ASSERT_NO_MSG(capability->variant[i].mu == 1);

				printk("    beta[%d]:                   %d\n", i,
				capability->variant[i].beta);
			}
		}
		ctrl_data.phy_api_capabilities_printed = true;
	}
	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
}

static void dect_phy_ctrl_mdm_on_band_get_cb(const struct nrf_modem_dect_phy_band_get_event *evt)
{
	ctrl_data.band_info_count = 0;
	ctrl_data.band_info_band4_supported = false;

	for (size_t i = 0; i < evt->band_count && i < DESH_DECT_PHY_SUPPORTED_BAND_COUNT; i++) {
		/* Store band info */
		ctrl_data.band_info[i] = evt->band[i];
		ctrl_data.band_info_count++;
		if (ctrl_data.band_info[i].band_number == 4) {
			ctrl_data.band_info_band4_supported = true;
		}
	}
	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
}

static void dect_phy_ctrl_mdm_on_latency_cb(const struct nrf_modem_dect_phy_latency_info_event *evt)
{
	if (evt->err == NRF_MODEM_DECT_PHY_SUCCESS && evt->latency_info != NULL) {
		ctrl_data.mdm_latency_info = *evt->latency_info;
		ctrl_data.mdm_latency_info_valid = true;
	} else {
		ctrl_data.mdm_latency_info_valid = false;
	}

	k_sem_give(&dect_phy_ctrl_mdm_api_init_sema);
}

void dect_phy_ctrl_mdm_evt_handler(const struct nrf_modem_dect_phy_event *evt)
{
	dect_app_modem_time_save(&evt->time);

	switch (evt->id) {
	case NRF_MODEM_DECT_PHY_EVT_INIT:
		dect_phy_ctrl_mdm_initialize_cb(&evt->init);
		break;
	case NRF_MODEM_DECT_PHY_EVT_DEINIT:
		dect_phy_ctrl_mdm_on_deinit_cb(&evt->deinit);
		break;
	case NRF_MODEM_DECT_PHY_EVT_ACTIVATE:
		dect_phy_ctrl_mdm_activate_cb(&evt->activate);
		break;
	case NRF_MODEM_DECT_PHY_EVT_DEACTIVATE:
		dect_phy_ctrl_mdm_deactivate_cb(&evt->deactivate);
		break;
	case NRF_MODEM_DECT_PHY_EVT_CONFIGURE:
		dect_phy_ctrl_mdm_configure_cb(&evt->configure);
		break;
	case NRF_MODEM_DECT_PHY_EVT_RADIO_CONFIG:
		dect_phy_ctrl_mdm_radio_config_cb(&evt->radio_config);
		break;
	case NRF_MODEM_DECT_PHY_EVT_COMPLETED:
		dect_phy_ctrl_mdm_operation_complete_cb(&evt->op_complete, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_CANCELED:
		dect_phy_ctrl_mdm_cancel_cb(&evt->cancel);
		break;
	case NRF_MODEM_DECT_PHY_EVT_RSSI:
		dect_phy_ctrl_mdm_on_rssi_cb(&evt->rssi);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC:
		dect_phy_ctrl_mdm_on_rx_pcc_cb(&evt->pcc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC_ERROR:
		dect_phy_ctrl_mdm_on_pcc_crc_failure_cb(&evt->pcc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC:
		dect_phy_ctrl_mdm_on_rx_pdc_cb(&evt->pdc);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC_ERROR:
		dect_phy_ctrl_mdm_on_pdc_crc_failure_cb(&evt->pdc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_TIME:
		dect_phy_ctrl_mdm_time_query_cb(&evt->time_get, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_CAPABILITY:
		dect_phy_ctrl_mdm_on_capability_get_cb(&evt->capability_get);
		break;
	case NRF_MODEM_DECT_PHY_EVT_BANDS:
		dect_phy_ctrl_mdm_on_band_get_cb(&evt->band_get);
		break;
	case NRF_MODEM_DECT_PHY_EVT_LATENCY:
		dect_phy_ctrl_mdm_on_latency_cb(&evt->latency_get);
		break;
	default:
		printk("%s: WARN: Unexpected event id %d\n", (__func__), evt->id);
		break;
	}
}

static void dect_phy_ctrl_phy_init(void)
{
	int ret = nrf_modem_dect_phy_event_handler_set(dect_phy_ctrl_mdm_evt_handler);

	if (ret) {
		printk("nrf_modem_dect_phy_event_handler_set returned: %i\n", ret);
	} else {
		ret = nrf_modem_dect_phy_init();
		if (ret) {
			printk("nrf_modem_dect_phy_init returned: %i\n", ret);
		}
	}
}

void dect_phy_ctrl_mdm_op_cancel_all(void)
{
	int err;

	k_mutex_lock(&dect_phy_ctrl_mdm_api_op_cancel_all_mutex, K_FOREVER);

	k_sem_reset(&dect_phy_ctrl_mdm_api_op_cancel_sema);
	err = nrf_modem_dect_phy_cancel(NRF_MODEM_DECT_PHY_HANDLE_CANCEL_ALL);

	if (err) {
		desh_error("%s: cannot cancel ongoing operations: %d", (__func__), err);
		/* continue */
	} else {
		/* Wait that cancel(all) is done */
		err = k_sem_take(&dect_phy_ctrl_mdm_api_op_cancel_sema, K_SECONDS(5));
		if (err) {
			desh_error("%s: nrf_modem_dect_phy_cancel() timeout.", (__func__));
			/* continue */
		}
	}
	k_mutex_unlock(&dect_phy_ctrl_mdm_api_op_cancel_all_mutex);
}

/**************************************************************************************************/

int dect_phy_ctrl_time_query(void)
{
	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}
	ctrl_data.time_cmd = true;

	return nrf_modem_dect_phy_time_get();
}

int dect_phy_ctrl_modem_temperature_get(void)
{
	return ctrl_data.last_valid_temperature;
}

int dect_phy_ctrl_current_radio_mode_get(
	enum nrf_modem_dect_phy_radio_mode *radio_mode)
{
	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}
	*radio_mode = ctrl_data.last_set_radio_mode;

	return 0;
}

static enum nrf_modem_dect_phy_radio_mode dect_phy_ctrl_last_radio_mode_get(void)
{
	return ctrl_data.last_set_radio_mode;
}

static struct nrf_modem_dect_phy_latency_info *dect_phy_ctrl_modem_latency_info_ref_get(void)
{
	return &ctrl_data.mdm_latency_info;
}

uint64_t dect_phy_ctrl_modem_latency_min_margin_between_ops_get(void)
{
	uint64_t latency = DECT_PHY_TX_RX_SCHEDULING_OFFSET_MDM_TICKS;

	if (ctrl_data.mdm_latency_info_valid) {
		struct nrf_modem_dect_phy_latency_info *latency_info =
			dect_phy_ctrl_modem_latency_info_ref_get();
		enum nrf_modem_dect_phy_radio_mode radio_mode = dect_phy_ctrl_last_radio_mode_get();
		uint32_t scheduler_offset =
			latency_info->radio_mode[radio_mode]
				.scheduled_operation_transition;

		latency = scheduler_offset;
	}
	return latency;
}

uint64_t dect_phy_ctrl_modem_latency_for_next_op_get(bool is_tx)
{
	uint64_t latency = DECT_PHY_TX_RX_SCHEDULING_OFFSET_MDM_TICKS;
	uint64_t min_margin = dect_phy_ctrl_modem_latency_min_margin_between_ops_get();

	if (ctrl_data.mdm_latency_info_valid) {
		struct nrf_modem_dect_phy_latency_info *latency_info =
			dect_phy_ctrl_modem_latency_info_ref_get();
		enum nrf_modem_dect_phy_radio_mode radio_mode = dect_phy_ctrl_last_radio_mode_get();

		if (is_tx) {
			latency = latency_info->radio_mode[radio_mode]
					.scheduled_operation_startup +
				latency_info->operation.transmit.idle_to_active;
		} else {
			latency = latency_info->radio_mode[radio_mode]
					.scheduled_operation_startup +
				latency_info->operation.receive.idle_to_active;
		}
	}
	return MAX(latency, min_margin);
}

/**************************************************************************************************/

bool dect_phy_ctrl_rx_is_ongoing(void)
{
	return ctrl_data.rx_cmd_on_going;
}

void dect_phy_ctrl_rx_stop(void)
{
	ctrl_data.rx_cmd_on_going = false;
	(void)nrf_modem_dect_phy_cancel(ctrl_data.rx_cmd_params.handle);
	dect_phy_api_scheduler_resume();
}

int dect_phy_ctrl_rx_start(struct dect_phy_rx_cmd_params *params, bool restart)
{
	int err;

	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	if (restart && !ctrl_data.rx_cmd_on_going) {
		desh_warn("Cannot restart RX when RX is not on going.");
		return -EINVAL;
	}

	if (ctrl_data.rx_cmd_on_going && !restart) {
		desh_error("rx cmd already on going.");
		return -EALREADY;
	}

	if (params->suspend_scheduler) {
		dect_phy_ctrl_mdm_op_cancel_all();
		dect_phy_api_scheduler_suspend();
	}

	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	if (params->channel == 0) {
		if (restart) {
			uint16_t next_channel = dect_common_utils_get_next_channel_in_band_range(
				current_settings->common.band_nbr,
				ctrl_data.rx_cmd_current_channel,
				!params->ch_acc_use_all_channels);

			if (next_channel) {
				ctrl_data.rx_cmd_current_channel = next_channel;
			} else {
				/* We are done */
				desh_print("All channels in band scanned.");
				return -ENODATA;
			}
		} else {
			/* All channels in a set band */
			ctrl_data.rx_cmd_current_channel =
				dect_common_utils_channel_min_on_band(
					current_settings->common.band_nbr);
		}
	} else {
		if (restart) {
			/* Cannot restart RX on a specific channel */
			return -EINVAL;
		}
		ctrl_data.rx_cmd_current_channel = params->channel;
	}

	struct dect_phy_rssi_scan_params rssi_params = {
		.channel = ctrl_data.rx_cmd_current_channel,
		.result_verdict_type = DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL,
		.busy_rssi_limit = params->busy_rssi_limit,
		.free_rssi_limit = params->free_rssi_limit,
		.interval_secs = params->rssi_interval_secs,
		.scan_time_ms = params->rssi_interval_secs * 1000,
	};

	ctrl_data.rx_cmd_params = *params;
	params->channel = ctrl_data.rx_cmd_current_channel;

	err = dect_phy_scan_rssi_data_init(&rssi_params);
	if (err) {
		desh_error("RSSI scan data init failed: err", err);
		return err;
	}

	err = dect_phy_rx_msgq_data_op_add(DECT_PHY_RX_OP_START, (void *)params,
					   sizeof(struct dect_phy_rx_cmd_params));
	if (!err) {
		ctrl_data.rx_cmd_on_going = true;
	}
	return err;
}

/**************************************************************************************************/

void dect_phy_ctrl_status_get_n_print(void)
{
	char tmp_mdm_str[128] = {0};
	int ret;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int temperature = dect_phy_ctrl_modem_temperature_get();

	desh_print("desh-dect-phy status:");
	ret = nrf_modem_at_scanf("AT+CGMR", "%s", tmp_mdm_str);
	if (ret <= 0) {
		sprintf(tmp_mdm_str, "%s", "not known");
	}
	desh_print("  Transmitter ID:    %d", current_settings->common.transmitter_id);
	desh_print("  Modem firmware:    %s", tmp_mdm_str);

	ret = nrf_modem_at_scanf("AT%HWVERSION", "%%HWVERSION: %" STRINGIFY(128) "[^\r\n]",
									    tmp_mdm_str);
	if (ret <= 0) {
		sprintf(tmp_mdm_str, "%s", "not known");
	}
	desh_print("  HW version:        %s", tmp_mdm_str);

	if (temperature == NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED) {
		sprintf(tmp_mdm_str, "%s", "not known");
	} else {
		sprintf(tmp_mdm_str, "%d", temperature);
	}
	desh_print("  Modem temperature: %sC", tmp_mdm_str);
	dect_phy_ctrl_utils_mdm_band_info_print();
#if defined(CONFIG_BME680)
	struct sensor_value data = {0};

	/* Fetch all data the sensor supports. */
	ret = sensor_sample_fetch_chan(temp_sensor, SENSOR_CHAN_ALL);
	if (ret) {
		desh_error("Failed to sample temperature sensor, error %d", ret);
	} else {
		/* Pick out the ambient temperature data. */
		ret = sensor_channel_get(temp_sensor, SENSOR_CHAN_AMBIENT_TEMP, &data);
		if (ret) {
			desh_error("Failed to read ambient temperature, error %d", ret);
		} else {
			desh_print("  Ambient temperature: %d", (int)sensor_value_to_double(&data));
		}
		/* Pick out the ambient temperature data. */
		ret = sensor_channel_get(temp_sensor, SENSOR_CHAN_PRESS, &data);
		if (ret) {
			desh_error("Failed to read pressure sensor, error %d", ret);
		} else {
			desh_print("  Pressure value: %d", (int)sensor_value_to_double(&data));
		}
		/* Pick out the ambient temperature data. */
		ret = sensor_channel_get(temp_sensor, SENSOR_CHAN_HUMIDITY, &data);
		if (ret) {
			desh_error("Failed to read humidity sensor, error %d", ret);
		} else {
			desh_print("  Humidity value: %d", (int)sensor_value_to_double(&data));
		}
	}

#endif
}

/**************************************************************************************************/

int dect_phy_ctrl_rssi_scan_results_print_and_best_channel_get(bool print_results)
{
	struct dect_phy_rssi_scan_channel_results scan_results;
	int err = dect_phy_scan_rssi_channel_results_get(&scan_results);
	int best_channel = -ENODATA;

	if (err) {
		desh_error("RSSI scan results not available.");
		return -ENODATA;
	}
	best_channel = dect_phy_scan_rssi_results_based_best_channel_get();

	if (print_results) {
		desh_print("--------------------------------------------------------------"
			   "---------------");
		desh_print(" RSSI scan done. Found %d free, %d possible and "
			   "%d busy channels.",
			   scan_results.free_channels_count, scan_results.possible_channels_count,
			   scan_results.busy_channels_count);
	}

	if (best_channel <= 0) {
		desh_print("   No best channel found.");
		return -ENODATA;
	}

	char final_verdict_str[10];
	struct dect_phy_rssi_scan_data_result scan_result;

	if (print_results) {
		desh_print("   Best channel: %d", best_channel);
	}

	err = dect_phy_scan_rssi_results_get_by_channel(best_channel, &scan_result);
	if (err) {
		desh_warn("   No scan result found for best channel.");
		return best_channel;
	}

	if (print_results) {
		dect_phy_rssi_scan_result_verdict_to_string(scan_result.result, final_verdict_str);

		desh_print("   Final verdict: %s", final_verdict_str);

		if (ctrl_data.rssi_scan_params.result_verdict_type ==
		    DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT) {
			desh_print("     Free subslots: %d",
				   scan_result.subslot_count_type_results.free_subslot_count);
			desh_print("     Possible subslots: %d",
				   scan_result.subslot_count_type_results.possible_subslot_count);
			desh_print("     Busy subslots: %d",
				   scan_result.subslot_count_type_results.busy_subslot_count);
		}
	}
	return best_channel;
}

static void dect_phy_ctrl_rssi_scan_timer_handler(struct k_timer *timer_id)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RSSI_SCAN_RUN);
}
K_TIMER_DEFINE(rssi_scan_timer, dect_phy_ctrl_rssi_scan_timer_handler, NULL);

static void dect_phy_rssi_channel_scan_completed_cb(enum nrf_modem_dect_phy_err phy_status)
{
	(void)dect_phy_ctrl_rssi_scan_results_print_and_best_channel_get(true);

	if (phy_status == NRF_MODEM_DECT_PHY_SUCCESS) {
		k_sem_give(&rssi_scan_sema);
	} else {
		char tmp_str[128] = {0};

		dect_common_utils_modem_phy_err_to_string(
			phy_status, NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED, tmp_str);

		k_sem_reset(&rssi_scan_sema); /* -EAGAIN should be returned from k_sem_take() */
		desh_warn(" RSSI scan failed: %s (%d)", tmp_str, phy_status);
	}
	dect_phy_api_scheduler_resume();
	ctrl_data.rssi_scan_ongoing = false;
	if (ctrl_data.rssi_scan_complete_cb) {
		ctrl_data.rssi_scan_complete_cb(phy_status);
	}
	if (ctrl_data.rssi_scan_cmd_running && ctrl_data.rssi_scan_params.interval_secs) {
		k_timer_start(&rssi_scan_timer, K_SECONDS(ctrl_data.rssi_scan_params.interval_secs),
			      K_NO_WAIT);
		desh_print("Restarting RSSI scanning in %d seconds.",
			   ctrl_data.rssi_scan_params.interval_secs);
	}
}

static int dect_phy_ctrl_phy_handlers_init(void)
{
	int ret;

	dect_phy_ctrl_mdm_op_cancel_all();

	ret = nrf_modem_dect_phy_event_handler_set(dect_phy_ctrl_mdm_evt_handler);
	if (ret) {
		desh_error("nrf_modem_dect_phy_event_handler_set returned: %i", ret);
	}

	return ret;
}

int dect_phy_ctrl_rssi_scan_start(struct dect_phy_rssi_scan_params *params,
				  dect_phy_ctrl_rssi_scan_completed_callback_t fp_result_callback)
{
	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	if (ctrl_data.rssi_scan_ongoing) {
		desh_error("RSSI scan already on going.\n");
		return -EBUSY;
	}

	k_sem_reset(&rssi_scan_sema);

	ctrl_data.rssi_scan_ongoing = true;
	ctrl_data.rssi_scan_cmd_running = true;
	ctrl_data.rssi_scan_params = *params;
	ctrl_data.rssi_scan_complete_cb = fp_result_callback;

	if (params->suspend_scheduler) {
		dect_phy_ctrl_mdm_op_cancel_all();
		dect_phy_api_scheduler_suspend();
	}

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RSSI_SCAN_RUN);
	return 0;
}

void dect_phy_ctrl_rssi_scan_stop(void)
{
	ctrl_data.rssi_scan_cmd_running = false;
	ctrl_data.rssi_scan_ongoing = false;
	k_timer_stop(&rssi_scan_timer);
	dect_phy_scan_rssi_stop();
	k_sem_reset(&rssi_scan_sema);
	dect_phy_api_scheduler_resume();
}

/**************************************************************************************************/

static bool dect_phy_ctrl_op_ongoing(void)
{
	if (ctrl_data.rssi_scan_ongoing) {
		desh_warn("RSSI scan ongoing.");
		return true;
	}
	if (ctrl_data.perf_ongoing) {
		desh_warn("Performance test ongoing.");
		return true;
	}
	if (ctrl_data.ping_ongoing) {
		desh_warn("Ping ongoing.");
		return true;
	}
	if (ctrl_data.cert_ongoing) {
		desh_warn("Certification ongoing.");
		return true;
	}
	if (ctrl_data.ext_command_running) {
		desh_warn("External command ongoing.");
		return true;
	}
	return false;
}

int dect_phy_ctrl_perf_cmd(struct dect_phy_perf_params *params)
{
	int ret;

	if (dect_phy_ctrl_op_ongoing()) {
		desh_error(
			"DECT operation already on going. Stop all before starting running perf.");
		return -EBUSY;
	}
	dect_phy_ctrl_mdm_op_cancel_all();

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.perf_ongoing = true;
	ret = dect_phy_perf_cmd_handle(params);
	if (ret) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_PERF_CMD_DONE);
	}

	return ret;
}

void dect_phy_ctrl_perf_cmd_stop(void)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

	dect_phy_perf_cmd_stop();
}

/**************************************************************************************************/

int dect_phy_ctrl_ping_cmd(struct dect_phy_ping_params *params)
{
	int ret;

	if (dect_phy_ctrl_op_ongoing()) {
		desh_error("Operation already on going. Stop all before starting running ping.");
		return -EBUSY;
	}

	dect_phy_ctrl_mdm_op_cancel_all();
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.ping_ongoing = true;
	ret = dect_phy_ping_cmd_handle(params);
	if (ret) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_PING_CMD_DONE);
	}

	return ret;
}

void dect_phy_ctrl_ping_cmd_stop(void)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

	dect_phy_ping_cmd_stop();
}

/**************************************************************************************************/

int dect_phy_ctrl_rf_tool_cmd(struct dect_phy_rf_tool_params *params)
{
	int ret;

	if (dect_phy_ctrl_op_ongoing()) {
		desh_error("Operation already on going. Stop all before starting running rf_tool.");
		return -EBUSY;
	}

	dect_phy_ctrl_mdm_op_cancel_all();
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.cert_ongoing = true;
	ret = dect_phy_rf_tool_cmd_handle(params);
	if (ret) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE);
	}

	return ret;
}

void dect_phy_ctrl_rf_tool_cmd_stop(void)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

	dect_phy_rf_tool_cmd_stop();
}

/**************************************************************************************************/

int dect_phy_ctrl_th_register_default_phy_api_pcc_rcv_cb(
	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t default_pcc_rcv_cb)
{
	if (ctrl_data.default_pcc_rcv_cb != NULL) {
		printk("default_pcc_rcv_cb already registered");
		return -EINVAL;
	}

	ctrl_data.default_pcc_rcv_cb = default_pcc_rcv_cb;

	if (!ctrl_data.ext_command_running) {
		ctrl_data.ext_cmd.pcc_rcv_cb = default_pcc_rcv_cb;
	}
	return 0;
}

int dect_phy_ctrl_th_register_default_phy_api_pdc_rcv_cb(
	dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t default_pdc_rcv_cb)
{
	if (ctrl_data.default_pdc_rcv_cb != NULL) {
		printk("default_pdc_rcv_cb already registered");
		return -EINVAL;
	}

	ctrl_data.default_pdc_rcv_cb = default_pdc_rcv_cb;

	if (!ctrl_data.ext_command_running) {
		ctrl_data.ext_cmd.pdc_rcv_cb = default_pdc_rcv_cb;
	}
	return 0;
}

/**************************************************************************************************/

bool dect_phy_ctrl_mdm_phy_api_initialized(void)
{
	return ctrl_data.phy_api_initialized;
}

bool dect_phy_ctrl_phy_api_scheduler_suspended(void)
{
	return ctrl_data.scheduler_suspended;
}

/**************************************************************************************************/

int dect_phy_ctrl_ext_command_start(struct dect_phy_ctrl_ext_callbacks ext_callbacks)
{
	if (!dect_phy_ctrl_mdm_phy_api_initialized()) {
		desh_error("(%s): Phy api not initialized", (__func__));
		return -EACCES;
	}

	if (ctrl_data.rssi_scan_ongoing ||
	    ctrl_data.perf_ongoing || ctrl_data.ping_ongoing) {
		desh_error("(%s): Operation already on going. "
			   "Stop all before starting running ext command.",
				(__func__));
		return -EBUSY;
	}
	ctrl_data.ext_cmd = ext_callbacks;
	ctrl_data.ext_command_running = true;
	return 0;
}

void dect_phy_ctrl_ext_command_stop(void)
{
	ctrl_data.ext_command_running = false;

	/* De-register callbacks */
	memset(&ctrl_data.ext_cmd, 0, sizeof(struct dect_phy_ctrl_ext_callbacks));

	/* Set default PCC&PDC callbacks back */
	ctrl_data.ext_cmd.pcc_rcv_cb = ctrl_data.default_pcc_rcv_cb;
	ctrl_data.ext_cmd.pdc_rcv_cb = ctrl_data.default_pdc_rcv_cb;
}

/**************************************************************************************************/

bool dect_phy_ctrl_nbr_is_in_channel(uint16_t channel)
{
	if (ctrl_data.ext_cmd.channel_has_nbr_cb != NULL) {
		return ctrl_data.ext_cmd.channel_has_nbr_cb(channel);
	} else {
		return dect_phy_mac_nbr_is_in_channel(channel);
	}
}

/**************************************************************************************************/

int dect_phy_ctrl_activate_cmd(enum nrf_modem_dect_phy_radio_mode radio_mode)
{
	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}
	dect_phy_ctrl_phy_configure_from_settings();
	dect_phy_ctrl_radio_activate(radio_mode);

	return 0;
}

int dect_phy_ctrl_deactivate_cmd(void)
{
	int ret;

	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	ret = nrf_modem_dect_phy_deactivate();
	if (ret) {
		desh_error("nrf_modem_dect_phy_deactivate() failed, err=%d", ret);
	}
	return ret;
}

int dect_phy_ctrl_radio_mode_cmd(enum nrf_modem_dect_phy_radio_mode radio_mode)
{
	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	if (dect_phy_ctrl_op_ongoing()) {
		/* Cancel all from modem before */
		desh_warn("Operation already on going. "
			  "Canceling all operations in modem "
			  "before starting running radio mode change.");
		dect_phy_ctrl_mdm_op_cancel_all();
	}

	uint64_t first_possible_tx;
	uint64_t time_now = dect_app_modem_time_now();
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	first_possible_tx =
		time_now +
			dect_phy_ctrl_modem_latency_for_next_op_get(true) +
				US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us);

	struct nrf_modem_dect_phy_radio_config_params config_params = {
		.radio_mode = radio_mode,
		.start_time = first_possible_tx,
		.handle = DECT_PHY_RADIO_MODE_CONFIG_HANDLE,
	};
	int ret = nrf_modem_dect_phy_radio_config(&config_params);

	ctrl_data.last_set_radio_mode_successful = false;
	if (ret) {
		desh_error("nrf_modem_dect_phy_radio_config() failed, err=%d", ret);
	}

	/* Wait that radio mode configure is done */
	ret = k_sem_take(&dect_phy_ctrl_mdm_api_radio_mode_config_sema, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_radio_config() timeout.", (__func__));
		ctrl_data.last_set_radio_mode_successful = false;
	} else {
		char tmp_str[128] = {0};

		dect_common_utils_radio_mode_to_string(radio_mode, tmp_str);
		if (ctrl_data.last_set_radio_mode_successful) {
			ctrl_data.last_set_radio_mode = radio_mode;
			desh_print("DECT radio mode configured to: %s.", tmp_str);
		} else {
			desh_error("DECT radio mode configuration failed to %s.", tmp_str);
			desh_print("Last set DECT radio mode %s.",
				dect_common_utils_radio_mode_to_string(
					ctrl_data.last_set_radio_mode, tmp_str));
		}
	}
	return ret;
}

/**************************************************************************************************/

static void dect_phy_ctrl_on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ctx);

	if (ret != 0) {
		printk("Modem library did not initialize: %d\n", ret);
		return;
	}

	if (!ctrl_data.phy_api_initialized) {
		k_sem_reset(&dect_phy_ctrl_mdm_api_init_sema);

		dect_phy_ctrl_phy_init();

		/* Wait that init is done */
		ret = k_sem_take(&dect_phy_ctrl_mdm_api_init_sema, K_SECONDS(2));
		if (ret) {
			desh_error("(%s): nrf_modem_dect_phy_init() timeout.", (__func__));
		}
	}
}

NRF_MODEM_LIB_ON_INIT(dect_phy_ctrl_init_hook, dect_phy_ctrl_on_modem_lib_init, NULL);

static int dect_phy_ctrl_init(void)
{
	memset(&ctrl_data, 0, sizeof(struct dect_phy_ctrl_data));
	ctrl_data.last_valid_temperature = NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED;

	ctrl_data.debug = true; /* Debugs enabled as a default */
	k_work_queue_start(&dect_phy_ctrl_work_q, dect_phy_ctrl_th_stack,
			   K_THREAD_STACK_SIZEOF(dect_phy_ctrl_th_stack), DECT_PHY_CTRL_PRIORITY,
			   NULL);
	k_thread_name_set(&(dect_phy_ctrl_work_q.thread), "dect_phy_ctrl_worker_th");

	return 0;
}

SYS_INIT(dect_phy_ctrl_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
