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

#include "desh_print.h"
#include "nrf_modem_dect_phy.h"

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
#include "dect_phy_ctrl.h"

#define DECT_PHY_CTRL_STACK_SIZE 4096
#define DECT_PHY_CTRL_PRIORITY	 K_PRIO_COOP(9) /* -7 */
K_THREAD_STACK_DEFINE(dect_phy_ctrl_th_stack, DECT_PHY_CTRL_STACK_SIZE);

struct k_work_q dect_phy_ctrl_work_q;

/* States of all commands */
static struct dect_phy_ctrl_data {
	bool phy_api_initialized;
	struct nrf_modem_dect_phy_init_params dect_phy_init_params;

	uint16_t phy_api_init_count;
	bool phy_api_capabilities_printed;

	bool scheduler_suspended;

	bool perf_on_going;
	bool ping_on_going;
	bool cert_on_going;
	bool ext_command_running;

	bool rx_cmd_on_going;
	struct dect_phy_rx_cmd_params rx_cmd_params;

	bool time_cmd;

	bool debug;

	int16_t last_received_pcc_pwr_dbm;
	int8_t last_received_pcc_mcs;
	uint64_t last_received_stf_start_time;
	uint8_t last_received_pcc_short_nw_id;
	uint16_t last_received_pcc_transmitter_short_rd_id;

	uint8_t last_received_pcc_phy_len;
	enum dect_phy_packet_length_type last_received_pcc_phy_len_type;

	bool rssi_scan_on_going;
	bool rssi_scan_cmd_running;
	struct dect_phy_rssi_scan_params rssi_scan_params;
	struct dect_phy_rssi_channel_scan_result_t rssi_scan_results;
	dect_phy_ctrl_rssi_scan_completed_callback_t rssi_scan_complete_cb;

	/* Registered callbacks for ext command usage */
	struct dect_phy_ctrl_ext_callbacks ext_cmd;

	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t default_pcc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t default_pdc_rcv_cb;
} ctrl_data;

K_SEM_DEFINE(rssi_scan_sema, 0, 1);

K_SEM_DEFINE(phy_api_deinit, 0, 1);
K_SEM_DEFINE(phy_api_init, 0, 1);

/**************************************************************************************************/

static void dect_phy_rssi_channel_scan_callback(
	struct dect_phy_rssi_channel_scan_result_t *p_scan_results);
static void dect_phy_ctrl_on_modem_lib_init(int ret, void *ctx);

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

