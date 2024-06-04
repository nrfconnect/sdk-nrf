/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>

#if defined(CONFIG_MODEM_INFO)
#include <modem/modem_info.h>
#include <nrf_modem_at.h>
#endif

#include <memfault/metrics/metrics.h>
#include <memfault/core/platform/overrides.h>

#include "memfault_ncs_metrics.h"

LOG_MODULE_DECLARE(memfault_ncs_metrics, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

static bool connected;

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
static struct memfault_ncs_metrics_thread lte_metrics_thread = {
	.thread_name = "connection_poll_thread",
	.key = MEMFAULT_METRICS_KEY(ncs_connection_poll_unused_stack)
};
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */

static void lte_trace_cb(enum lte_lc_trace_type type)
{
	LOG_DBG("LTE trace: %d", type);

	switch (type) {
	case LTE_LC_TRACE_FUNC_MODE_NORMAL:
		__fallthrough;
	case LTE_LC_TRACE_FUNC_MODE_ACTIVATE_LTE:
		MEMFAULT_METRIC_TIMER_START(ncs_lte_on_time_ms);
		MEMFAULT_METRIC_TIMER_START(ncs_lte_time_to_connect_ms);
		break;
	case LTE_LC_TRACE_FUNC_MODE_POWER_OFF:
		__fallthrough;
	case LTE_LC_TRACE_FUNC_MODE_OFFLINE:
		__fallthrough;
	case LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_LTE:
		MEMFAULT_METRIC_TIMER_STOP(ncs_lte_on_time_ms);
		break;
	default:
		break;
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	int err;

#if defined(CONFIG_MODEM_INFO)
	int rsrp;
	uint8_t band;
	int snr;

	err = modem_info_get_rsrp(&rsrp);
	if (err) {
		LOG_WRN("LTE RSRP value collection failed, error: %d", err);
	} else {
		err = MEMFAULT_METRIC_SET_SIGNED(ncs_lte_rsrp_dbm, rsrp);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_rsrp_dbm");
		}
	};

	err = modem_info_get_current_band(&band);
	if (err != 0) {
		LOG_WRN("Network band collection failed, error: %d", err);
	} else {
		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_band, band);
		if (err) {
			LOG_ERR("Failed to set nce_lte_band");
		}
	}

	err = modem_info_get_snr(&snr);
	if (err != 0) {
		LOG_WRN("SNR collection failed, error: %d", err);
	} else {
		err = MEMFAULT_METRIC_SET_SIGNED(ncs_lte_snr_decibels, snr);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_snr_decibels");
		}
	}
#endif

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		switch (evt->nw_reg_status) {

		case LTE_LC_NW_REG_REGISTERED_HOME:
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			connected = true;
			MEMFAULT_METRIC_TIMER_STOP(ncs_lte_time_to_connect_ms);
			break;
		case LTE_LC_NW_REG_NOT_REGISTERED:
		case LTE_LC_NW_REG_SEARCHING:
		case LTE_LC_NW_REG_REGISTRATION_DENIED:
		case LTE_LC_NW_REG_UNKNOWN:
		case LTE_LC_NW_REG_UICC_FAIL:
			if (connected) {
				err = MEMFAULT_METRIC_ADD(ncs_lte_connection_loss_count, 1);
				if (err) {
					LOG_ERR("Fail to increment ncs_lte_connection_loss_count");
				}

				MEMFAULT_METRIC_TIMER_START(ncs_lte_time_to_connect_ms);
			}

			connected = false;
			break;
		default:
			break;
		}
	case LTE_LC_EVT_PSM_UPDATE:
		err = MEMFAULT_METRIC_SET_SIGNED(ncs_lte_psm_tau_seconds, evt->psm_cfg.tau);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_psm_tau_seconds");
		}

		err = MEMFAULT_METRIC_SET_SIGNED(ncs_lte_psm_active_time_seconds,
						 evt->psm_cfg.active_time);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_psm_active_time_seconds");
		}

		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_edrx_interval_ms,
						   (uint32_t)(evt->edrx_cfg.edrx * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set ncs_lte_edrx_interval_ms");
		}

		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_edrx_ptw_ms,
						   (uint32_t)(evt->edrx_cfg.ptw * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set ncs_lte_edrx_ptw_ms");
		}

		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_mode, evt->lte_mode);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_mode");
		}

		break;
	case LTE_LC_EVT_MODEM_EVENT:
		if (evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP) {
			MEMFAULT_METRIC_ADD(ncs_lte_reset_loop_detected_count, 1);
		}
		break;
	default:
		break;
	}
}

void memfault_lte_metrics_init(void)
{
	lte_lc_trace_handler_set(lte_trace_cb);
	lte_lc_register_handler(lte_handler);

#if defined(CONFIG_MODEM_INFO)
	int err;
	char buf[MODEM_INFO_FWVER_SIZE];

	modem_info_get_fw_version(buf, sizeof(buf));

	/* Ensure null-termination */
	buf[sizeof(buf) - 1] = '\0';

	MEMFAULT_METRIC_SET_STRING(ncs_lte_modem_fw_version, buf);

	err = modem_info_connectivity_stats_init();
	if (err) {
		LOG_WRN("Failed to init connectivity stats, error: %d", err);
	}
#endif

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
	err = memfault_ncs_metrics_thread_add(&lte_metrics_thread);
	if (err) {
		LOG_WRN("Failed to add thread: %s for stack unused space measurement, err: %d",
			lte_metrics_thread.thread_name, err);
	}
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */
}

void memfault_lte_metrics_update(void)
{
#if defined(CONFIG_MODEM_INFO)
	int tx_kbytes;
	int rx_kybtes;
	char operator_name[MODEM_INFO_SHORT_OP_NAME_SIZE];
	int err = modem_info_get_connectivity_stats(&tx_kbytes, &rx_kybtes);

	if (err) {
		LOG_WRN("Failed to collect connectivity stats, error: %d", err);
	} else {
		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_tx_kilobytes, tx_kbytes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_tx_kilobytes");
		}

		err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_lte_rx_kilobytes, rx_kybtes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_rx_kilobytes");
		}
	}

	err = modem_info_get_operator(operator_name, sizeof(operator_name));
	if (err != 0) {
		LOG_DBG("Network operator name was not reported by the modem");

		err = MEMFAULT_METRIC_SET_STRING(ncs_lte_operator, "Not available");
		if (err) {
			LOG_ERR("Failed to set ncs_lte_operator");
		}
	} else {
		err = MEMFAULT_METRIC_SET_STRING(ncs_lte_operator, operator_name);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_operator");
		}
	}
#endif
}
