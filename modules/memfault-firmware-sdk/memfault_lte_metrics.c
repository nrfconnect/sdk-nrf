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
static bool connect_timer_started;

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
static struct memfault_ncs_metrics_thread lte_metrics_thread = {
	.thread_name = "connection_poll_thread",
	.key = MEMFAULT_METRICS_KEY(Ncs_ConnectionPollUnusedStack)
};
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */

static void lte_trace_cb(enum lte_lc_trace_type type)
{
	int err;

	LOG_DBG("LTE trace: %d", type);

	switch (type) {
	case LTE_LC_TRACE_FUNC_MODE_NORMAL:
	case LTE_LC_TRACE_FUNC_MODE_ACTIVATE_LTE:
		if (connect_timer_started) {
			break;
		}

		err = memfault_metrics_heartbeat_timer_start(
			MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
		if (err) {
			LOG_WRN("LTE connection time tracking was not started, error: %d", err);
		} else {
			connect_timer_started = true;
		}

		break;
	default:
		break;
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	int err;

  // Get signal strength value whenever an event is handled
  int rsrp;
  err = modem_info_get_rsrp(&rsrp);
  if(err) {
    LOG_WRN("LTE RSRP value collection failed, error: %d", err);
  } else {
    err = memfault_metrics_heartbeat_set_signed(
			MEMFAULT_METRICS_KEY(Ncs_LteRsrp), rsrp);
		if (err) {
			LOG_ERR("Failed to set Ncs_LteRsrp");
		}
  };

  // Get connectivity stats (data tx and rx)
  int tx_kbytes;
  int rx_kybtes;
  err = modem_info_get_connectivity_stats(&tx_kbytes, &rx_kybtes);
  if (err) {
		LOG_WRN("LTE connectivity stats collections failed, error: %d", err);
  } else {
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(ncs_lte_tx_kilobytes), tx_kbytes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_tx_kilobytes");
		}

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(ncs_lte_rx_kilobytes), rx_kybtes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_rx_kilobytes");
		}
  }

  uint8_t band;
  err = modem_info_get_current_band(&band);
  if (err != 0) {
		LOG_WRN("Network band collection failed, error: %d", err);
  } else {
	  err = memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(ncs_lte_band), band);
	  if (err) {
			LOG_ERR("Failed to set nce_lte_band");
    }
  }
  
#if defined(CONFIG_MODEM_INFO)
  // Get the operator
  char operator_name[MODEM_INFO_MAX_SHORT_OP_NAME_SIZE];
  err = modem_info_get_operator(operator_name, sizeof(operator_name));
  if (err != 0) {
	  LOG_WRN("Network operator collection failed, error: %d", err);
  } else {
	  err = memfault_metrics_heartbeat_set_string(MEMFAULT_METRICS_KEY(ncs_lte_operator),
						      operator_name);
	  if (err) {
		  LOG_ERR("Failed to set ncs_lte_operator");
	  }
  }
#endif

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		switch (evt->nw_reg_status) {

		case LTE_LC_NW_REG_REGISTERED_HOME:
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			connected = true;

			if (!connect_timer_started) {
				LOG_WRN("Ncs_LteTimeToConnect was not started");
				break;
			}

			err = memfault_metrics_heartbeat_timer_stop(
				MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
			if (err) {
				LOG_WRN("Failed to stop LTE connection timer, error: %d", err);
			} else {
				LOG_DBG("Ncs_LteTimeToConnect stopped");
				connect_timer_started = false;
			}

			break;
		case LTE_LC_NW_REG_NOT_REGISTERED:
		case LTE_LC_NW_REG_SEARCHING:
		case LTE_LC_NW_REG_REGISTRATION_DENIED:
		case LTE_LC_NW_REG_UNKNOWN:
		case LTE_LC_NW_REG_UICC_FAIL:
			if (connected) {
				err = memfault_metrics_heartbeat_add(
					MEMFAULT_METRICS_KEY(Ncs_LteConnectionLossCount), 1);
				if (err) {
					LOG_ERR("Failed to increment Ncs_LteConnectionLossCount");
				}

				if (connect_timer_started) {
					break;
				}

				err = memfault_metrics_heartbeat_timer_start(
					MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect));
				if (err) {
					LOG_WRN("Failed to start LTE connection timer, error: %d",
						err);
				} else {
					LOG_DBG("Ncs_LteTimeToConnect started");
					connect_timer_started = true;
				}
			}

			connected = false;
			break;
		default:
			break;
		}
	case LTE_LC_EVT_PSM_UPDATE:
		err = memfault_metrics_heartbeat_set_signed(
			MEMFAULT_METRICS_KEY(Ncs_LtePsmTauSec), evt->psm_cfg.tau);
		if (err) {
			LOG_ERR("Failed to set Ncs_LtePsmTau");
		}

		err = memfault_metrics_heartbeat_set_signed(
			MEMFAULT_METRICS_KEY(Ncs_LtePsmActiveTimeSec), evt->psm_cfg.active_time);
		if (err) {
			LOG_ERR("Failed to set Ncs_LtePsmActiveTime");
		}

		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteEdrxIntervalMsec),
					     (uint32_t)(evt->edrx_cfg.edrx * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set Ncs_LteEdrxInterval");
		}

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteEdrxPtwMsec),
					     (uint32_t)(evt->edrx_cfg.ptw * MSEC_PER_SEC));
		if (err) {
			LOG_ERR("Failed to set Ncs_LteEdrxPtw");
		}

		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(Ncs_LteMode), evt->lte_mode);
		if (err) {
			LOG_ERR("Failed to set Ncs_LteMode");
		}

		break;
	case LTE_LC_EVT_MODEM_EVENT:
		if (evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP) {
			MEMFAULT_HEARTBEAT_ADD(ncs_lte_reset_loop_detected_count, 1);
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
	char buf[MODEM_INFO_FWVER_SIZE];

	modem_info_get_fw_version(buf, sizeof(buf));

	/* Ensure null-termination */
	buf[sizeof(buf) - 1] = '\0';

	memfault_metrics_heartbeat_set_string(MEMFAULT_METRICS_KEY(Ncs_LteModemFwVersion), buf);

	int err = modem_info_connectivity_stats_init();
	if (err) {
		LOG_ERR("Failed to init connectivity stats, err: %d", err);
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
	int err = modem_info_get_connectivity_stats(&tx_kbytes, &rx_kybtes);
	if (err) {
		LOG_WRN("LTE connectivity stats collections failed, error: %d", err);
	} else {
		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(ncs_lte_tx_kilobytes), tx_kbytes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_tx_kilobytes");
		}

		err = memfault_metrics_heartbeat_set_unsigned(
			MEMFAULT_METRICS_KEY(ncs_lte_rx_kilobytes), rx_kybtes);
		if (err) {
			LOG_ERR("Failed to set ncs_lte_rx_kilobytes");
		}
	}

	// Reset stats
	err = modem_info_connectivity_stats_init();
	if (err) {
		LOG_ERR("Failed to reset connectivity stats, err: %d", err);
	}
#endif
}