static void dect_phy_ctrl_msgq_thread_handler(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_ctrl_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_CTRL_OP_SCHEDULER_SUSPENDED: {
			ctrl_data.scheduler_suspended = true;
			break;
		}
		case DECT_PHY_CTRL_OP_SCHEDULER_RESUMED: {
			ctrl_data.scheduler_suspended = false;
			break;
		}
		case DECT_PHY_CTRL_OP_PERF_CMD_DONE: {
			ctrl_data.perf_on_going = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			/* Deinit was done by perf module:
			 * reinit phy api to have our callbacks back
			 */
			dect_phy_ctrl_on_modem_lib_init(0, NULL);
			break;
		}
		case DECT_PHY_CTRL_OP_PING_CMD_DONE: {
			ctrl_data.ping_on_going = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			/* Deinit was done by ping module:
			 * reinit phy api to have our callbacks back
			 */
			dect_phy_ctrl_on_modem_lib_init(0, NULL);
			break;
		}
		case DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE: {
			ctrl_data.cert_on_going = false;
			dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_ON);

			/* Deinit was done by rf tool module:
			 * reinit phy api to have our callbacks back
			 */
			dect_phy_ctrl_on_modem_lib_init(0, NULL);
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
			struct dect_phy_common_op_initialized_params *params =
				(struct dect_phy_common_op_initialized_params *)event.data;

			if (params->status) {
				desh_error("(%s): init failed (time %llu, temperature %d, "
					   "temp_limit %d): %d",
					   (__func__), params->time, params->temperature,
					   params->modem_configuration.temperature_limit,
					   params->status);
			} else {
				if (ctrl_data.phy_api_init_count <= 1) {
					desh_print("DECT modem initialized:");
					desh_print("  current temperature:       %dC",
						   params->temperature);
					desh_print("  max operating temperature: %dC",
						   params->modem_configuration.temperature_limit);
					desh_print("  Latencies (in mdm time)");
					desh_print(
						"    from Idle to RSSI RF sampling start: %d",
						params->modem_configuration.latency.idle_to_rssi);
					desh_print(
						"    from RF sampling of RSSI to Idle: %d",
						params->modem_configuration.latency.rssi_to_idle);
					desh_print("    from Idle to RX RF sampling start: %d",
						   params->modem_configuration.latency.idle_to_rx);
					desh_print("    from RF sampling of RX to Idle: %d",
						   params->modem_configuration.latency.rx_to_idle);
					desh_print("    from RF sampling of RX with RSSI "
						   "measurement to "
						   "Idle: %d",
						   params->modem_configuration.latency
							   .rx_rssi_to_idle);
					desh_print(
						"    from RX stop request to RF sampling stop: %d",
						params->modem_configuration.latency
							.rx_stop_to_rf_off);
					desh_print("    from Idle to TX RF sample transmission "
						   "start: %d",
						   params->modem_configuration.latency.idle_to_tx);
					desh_print("    from Idle to TX with LBT monitoring RF "
						   "sampling "
						   "start: %d",
						   params->modem_configuration.latency
							   .idle_to_tx_with_lbt);
					desh_print(
						"    from TX RF sample transmission end (with or "
						"without LBT) to Idle: %d",
						params->modem_configuration.latency.tx_to_idle);
					desh_print("    from Idle to TX with LBT monitoring RF "
						   "sampling start "
						   "in TX-RX operation: %d",
						   params->modem_configuration.latency
							   .idle_to_txrx_with_lbt);
					desh_print(
						"    from TX RF sample transmission end to RX RF "
						"sampling start in TX-RX operation: %d",
						params->modem_configuration.latency.txrx_tx_to_rx);
				}
			}
			k_sem_give(&phy_api_init);
			break;
		}

		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC_CRC_ERROR: {
			struct dect_phy_common_op_pcc_crc_fail_params *params =
				(struct dect_phy_common_op_pcc_crc_fail_params *)event.data;

			desh_warn("RX PCC CRC error (time %llu): SNR %d, RSSI-2 %d (%d dBm)",
				  params->time, params->crc_failure.snr, params->crc_failure.rssi_2,
				  (params->crc_failure.rssi_2 / 2));
			break;
		}
		case DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_CRC_ERROR: {
			struct dect_phy_common_op_pdc_crc_fail_params *params =
				(struct dect_phy_common_op_pdc_crc_fail_params *)event.data;

			desh_warn("RX PDC CRC error (time %llu): SNR %d, RSSI-2 %d (%d dBm)",
				  params->time, params->crc_failure.snr, params->crc_failure.rssi_2,
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

				desh_print("PCC received (stf start time %llu): "
					   "status: \"%s\", snr %d, "
					   "RSSI-2 %d (RSSI %d)",
					   params->stf_start_time, tmp_str, params->pcc_status.snr,
					   params->pcc_status.rssi_2, rssi_level);
			}
			if (params->pcc_status.header_status ==
			    NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
				if (ctrl_data.debug) {
					desh_print("  phy header: short nw id %d (0x%02x), "
						   "transmitter id %u",
						   params->short_nw_id, params->short_nw_id,
						   params->transmitter_short_rd_id);
					desh_print("  receiver id: %u",
						   params->receiver_short_rd_id);
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

			if (ctrl_data.ext_cmd.pdc_rcv_cb != NULL) {
				data_handled = ctrl_data.ext_cmd.pdc_rcv_cb(params);
			}

			if (!data_handled) {
				unsigned char hex_data[128];
				int i;
				struct nrf_modem_dect_phy_rx_pdc_status *p_rx_status =
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

			if (params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				char tmp_str[128] = {0};

				dect_common_utils_modem_phy_err_to_string(
					params->status, params->temperature, tmp_str);
				if (params->handle == DECT_PHY_COMMON_RSSI_SCAN_HANDLE) {
					dect_phy_scan_rssi_finished_handle(params->status);
				} else if (params->handle == DECT_PHY_COMMON_RX_CMD_HANDLE) {
					dect_phy_api_scheduler_resume();
					desh_error("%s: cannot start RX: %s", __func__, tmp_str);
					ctrl_data.rx_cmd_on_going = false;
				} else {
					if (ctrl_data.ext_cmd.op_complete_cb != NULL) {
						ctrl_data.ext_cmd.op_complete_cb(params);
					}
				}
			} else {
				if (params->handle == DECT_PHY_COMMON_RX_CMD_HANDLE) {
					ctrl_data.rx_cmd_on_going = false;
					dect_phy_api_scheduler_resume();
					desh_print("RX DONE.");
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
			if (ctrl_data.rssi_scan_on_going ||
			    (ctrl_data.rssi_scan_cmd_running &&
			     ctrl_data.rssi_scan_params.interval_secs)) {
				if (ctrl_data.rssi_scan_params.suspend_scheduler) {
					dect_phy_api_scheduler_suspend();
				}
				dect_phy_scan_rssi_start(&ctrl_data.rssi_scan_params,
							 dect_phy_rssi_channel_scan_callback);
			}
			break;
		}
		case DECT_PHY_CTRL_OP_PHY_API_MDM_RSSI_CB_ON_RX_OP: {
			if (ctrl_data.rx_cmd_on_going || ctrl_data.ext_command_running) {
				dect_phy_scan_rssi_finished_handle(NRF_MODEM_DECT_PHY_SUCCESS);
				dect_phy_scan_rssi_data_reinit_with_current_params();
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

static void dect_phy_ctrl_mdm_initialize_cb(const uint64_t *time, int16_t temperature,
				     enum nrf_modem_dect_phy_err status,
				     const struct nrf_modem_dect_phy_modem_cfg *modem_configuration)
{
	struct dect_phy_common_op_initialized_params ctrl_op_initialized_params;

	dect_app_modem_time_save(time);

	ctrl_op_initialized_params.time = *time;
	ctrl_op_initialized_params.temperature = temperature;
	ctrl_op_initialized_params.status = status;
	ctrl_op_initialized_params.modem_configuration = *modem_configuration;

	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		ctrl_data.phy_api_init_count++;
		ctrl_data.phy_api_initialized = true;
	}

	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_INITIALIZED,
				       (void *)&ctrl_op_initialized_params,
				       sizeof(struct dect_phy_common_op_initialized_params));
}

static void dect_phy_ctrl_mdm_time_query_cb(
	uint64_t const *time, enum nrf_modem_dect_phy_err status)
{
	dect_app_modem_time_save(time);

	if (ctrl_data.time_cmd) {
		if (status != NRF_MODEM_DECT_PHY_SUCCESS) {
			printk("\nCannot get modem time, err %d", status);
			return;
		}

		uint64_t time_now = dect_app_modem_time_now();
		static const char *behind_str = "behind";
		static const char *ahead_str = "ahead";
		int64_t time_diff = *time - time_now;

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

static void dect_phy_ctrl_mdm_rx_operation_stop_cb(uint64_t const *time,
					    enum nrf_modem_dect_phy_err status, uint32_t handle)
{
	dect_app_modem_time_save(time);
}

static void dect_phy_ctrl_mdm_operation_complete_cb(uint64_t const *time, int16_t temperature,
					     enum nrf_modem_dect_phy_err status, uint32_t handle)
{
	struct dect_phy_common_op_completed_params ctrl_op_completed_params = {
		.handle = handle,
		.temperature = temperature,
		.status = status,
		.time = *time,
	};
	struct dect_phy_api_scheduler_op_completed_params op_completed_params = {
		.handle = handle,
		.status = status,
		.time = *time,
	};
	dect_app_modem_time_save(time);

	dect_phy_api_scheduler_mdm_op_completed(&op_completed_params);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_COMPLETED,
				       (void *)&ctrl_op_completed_params,
				       sizeof(struct dect_phy_common_op_completed_params));
}

static void
dect_phy_ctrl_mdm_on_rx_pcc_cb(uint64_t const *time,
			       struct nrf_modem_dect_phy_rx_pcc_status const *p_rx_status,
			       union nrf_modem_dect_phy_hdr const *p_phy_header)
{
	struct dect_phy_common_op_pcc_rcv_params ctrl_pcc_op_params;

	ctrl_data.last_received_stf_start_time = p_rx_status->stf_start_time;
	dect_app_modem_time_save(time);

	if (p_rx_status->header_status == NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
		struct dect_phy_ctrl_field_common *phy_h = (void *)p_phy_header;
		const uint8_t *p_ptr = (uint8_t *)p_phy_header;

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

	ctrl_pcc_op_params.pcc_status = *p_rx_status;
	ctrl_pcc_op_params.phy_header = *p_phy_header;
	ctrl_pcc_op_params.time = *time;
	ctrl_pcc_op_params.stf_start_time = p_rx_status->stf_start_time;

	if (ctrl_data.ext_cmd.direct_pcc_rcv_cb != NULL) {
		ctrl_data.ext_cmd.direct_pcc_rcv_cb(&ctrl_pcc_op_params);
	}

	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC,
				       (void *)&ctrl_pcc_op_params,
				       sizeof(struct dect_phy_common_op_pcc_rcv_params));
}

static void dect_phy_ctrl_mdm_on_rx_pdc_cb(uint64_t const *time,
			       struct nrf_modem_dect_phy_rx_pdc_status const *p_rx_status,
			       void const *p_data, uint32_t length)
{
	int16_t rssi_level = p_rx_status->rssi_2 / 2;

	dect_app_modem_time_save(time);

	struct dect_phy_commmon_op_pdc_rcv_params ctrl_pdc_op_params;
	struct dect_phy_api_scheduler_op_pdc_type_rcvd_params pdc_op_params;

	ctrl_pdc_op_params.rx_status = *p_rx_status;

	ctrl_pdc_op_params.data_length = length;
	ctrl_pdc_op_params.time = ctrl_data.last_received_stf_start_time;

	ctrl_pdc_op_params.rx_pwr_dbm = ctrl_data.last_received_pcc_pwr_dbm;
	ctrl_pdc_op_params.rx_mcs = ctrl_data.last_received_pcc_mcs;
	ctrl_pdc_op_params.rx_rssi_level_dbm = rssi_level;
	ctrl_pdc_op_params.last_rx_op_channel = dect_phy_common_rx_get_last_rx_op_channel();
	ctrl_pdc_op_params.last_received_pcc_short_nw_id = ctrl_data.last_received_pcc_short_nw_id;
	ctrl_pdc_op_params.last_received_pcc_transmitter_short_rd_id =
		ctrl_data.last_received_pcc_transmitter_short_rd_id;
	ctrl_pdc_op_params.last_received_pcc_phy_len_type =
		ctrl_data.last_received_pcc_phy_len_type;
	ctrl_pdc_op_params.last_received_pcc_phy_len = ctrl_data.last_received_pcc_phy_len;

	if (length <= sizeof(ctrl_pdc_op_params.data)) {
		memcpy(ctrl_pdc_op_params.data, p_data, length);
		if (ctrl_data.ext_cmd.direct_pdc_rcv_cb != NULL) {
			ctrl_data.ext_cmd.direct_pdc_rcv_cb(&ctrl_pdc_op_params);
		}

		dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_DATA,
					       (void *)&ctrl_pdc_op_params,
					       sizeof(struct dect_phy_commmon_op_pdc_rcv_params));
	} else {
		printk("Received data is too long to be received by CTRL TH - discarded (len %d, "
		       "buf size %d)\n",
		       length, sizeof(ctrl_pdc_op_params.data));
	}

	pdc_op_params.data_length = length;
	pdc_op_params.time = ctrl_data.last_received_stf_start_time;
	pdc_op_params.rx_pwr_dbm = ctrl_data.last_received_pcc_pwr_dbm;
	pdc_op_params.rx_rssi_dbm = rssi_level;
	if (length <= sizeof(pdc_op_params.data)) {
		memcpy(pdc_op_params.data, p_data, length);
		dect_phy_api_scheduler_mdm_pdc_data_recv(&pdc_op_params);
	} else {
		printk("Received data is too long to be received by SCHEDULER TH - discarded (len "
		       "%d, buf size %d)\n",
		       length, sizeof(pdc_op_params.data));
	}
}

static void dect_phy_ctrl_mdm_on_rssi_cb(const uint64_t *time,
				  const struct nrf_modem_dect_phy_rssi_meas *p_result)
{
	dect_app_modem_time_save(time);
	dect_phy_scan_rssi_cb_handle(NRF_MODEM_DECT_PHY_SUCCESS, p_result);
}

static void dect_phy_ctrl_mdm_on_link_configuration_cb(uint64_t const *time,
						enum nrf_modem_dect_phy_err status)
{
	dect_app_modem_time_save(time);
}

static void dect_phy_ctrl_mdm_on_pcc_crc_failure_cb(
	uint64_t const *time, struct nrf_modem_dect_phy_rx_pcc_crc_failure const *crc_failure)
{
	struct dect_phy_common_op_pcc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *crc_failure,
	};

	dect_app_modem_time_save(time);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pcc_crc_fail_params));
}

static void dect_phy_ctrl_mdm_on_pdc_crc_failure_cb(
	uint64_t const *time, struct nrf_modem_dect_phy_rx_pdc_crc_failure const *crc_failure)
{
	struct dect_phy_common_op_pdc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *crc_failure,
	};

	dect_app_modem_time_save(time);
	dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pdc_crc_fail_params));
}

static void dect_phy_ctrl_mdm_on_capability_get_cb(
	const uint64_t *time, enum nrf_modem_dect_phy_err err,
	const struct nrf_modem_dect_phy_capability *capabilities)
{
	if (!ctrl_data.phy_api_capabilities_printed) {
		printk("\nModem capabilities:\n");
		printk("  DECT NR+ version: %d\n", capabilities->dect_version);
		for (int i = 0; i < capabilities->variant_count; i++) {
			printk("    power_class[%d]:            %d\n", i,
			       capabilities->variant[i].power_class);
			printk("    rx_spatial_streams[%d]:     %d\n", i,
			       capabilities->variant[i].rx_spatial_streams);
			printk("    rx_tx_diversity[%d]:        %d\n", i,
			       capabilities->variant[i].rx_tx_diversity);
			printk("    rx_gain[%d]:                %ddBm\n", i,
			       capabilities->variant[i].rx_gain);
			printk("    mcs_max[%d]:                %d\n", i,
			       capabilities->variant[i].mcs_max);
			printk("    harq_soft_buf_size[%d]:     %d\n", i,
			       capabilities->variant[i].harq_soft_buf_size);
			printk("    harq_process_count_max[%d]: %d\n", i,
			       capabilities->variant[i].harq_process_count_max);
			printk("    harq_feedback_delay[%d]:    %d\n", i,
			       capabilities->variant[i].harq_feedback_delay);
			printk("    mu[%d]:                     %d\n", i,
			       capabilities->variant[i].mu);
			printk("    beta[%d]:                   %d\n", i,
			       capabilities->variant[i].beta);
		}
		ctrl_data.phy_api_capabilities_printed = true;
	}
}

static void dect_phy_ctrl_mdm_on_stf_cover_seq_control_cb(
	const uint64_t *time, enum nrf_modem_dect_phy_err err)
{
	printk("WARN: Unexpectedly in %s\n", (__func__));
}

static void dect_phy_ctrl_mdm_on_deinit_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err)
{
	ctrl_data.phy_api_initialized = false;
	k_sem_give(&phy_api_deinit);
}

static const struct nrf_modem_dect_phy_callbacks dect_phy_api_config = {

	.init = dect_phy_ctrl_mdm_initialize_cb,
	.rx_stop = dect_phy_ctrl_mdm_rx_operation_stop_cb,
	.op_complete = dect_phy_ctrl_mdm_operation_complete_cb,
	.pcc = dect_phy_ctrl_mdm_on_rx_pcc_cb,
	.pcc_crc_err = dect_phy_ctrl_mdm_on_pcc_crc_failure_cb,
	.pdc = dect_phy_ctrl_mdm_on_rx_pdc_cb,
	.pdc_crc_err = dect_phy_ctrl_mdm_on_pdc_crc_failure_cb,
	.rssi = dect_phy_ctrl_mdm_on_rssi_cb,
	.link_config = dect_phy_ctrl_mdm_on_link_configuration_cb,
	.time_get = dect_phy_ctrl_mdm_time_query_cb,
	.capability_get = dect_phy_ctrl_mdm_on_capability_get_cb,
	.stf_cover_seq_control = dect_phy_ctrl_mdm_on_stf_cover_seq_control_cb,
	.deinit = dect_phy_ctrl_mdm_on_deinit_cb,
};

static void dect_phy_ctrl_phy_init(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int ret = nrf_modem_dect_phy_callback_set(&dect_phy_api_config);

	ctrl_data.dect_phy_init_params.harq_rx_expiry_time_us =
		current_settings->harq.mdm_init_harq_expiry_time_us;
	ctrl_data.dect_phy_init_params.harq_rx_process_count =
		current_settings->harq.mdm_init_harq_process_count;
	if (ret) {
		printk("nrf_modem_dect_phy_callback_set returned: %i\n", ret);
	} else {
		ret = nrf_modem_dect_phy_init(&ctrl_data.dect_phy_init_params);
		if (ret) {
			printk("nrf_modem_dect_phy_init returned: %i\n", ret);
		} else {
			ret = nrf_modem_dect_phy_capability_get(); /* asynch: result in callback */
			if (ret) {
				printk("nrf_modem_dect_phy_capability_get returned: %i\n", ret);
			}
		}
	}
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

/**************************************************************************************************/

void dect_phy_ctrl_rx_stop(void)
{
	ctrl_data.rx_cmd_on_going = false;
	(void)nrf_modem_dect_phy_rx_stop(DECT_PHY_COMMON_RX_CMD_HANDLE);
	dect_phy_api_scheduler_resume();
}

int dect_phy_ctrl_rx_start(struct dect_phy_rx_cmd_params *params)
{
	int err;

	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	if (ctrl_data.rx_cmd_on_going) {
		desh_error("rx cmd already on going.");
		return -EALREADY;
	}

	if (params->suspend_scheduler) {
		dect_phy_api_scheduler_suspend();
	}

	struct dect_phy_rssi_scan_params rssi_params = {
		.channel = params->channel,
		.busy_rssi_limit = params->busy_rssi_limit,
		.free_rssi_limit = params->free_rssi_limit,
		.interval_secs = params->rssi_interval_secs,
	};

	ctrl_data.rx_cmd_params = *params;
	dect_phy_scan_rssi_data_init(&rssi_params);

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
	int temperature;
	int ret;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

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

	ret = nrf_modem_at_scanf("AT%XTEMP?", "%%XTEMP: %d", &temperature);
	if (ret <= 0) {
		sprintf(tmp_mdm_str, "%s", "not known");
	} else {
		sprintf(tmp_mdm_str, "%d", temperature);
	}
	desh_print("  Modem temperature: %sC", tmp_mdm_str);
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

static void dect_phy_ctrl_rssi_scan_timer_handler(struct k_timer *timer_id)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RSSI_SCAN_RUN);
}
K_TIMER_DEFINE(rssi_scan_timer, dect_phy_ctrl_rssi_scan_timer_handler, NULL);

static void dect_phy_rssi_channel_scan_callback(
	struct dect_phy_rssi_channel_scan_result_t *p_scan_results)
{
	memcpy(ctrl_data.rssi_scan_results.free_channels, p_scan_results->free_channels,
	       p_scan_results->free_channels_count * sizeof(uint16_t));
	ctrl_data.rssi_scan_results.free_channels_count = p_scan_results->free_channels_count;

	memcpy(ctrl_data.rssi_scan_results.possible_channels, p_scan_results->possible_channels,
	       p_scan_results->possible_channels_count * sizeof(uint16_t));
	ctrl_data.rssi_scan_results.possible_channels_count =
		p_scan_results->possible_channels_count;

	memcpy(ctrl_data.rssi_scan_results.busy_channels, p_scan_results->busy_channels,
	       p_scan_results->busy_channels_count * sizeof(uint16_t));
	ctrl_data.rssi_scan_results.busy_channels_count = p_scan_results->busy_channels_count;

	desh_print(" RSSI scan done. Found %d free, %d possible and %d busy channels.",
		   ctrl_data.rssi_scan_results.free_channels_count,
		   ctrl_data.rssi_scan_results.possible_channels_count,
		   ctrl_data.rssi_scan_results.busy_channels_count);

	if (p_scan_results->phy_status == NRF_MODEM_DECT_PHY_SUCCESS) {
		k_sem_give(&rssi_scan_sema);
	} else {
		char tmp_str[128] = {0};

		dect_common_utils_modem_phy_err_to_string(
			p_scan_results->phy_status, NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED, tmp_str);

		k_sem_reset(&rssi_scan_sema); /* -EAGAIN should be returned from k_sem_take() */
		desh_warn(" RSSI scan failed: %s (%d)", tmp_str, p_scan_results->phy_status);
	}
	dect_phy_api_scheduler_resume();
	ctrl_data.rssi_scan_on_going = false;
	if (ctrl_data.rssi_scan_complete_cb) {
		ctrl_data.rssi_scan_complete_cb(p_scan_results->phy_status);
	}
	if (ctrl_data.rssi_scan_cmd_running && ctrl_data.rssi_scan_params.interval_secs) {
		k_timer_start(&rssi_scan_timer, K_SECONDS(ctrl_data.rssi_scan_params.interval_secs),
			      K_NO_WAIT);
		desh_print("Restarting RSSI scanning in %d seconds.",
			   ctrl_data.rssi_scan_params.interval_secs);
	}
}

static int dect_phy_ctrl_phy_reinit(void)
{
	int ret;

	k_sem_reset(&phy_api_deinit);

	ret = nrf_modem_dect_phy_deinit();

	if (ret) {
		desh_error("nrf_modem_dect_phy_deinit failed, err=%d", ret);
		return ret;
	}

	/* Wait that deinit is done */
	ret = k_sem_take(&phy_api_deinit, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_deinit() timeout.", (__func__));
		return -ETIMEDOUT;
	}

	k_sem_reset(&phy_api_init);
	dect_phy_ctrl_phy_init();

	ret = k_sem_take(&phy_api_init, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_init() timeout.", (__func__));
		return -ETIMEDOUT;
	}
	return 0;
}

int dect_phy_ctrl_rssi_scan_start(struct dect_phy_rssi_scan_params *params,
				  dect_phy_ctrl_rssi_scan_completed_callback_t fp_result_callback)
{
	int ret;

	if (!ctrl_data.phy_api_initialized) {
		desh_error("Phy api not initialized");
		return -EACCES;
	}

	if (ctrl_data.rssi_scan_on_going) {
		desh_error("RSSI scan already on going.\n");
		return -EBUSY;
	}

	k_sem_reset(&rssi_scan_sema);

	ctrl_data.rssi_scan_on_going = true;
	ctrl_data.rssi_scan_cmd_running = true;
	ctrl_data.rssi_scan_params = *params;
	ctrl_data.rssi_scan_complete_cb = fp_result_callback;

	if (params->suspend_scheduler) {
		dect_phy_api_scheduler_suspend();
	}

	if (params->reinit_mdm_api) {
		ret = dect_phy_ctrl_phy_reinit();
		if (ret) {
			return ret;
		}
	}

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RSSI_SCAN_RUN);
	return 0;
}

void dect_phy_ctrl_rssi_scan_stop(void)
{
	ctrl_data.rssi_scan_cmd_running = false;
	ctrl_data.rssi_scan_on_going = false;
	k_timer_stop(&rssi_scan_timer);
	dect_phy_scan_rssi_stop();
	k_sem_reset(&rssi_scan_sema);
	dect_phy_api_scheduler_resume();
}

/**************************************************************************************************/

int dect_phy_ctrl_perf_cmd(struct dect_phy_perf_params *params)
{
	int ret;

	if (ctrl_data.rssi_scan_on_going || ctrl_data.perf_on_going || ctrl_data.ping_on_going ||
	    ctrl_data.cert_on_going || ctrl_data.ext_command_running) {
		desh_error(
			"DECT operation already on going. Stop all before starting running perf.");
		return -EBUSY;
	}
	k_sem_reset(&phy_api_deinit);

	/* We want to have own callbacks for perf command */
	ret = nrf_modem_dect_phy_deinit();
	if (ret) {
		desh_error("nrf_modem_dect_phy_deinit failed, err=%d", ret);
		return ret;
	}

	/* Wait that deinit is done */
	ret = k_sem_take(&phy_api_deinit, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_deinit() timeout.", (__func__));
		return -ETIMEDOUT;
	}

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.perf_on_going = true;
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

	if (ctrl_data.rssi_scan_on_going || ctrl_data.perf_on_going || ctrl_data.ping_on_going ||
	    ctrl_data.cert_on_going || ctrl_data.ext_command_running) {
		desh_error("Operation already on going. Stop all before starting running ping.");
		return -EBUSY;
	}
	k_sem_reset(&phy_api_deinit);

	/* We want to have own mdm phy api callbacks for ping command */
	ret = nrf_modem_dect_phy_deinit();
	if (ret) {
		desh_error("nrf_modem_dect_phy_deinit failed, err=%d", ret);
		return ret;
	}

	/* Wait that deinit is done */
	ret = k_sem_take(&phy_api_deinit, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_deinit() timeout.", (__func__));
		return -ETIMEDOUT;
	}

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.ping_on_going = true;
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

int dect_phy_ctrl_cert_cmd(struct dect_phy_rf_tool_params *params)
{
	int ret;

	if (ctrl_data.rssi_scan_on_going || ctrl_data.perf_on_going || ctrl_data.ping_on_going ||
	    ctrl_data.cert_on_going || ctrl_data.ext_command_running) {
		desh_error("Operation already on going. Stop all before starting running rf_tool.");
		return -EBUSY;
	}
	k_sem_reset(&phy_api_deinit);

	/* We want to have own mdm phy api callbacks for cert command */
	ret = nrf_modem_dect_phy_deinit();
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_deinit failed, err=%d", (__func__), ret);
		return ret;
	}

	/* Wait that deinit is done */
	ret = k_sem_take(&phy_api_deinit, K_SECONDS(5));
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_deinit() timeout.", (__func__));
		return -ETIMEDOUT;
	}

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_DEBUG_OFF);
	ctrl_data.cert_on_going = true;
	ret = dect_phy_rf_tool_cmd_handle(params);
	if (ret) {
		dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE);
	}

	return ret;
}

void dect_phy_ctrl_cert_cmd_stop(void)
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
	if (!dect_phy_ctrl_mdm_phy_api_initialized() || ctrl_data.rssi_scan_on_going ||
	    ctrl_data.perf_on_going || ctrl_data.ping_on_going) {
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
		return false;
	}
}

/**************************************************************************************************/

static void dect_phy_ctrl_on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ret);
	ARG_UNUSED(ctx);

	if (!ctrl_data.phy_api_initialized) {
		dect_phy_ctrl_phy_init();
	}
}

NRF_MODEM_LIB_ON_INIT(dect_phy_ctrl_init_hook, dect_phy_ctrl_on_modem_lib_init, NULL);

static int dect_phy_ctrl_init(void)
{
	memset(&ctrl_data, 0, sizeof(struct dect_phy_ctrl_data));

	ctrl_data.debug = true; /* Debugs enabled as a default */
	k_work_queue_start(&dect_phy_ctrl_work_q, dect_phy_ctrl_th_stack,
			   K_THREAD_STACK_SIZEOF(dect_phy_ctrl_th_stack), DECT_PHY_CTRL_PRIORITY,
			   NULL);
	k_thread_name_set(&(dect_phy_ctrl_work_q.thread), "dect_phy_ctrl_worker_th");

	return 0;
}

SYS_INIT(dect_phy_ctrl_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
